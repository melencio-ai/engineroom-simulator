#include "scene/Game3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace enginewatch {
namespace {
float clamp(float value,float low,float high) {
    return std::max(low,std::min(high,value));
}
const char* pumpText(const PumpState& p) {
    return p.fault ? "FAULT" : p.running ? "RUN" : p.requested ? "STARTING" : "STOP";
}
const char* onOff(bool value) { return value ? "OPEN" : "SHUT"; }
}

Game3D::Game3D(const World3D& world):world_(world) { reset(); }

void Game3D::reset() {
    sim_.reset();
    deck_=EcrDeck;
    camera_=Vec3{-7.35f,World3D::floorHeight(deck_)+1.65f,0.0f};
    yaw_=0.0f; // Start facing the real upper-level ECR doorway.
    pitch_=-0.16f;
    panel_=Closed;
    selected_=0;
    ladder_index_=-1;
    paused_=false;
    climbing_=false;
    climb_target_deck_=EcrDeck;
    climb_elapsed_=0.0f;
    climb_start_=camera_;
    climb_end_=camera_;
}

void Game3D::move(float dt,float forward,float strafe,float turn,float look) {
    yaw_+=clamp(turn,-1.0f,1.0f)*dt*2.15f;
    if(yaw_>6.28318530718f) yaw_-=6.28318530718f;
    if(yaw_<-6.28318530718f) yaw_+=6.28318530718f;
    pitch_=clamp(pitch_+clamp(look,-1.0f,1.0f)*dt*1.20f,-0.90f,0.80f);
    forward=clamp(forward,-1.0f,1.0f);
    strafe=clamp(strafe,-1.0f,1.0f);
    const float magnitude=std::sqrt(forward*forward+strafe*strafe);
    if(magnitude>1.0f) { forward/=magnitude;strafe/=magnitude; }
    const float cs=std::cos(yaw_),sn=std::sin(yaw_);
    const float dx=(cs*forward-sn*strafe)*dt*3.10f;
    const float dz=(sn*forward+cs*strafe)*dt*3.10f;
    // Slide along collision geometry instead of walking into engines or deck voids.
    if(world_.canStand(camera_.x+dx,camera_.z,deck_)) camera_.x+=dx;
    if(world_.canStand(camera_.x,camera_.z+dz,deck_)) camera_.z+=dz;
}

void Game3D::operate() {
    const MachineryState& s=sim_.state();
    switch(selected_) {
    case 0:sim_.requestPump(MainLoPump,!s.main_pump.requested);break;
    case 1:sim_.requestPump(StandbyLoPump,!s.standby_pump.requested);break;
    case 2:sim_.setValve(MainLoPump,SuctionValve,!s.main_pump.suction_open);break;
    case 3:sim_.setValve(MainLoPump,DischargeValve,!s.main_pump.discharge_open);break;
    case 4:sim_.setValve(StandbyLoPump,SuctionValve,!s.standby_pump.suction_open);break;
    case 5:sim_.setValve(StandbyLoPump,DischargeValve,!s.standby_pump.discharge_open);break;
    default:break;
    }
}

void Game3D::beginClimb(int destination) {
    if(destination<0 || destination>=DeckCount || std::abs(destination-deck_)!=1)
        return;
    if(ladder_index_<0 || ladder_index_>=world_.ladderCount()) return;
    const Ladder3D& ladder=world_.ladder(ladder_index_);
    const float dx=camera_.x-ladder.landing_x;
    const float dz=camera_.z-ladder.landing_z;
    if(dx*dx+dz*dz>1.55f*1.55f) return;
    climb_start_=camera_;
    climb_target_deck_=destination;
    climb_end_=Vec3{ladder.landing_x,World3D::floorHeight(destination)+1.65f,
                    ladder.landing_z};
    climb_elapsed_=0.0f;
    climbing_=true;
    panel_=Closed;
}

void Game3D::advanceClimb(float dt) {
    if(!climbing_) return;
    climb_elapsed_+=dt;
    const float t=clamp(climb_elapsed_/2.60f,0.0f,1.0f);
    const Ladder3D& ladder=world_.ladder(ladder_index_);
    // The camera moves into the open shaft, ascends/descends in real world Y,
    // then comes out on the next deck. No disconnected scenes or instant teleport.
    const float inside=t<0.18f?t/0.18f:
                       t>0.82f?(1.0f-t)/0.18f:1.0f;
    camera_.x=climb_start_.x+(ladder.shaft_x-climb_start_.x)*inside;
    camera_.z=climb_start_.z+(ladder.shaft_z-climb_start_.z)*inside;
    camera_.y=climb_start_.y+(climb_end_.y-climb_start_.y)*t;
    if(t>=1.0f) {
        deck_=climb_target_deck_;
        camera_=climb_end_;
        climbing_=false;
        ladder_index_=-1;
    }
}

