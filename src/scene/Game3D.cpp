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
    paused_=false;
    climbing_=false;
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
    const float feet=camera_.y-1.65f;
    const float speed=world_.onStair(camera_.x,camera_.z,feet) ? 1.60f:2.55f;
    const float dx=(cs*forward-sn*strafe)*dt*speed;
    const float dz=(sn*forward+cs*strafe)*dt*speed;
    // Swept movement approximated by short, independent horizontal steps.
    // This stops diagonal movement from skipping the narrow ladder landing
    // and the colliders around valve stations, while retaining wall sliding.
    // walkSurface also rejects camera/head intersections below a stair.
    const float travel=std::sqrt(dx*dx+dz*dz);
    const int steps=std::max(1,std::min(8,
        static_cast<int>(std::ceil(travel/0.065f))));
    for(int step_index=0;step_index<steps;++step_index) {
        float surface=camera_.y-1.65f;
        const float step_x=dx/static_cast<float>(steps);
        const float step_z=dz/static_cast<float>(steps);
        if(world_.walkSurface(camera_.x+step_x,camera_.z,
                              camera_.y-1.65f,surface)) {
            camera_.x+=step_x;
            camera_.y=surface+1.65f;
        }
        surface=camera_.y-1.65f;
        if(world_.walkSurface(camera_.x,camera_.z+step_z,
                              camera_.y-1.65f,surface)) {
            camera_.z+=step_z;
            camera_.y=surface+1.65f;
        }
    }
    deck_=World3D::cameraDeck(camera_.y);
    climbing_=world_.onStair(camera_.x,camera_.z,camera_.y-1.65f);
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
    const StationId station=world_.lookAtStation(camera_,yaw_);
    switch(station) {
    case EcrConsoleStation:panel_=Console;break;
    case LoPumpStation:panel_=Pumps;selected_=0;break;
    case StrainerStation:sim_.inspectStrainer();panel_=Strainer;break;
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
    if(in.help) { panel_=panel_==Help?Closed:Help;return; }
    if(in.back) { panel_=Closed;return; }
    if(panel_==Pumps) {
        if(in.up) selected_=(selected_+5)%6;
        if(in.down) selected_=(selected_+1)%6;
    }
    if(in.interact) use();
    if(panel_==Closed) move(dt,in.forward,in.strafe,in.turn,in.look);
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
            s.main_pump.running?"001: MAIN LO PUMP RUNNING - WATCH PRESSURE":
            s.main_pump.requested?"001: MAIN LO PUMP STARTING":
            deck_==PumpFlatDeck?"001: USE LOCAL LO PANEL TO START MAIN PUMP":
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
    if(panel_!=Closed) prompt="D-PAD UP/DOWN SELECT   X OPERATE   O CLOSE";
    if(climbing_) prompt="INCLINED STEEL STAIRS - KEEP WALKING TO THE LANDING";
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
            "WALK UP/DOWN INCLINED STEEL STAIRS",
            "D-PAD UP/DOWN: WALK OR SELECT PANEL",
            "O: CLOSE   TRIANGLE: HELP",
            "DESCEND FROM ECR TO DECK 01"
        };
        h.panel_count=7;
        for(int i=0;i<h.panel_count;++i)
            std::snprintf(h.panel_rows[i],sizeof(h.panel_rows[i]),"%s",lines[i]);
    } else if(panel_==Pumps) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"DECK 01 | LOCAL LO PUMPS / VALVES");
        const char* names[]={"MAIN PUMP","STANDBY PUMP","MAIN SUCTION","MAIN DISCHARGE",
                             "STBY SUCTION","STBY DISCHARGE"};
        const char* values[]={
            pumpText(s.main_pump),pumpText(s.standby_pump),
            onOff(s.main_pump.suction_open),onOff(s.main_pump.discharge_open),
            onOff(s.standby_pump.suction_open),onOff(s.standby_pump.discharge_open)
        };
        h.panel_count=7;
        h.selected_row=selected_;
        for(int i=0;i<6;++i)
            std::snprintf(h.panel_rows[i],sizeof(h.panel_rows[i]),"%c %-19s %s",
                          selected_==i?'>':' ',names[i],values[i]);
        std::snprintf(h.panel_rows[6],sizeof(h.panel_rows[6]),
                      "LO HEADER %.2f BAR | BUS %s",
                      s.lo_pressure_bar,s.electrical_bus_energized?"LIVE":"OFF");
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
