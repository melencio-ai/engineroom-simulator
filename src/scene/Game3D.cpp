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
    camera_=Vec3{-7.35f,World3D::floorHeight(deck_)+PlayerCharacter::EyeHeight,0.0f};
    yaw_=0.0f; // Start facing the real upper-level ECR doorway.
    pitch_=-0.16f;
    panel_=Closed;
    selected_=0;
    paused_=false;
    climbing_=false;
    third_person_=false;
    travel_this_frame_=0.0f;
    character_animator_.reset();
}

void Game3D::move(float dt,float forward,float strafe,float turn,float look) {
    const float initial_x=camera_.x,initial_z=camera_.z;
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
    climbing_=world_.onStair(camera_.x,camera_.z,
                            camera_.y-PlayerCharacter::EyeHeight);
    const float xmove=camera_.x-initial_x,zmove=camera_.z-initial_z;
    travel_this_frame_=std::sqrt(xmove*xmove+zmove*zmove);
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
    if(panel_==Pumps) {
        character_animator_.trigger(CharacterPressControl);
        operate();
        return;
    }
    if(panel_==Console) {
        character_animator_.trigger(CharacterPressControl);
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
    case EcrConsoleStation:
        panel_=Console;
        character_animator_.trigger(CharacterInspectInstrument);
        break;
    case LoPumpStation:
        panel_=Pumps;selected_=0;
        character_animator_.trigger(CharacterInspectInstrument);
        break;
    case StrainerStation:
        sim_.inspectStrainer();panel_=Strainer;
        character_animator_.trigger(CharacterInspectInstrument);
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
    if(in.toggle_view) third_person_=!third_person_;
    sim_.step(dt);
    if(in.help) { panel_=panel_==Help?Closed:Help;return; }
    if(in.back) { panel_=Closed;return; }
    if(panel_==Pumps) {
        if(in.up) selected_=(selected_+5)%6;
        if(in.down) selected_=(selected_+1)%6;
    }
    if(in.interact) use();
    travel_this_frame_=0.0f;
    if(panel_==Closed) move(dt,in.forward,in.strafe,in.turn,in.look);
    character_animator_.advance(dt,travel_this_frame_,climbing_);
}

CameraView3D Game3D::renderCamera() const {
    const float cs=std::cos(yaw_),sn=std::sin(yaw_);
    const float cp=std::cos(pitch_),sp=std::sin(pitch_);
    const Vec3 first_target{
        camera_.x+cs*cp,camera_.y+sp,camera_.z+sn*cp
    };
    // C++11: CameraView3D has default member initializers, so assign
    // explicitly instead of relying on C++14 aggregate initialization.
    CameraView3D view;
    view.eye=camera_;
    view.target=first_target;
    view.show_body=false;
    if(!third_person_) return view;

    // Small shoulder offset and trailing camera behind the engineer. We
    // sample the physical deck's navigation/collision proxy along the chase
    // ray; do not allow the camera to pass through known machinery blocks,
    // platform edges or stair shafts. When there is insufficient room to
    // see the full character, fall back to safe first-person hands.
    const Vec3 desired{
        camera_.x-cs*2.35f-sn*0.16f,
        camera_.y+0.38f,
        camera_.z-sn*2.35f+cs*0.16f
    };
    const float feet=camera_.y-PlayerCharacter::EyeHeight;
    float best=0.0f;
    const int samples=24;
    for(int i=1;i<=samples;++i) {
        const float t=static_cast<float>(i)/samples;
        const float x=camera_.x+(desired.x-camera_.x)*t;
        const float z=camera_.z+(desired.z-camera_.z)*t;
        float supported=feet;
        if(!world_.walkSurface(x,z,feet,supported,0.08f) ||
           std::fabs(supported-feet)>0.47f ||
           !world_.hasHeadroom(x,z,supported,1.79f))
            break;
        best=t;
    }
    if(best<0.36f) return view;
    best=std::max(0.36f,best-0.04f);
    view.eye=Vec3{
        camera_.x+(desired.x-camera_.x)*best,
        camera_.y+(desired.y-camera_.y)*best,
        camera_.z+(desired.z-camera_.z)*best
    };
    view.target=Vec3{
        camera_.x+cs*0.63f,
        camera_.y-0.12f+sp*0.58f,
        camera_.z+sn*0.63f
    };
    view.show_body=true;
    return view;
}

void Game3D::buildCharacterGeometry(const CameraView3D& view,
                                    std::vector<Vertex3D>& out) const {
    if(view.show_body) {
        const Vec3 feet{camera_.x,camera_.y-PlayerCharacter::EyeHeight,
                        camera_.z};
        PlayerCharacter::buildBody(feet,yaw_,character_animator_.pose(),out);
    } else {
        PlayerCharacter::buildFirstPersonHands(camera_,yaw_,pitch_,
                                               character_animator_.pose(),out);
    }
}

Hud3D Game3D::hud() const {
    Hud3D h{};
    const MachineryState& s=sim_.state();
    h.paused=paused_;
    h.alarm=s.lo_pressure_alarm;
    h.climbing=climbing_;
    std::snprintf(h.title,sizeof(h.title),"ENGINE WATCH 3D - %s",World3D::deckName(deck_));
    std::snprintf(h.status,sizeof(h.status),"LO %.2f BAR  MAIN %s  STBY %s  VIEW %s",
                  s.lo_pressure_bar,pumpText(s.main_pump),pumpText(s.standby_pump),
                  third_person_?"3P":"1P");
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
    const char* prompt="STICK WALK  D-PAD TURN  L/R LOOK  X USE  SELECT VIEW";
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
            "O: CLOSE  TRIANGLE: HELP  SELECT: CAMERA",
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