void Game3D::use() {
    if(panel_==Help || panel_==Strainer) { panel_=Closed;return; }
    if(panel_==Pumps) { operate();return; }
    if(panel_==Console) {
        const MachineryState& s=sim_.state();
        if(s.mission==1) {
            if(s.objective_complete) {
                sim_.beginMissionTwo();
                panel_=Closed;
            } else sim_.inspectEcrConsole();
        } else if(s.lo_pressure_alarm && !s.alarm_acknowledged) {
            sim_.acknowledgeAlarm();
        } else sim_.verifyMissionTwo();
        return;
    }
    if(panel_==Ladder) {
        const int destination=selected_==0?deck_+1:deck_-1;
        beginClimb(destination);
        return;
    }

    const StationId station=world_.lookAtStation(camera_,yaw_);
    switch(station) {
    case EcrConsoleStation:panel_=Console;break;
    case LoPumpStation:panel_=Pumps;selected_=0;break;
    case StrainerStation:sim_.inspectStrainer();panel_=Strainer;break;
    case MainLadderStation:
    case EscapeLadderStation:
        ladder_index_=station==MainLadderStation?0:1;
        panel_=Ladder;
        selected_=deck_==EcrDeck?1:0;
        break;
    default:break;
    }
}

void Game3D::update(float dt_seconds,const Input3D& in) {
    if(in.pause) { paused_=!paused_;return; }
    if(paused_) {
        if(in.interact) reset();
        return;
    }
    if(!(dt_seconds>0.0f) || !std::isfinite(dt_seconds)) return;
    const float dt=clamp(dt_seconds,0.0f,0.05f);
    sim_.step(dt);
    if(climbing_) { advanceClimb(dt);return; }

    if(in.help) { panel_=panel_==Help?Closed:Help;return; }
    if(in.back) { panel_=Closed;return; }
    if(panel_==Pumps || panel_==Ladder) {
        if(in.up) selected_=(selected_+(panel_==Pumps?5:1))%(panel_==Pumps?6:2);
        if(in.down) selected_=(selected_+1)%(panel_==Pumps?6:2);
    }
    if(in.interact) use();
    if(panel_==Closed && !climbing_) move(dt,in.forward,in.strafe,in.turn,in.look);
}

Hud3D Game3D::hud() const {
    Hud3D h{};
    const MachineryState& s=sim_.state();
    h.paused=paused_;
    h.alarm=s.lo_pressure_alarm;
    h.climbing=climbing_;
    std::snprintf(h.title,sizeof(h.title),"ENGINE WATCH 3D  |  %s",World3D::deckName(deck_));
    std::snprintf(h.status,sizeof(h.status),"LO %.2f BAR   MAIN %s   STBY %s",
                  s.lo_pressure_bar,pumpText(s.main_pump),pumpText(s.standby_pump));
    if(s.mission==1) {
        std::snprintf(h.objective,sizeof(h.objective),"%s",
            s.objective_complete?"001 DONE - VERIFY ECR TO ADVANCE":
            s.lo_pressure_bar>=3.0f?"RETURN TO DECK 04 ECR AND VERIFY":
            "001: DESCEND TO DECK 01 AND START MAIN LO PUMP");
    } else {
        std::snprintf(h.objective,sizeof(h.objective),"%s",
            s.mission_two_complete?"002 DONE - LO HEADER RESTORED":
            s.main_strainer_dp_bar>1.0f?"002: HIGH STRAINER DP - CHANGE OVER TO STANDBY":
            "002: DIAGNOSE LO PRESSURE - DECK 01 PUMP FLAT");
    }

    const StationId station=world_.lookAtStation(camera_,yaw_);
    const char* prompt="STICK WALK   D-PAD TURN   L/R LOOK   X USE";
    if(station==EcrConsoleStation) prompt="X: OPEN ECR LO CONSOLE";
    else if(station==LoPumpStation) prompt="X: OPERATE LO PUMP STATION";
    else if(station==StrainerStation) prompt="X: INSPECT MAIN LO STRAINER";
    else if(station==MainLadderStation) prompt="X: MAIN LADDER - SELECT DECK";
    else if(station==EscapeLadderStation) prompt="X: ALTERNATE ESCAPE LADDER";
    if(panel_!=Closed) prompt="D-PAD UP/DOWN SELECT   X OPERATE   O CLOSE";
    if(climbing_) prompt="CLIMBING BETWEEN DECKS - WATCH YOUR STEP";
    std::snprintf(h.prompt,sizeof(h.prompt),"%s",prompt);
    h.panel_visible=panel_!=Closed || paused_;
    if(paused_) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"SIMULATION PAUSED");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),"START: RESUME");
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),"X: RESET ALL MISSIONS / DECK");
        h.panel_count=2;
    } else if(panel_==Help) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"CONTROLS / MULTI-DECK GUIDE");
        const char* lines[]={
            "ANALOG STICK: WALK AND STRAFE",
            "D-PAD LEFT/RIGHT: TURN CAMERA",
            "L/R: LOOK DOWN / UP",
            "X: USE MACHINERY / ENTER LADDER",
            "D-PAD UP/DOWN: SELECT PANEL",
            "O: CLOSE   TRIANGLE: HELP",
            "DESCEND FROM ECR TO DECK 01"
        };
        h.panel_count=7;
        for(int i=0;i<h.panel_count;++i)
            std::snprintf(h.panel_rows[i],sizeof(h.panel_rows[i]),"%s",lines[i]);
    } else if(panel_==Ladder) {
        const Ladder3D& ladder=world_.ladder(ladder_index_);
        std::snprintf(h.panel_title,sizeof(h.panel_title),"%s LADDER | %s",
                      ladder.alternate_escape?"ESCAPE":"SERVICE",World3D::deckName(deck_));
        h.panel_count=3;
        h.selected_row=selected_;
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),"%c UP: %s",
                      selected_==0?'>':' ',deck_==EcrDeck?"NO HIGHER PLATFORM":
                      World3D::deckName(std::min(deck_+1,DeckCount-1)));
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),"%c DOWN: %s",
                      selected_==1?'>':' ',deck_==TankTopDeck?"LOWEST ACCESS LEVEL":
                      World3D::deckName(std::max(0,deck_-1)));
        std::snprintf(h.panel_rows[2],sizeof(h.panel_rows[2]),"X CLIMB    O BACK TO DECK");
    } else if(panel_==Pumps) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"DECK 01 | LOCAL LO PUMPS / VALVES");
        const char* names[]={"MAIN PUMP","STANDBY PUMP","MAIN SUCTION","MAIN DISCHARGE",
                             "STBY SUCTION","STBY DISCHARGE"};
        const char* values[]={
            pumpText(s.main_pump),pumpText(s.standby_pump),
            onOff(s.main_pump.suction_open),onOff(s.main_pump.discharge_open),
            onOff(s.standby_pump.suction_open),onOff(s.standby_pump.discharge_open)
        };
        h.panel_count=6;
        h.selected_row=selected_;
        for(int i=0;i<6;++i)
            std::snprintf(h.panel_rows[i],sizeof(h.panel_rows[i]),"%c %-19s %s",
                          selected_==i?'>':' ',names[i],values[i]);
    } else if(panel_==Console) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"DECK 04 | ECR LO SYSTEM MONITOR");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),
                      "LO HEADER: %.2f BAR  /  %.0f C",s.lo_pressure_bar,s.lo_oil_temp_c);
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),
                      "MAIN %s   STANDBY %s",pumpText(s.main_pump),pumpText(s.standby_pump));
        std::snprintf(h.panel_rows[2],sizeof(h.panel_rows[2]),
                      "MAIN STRAINER DP: %.2f BAR",s.main_strainer_dp_bar);
        std::snprintf(h.panel_rows[3],sizeof(h.panel_rows[3]),"%s",
                      s.lo_pressure_alarm?
                      (s.alarm_acknowledged?"LO LOW PRESSURE: ACK":"LO LOW PRESSURE: ALARM"):
                      "NO ACTIVE LO ALARM");
        if(s.mission==1) {
            std::snprintf(h.panel_rows[4],sizeof(h.panel_rows[4]),"%s",
                          s.objective_complete?"X: BEGIN MISSION 002":"X: VERIFY PRESSURE");
        } else {
            std::snprintf(h.panel_rows[4],sizeof(h.panel_rows[4]),"%s",
                          s.lo_pressure_alarm&&!s.alarm_acknowledged?"X: ACKNOWLEDGE ALARM":
                          s.mission_two_complete?"MISSION 002 VERIFIED":"X: VERIFY RESTORATION");
        }
        h.panel_count=5;
    } else if(panel_==Strainer) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"DECK 01 | MAIN LO SUCTION STRAINER");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),
                      "DIFFERENTIAL: %.2f BAR",s.main_strainer_dp_bar);
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),
                      "MAIN INLET: %.2f BAR",s.main_pump.suction_pressure_bar);
        std::snprintf(h.panel_rows[2],sizeof(h.panel_rows[2]),"%s",
                      s.main_strainer_dp_bar>1.0f?
                      "HIGH DP - SUCTION RESTRICTION":"DP BELOW ALARM LIMIT");
        std::snprintf(h.panel_rows[3],sizeof(h.panel_rows[3]),"%s",
                      s.strainer_inspected?"INSPECTION RECORDED":"MONITOR THE SUCTION PATH");
        h.panel_count=4;
    }
    return h;
}

} // namespace enginewatch
