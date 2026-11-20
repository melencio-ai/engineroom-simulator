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
    chase_initialized_=false;
    chase_distance_=0.0f;
    chase_constrained_=false;
    chase_shifted_=false;
    chase_candidate_=-1;
    chase_eye_=camera_;
    chase_target_=camera_;
    pending_pump_=false;
    pending_pump_selection_=-1;
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

void Game3D::operate(int selected) {
    const MachineryState& s=sim_.state();
    switch(selected) {
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
        // X may be pressed only from the front side of the actual local
        // control cabinet, within a human reach distance. A station menu
        // can be read from farther away, but controls are not telekinetic.
        if(pending_pump_ ||
           character_animator_.pose().action==CharacterPressControl) return;
        const InteractionSocketId id=World3D::pumpMenuSocket(selected_);
        const InteractionSocket* socket=world_.interactionSocket(id);
        if(!socket || !world_.canReachSocket(id,camera_,yaw_)) return;
        character_animator_.trigger(CharacterPressControl,&socket->point);
        pending_pump_=true;
        pending_pump_selection_=selected_;
        return;
    }
    if(panel_==Console) {
        // ECR electronic verification remains the existing, immediate
        // process-simulation action. When physically close, its 3D
        // monitor socket guides the same right-hand animation.
        const InteractionSocket* socket=world_.interactionSocket(EcrVerifyButton);
        if(socket && world_.canReachSocket(EcrVerifyButton,camera_,yaw_))
            character_animator_.trigger(CharacterPressControl,&socket->point);
        else
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
    case EcrConsoleStation: {
        panel_=Console;
        const InteractionSocket* socket=world_.interactionSocket(EcrVerifyButton);
        character_animator_.trigger(CharacterInspectInstrument,
            socket && world_.canReachSocket(EcrVerifyButton,camera_,yaw_)?
            &socket->point:nullptr);
        break;
    }
    case LoPumpStation: {
        panel_=Pumps;selected_=0;
        const InteractionSocket* gauge=world_.interactionSocket(LoMainPressureGauge);
        character_animator_.trigger(CharacterInspectInstrument,
            gauge && world_.canReachSocket(LoMainPressureGauge,camera_,yaw_)?
            &gauge->point:nullptr);
        break;
    }
    case StrainerStation: {
        sim_.inspectStrainer();panel_=Strainer;
        const InteractionSocket* handle=world_.interactionSocket(StrainerInspectionHandle);
        character_animator_.trigger(CharacterInspectInstrument,
            handle && world_.canReachSocket(StrainerInspectionHandle,camera_,yaw_)?
            &handle->point:nullptr);
        break;
    }
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
    if(in.toggle_view) {
        third_person_=!third_person_;
        if(third_person_) chase_initialized_=false;
    }
    sim_.step(dt);
    if(in.help) {
        panel_=panel_==Help?Closed:Help;
        pending_pump_=false;
        pending_pump_selection_=-1;
        updateChaseCamera(dt);
        return;
    }
    if(in.back) {
        panel_=Closed;
        pending_pump_=false;
        pending_pump_selection_=-1;
        updateChaseCamera(dt);
        return;
    }
    if(panel_==Pumps && !pending_pump_) {
        if(in.up) selected_=(selected_+5)%6;
        if(in.down) selected_=(selected_+1)%6;
    }
    if(in.interact) use();
    travel_this_frame_=0.0f;
    if(panel_==Closed) move(dt,in.forward,in.strafe,in.turn,in.look);
    character_animator_.advance(dt,travel_this_frame_,climbing_);
    // Deliver exactly one simulation command at actual hand contact,
    // not at the instant the user presses X. Closing the panel before
    // contact cancels the operation; holding X cannot queue duplicates.
    if(pending_pump_ &&
       character_animator_.pose().action==CharacterPressControl &&
       character_animator_.pose().action_progress>=0.47f) {
        operate(pending_pump_selection_);
        pending_pump_=false;
        pending_pump_selection_=-1;
    }
    updateChaseCamera(dt);
}

void Game3D::updateChaseCamera(float dt) {
    if(!third_person_) return;
    const float cs=std::cos(yaw_),sn=std::sin(yaw_);
    const float safe_dt=clamp(dt,0.0f,0.05f);

    // The visibility ray begins just above the character's eye, safely in
    // front of the helmet and above a steel walking surface. The same ray
    // is used for all candidate camera placements, rather than extrapolating
    // the player's *floor* collision to elevated third-person viewpoints.
    const Vec3 focus{camera_.x,camera_.y+0.15f,camera_.z};
    const Vec3 aim{
        camera_.x+cs*1.25f,
        camera_.y-0.05f+std::sin(pitch_)*0.28f,
        camera_.z+sn*1.25f
    };

    struct Candidate {
        float back,side,raise,preference;
    };
    // In cramped spaces evaluate alternate left/right shoulders and higher
    // angles BEFORE accepting a helmet-close center camera. Side-only
    // options handle the ECR console directly behind the spawn point.
    // These are relative to the character's facing direction (metres).
    static const Candidate options[]={
        {3.05f, 0.32f,0.55f,0.00f}, // default trailing shoulder
        {2.80f, 1.25f,0.65f,0.38f},
        {2.80f,-1.25f,0.65f,0.38f},
        {2.25f, 1.55f,1.00f,0.54f},
        {2.25f,-1.55f,1.00f,0.54f},
        {1.50f, 0.20f,1.55f,0.65f}, // elevated behind
        {1.05f, 1.65f,1.20f,0.70f},
        {1.05f,-1.65f,1.20f,0.70f},
        {0.25f, 2.10f,0.88f,0.80f}, // lateral ECR/column escape
        {0.25f,-2.10f,0.88f,0.80f},
        {0.45f, 0.88f,1.55f,0.98f}, // last-resort upper shoulder
        {0.45f,-0.88f,1.55f,0.98f}
    };
    const int count=static_cast<int>(sizeof(options)/sizeof(options[0]));

    Vec3 positions[count];
    float distances[count];
    float scores[count];
    int best=0;
    float highest=-10000.0f;
    for(int i=0;i<count;++i) {
        const Candidate& c=options[i];
        const Vec3 want{
            focus.x-cs*c.back-sn*c.side,
            focus.y+c.raise,
            focus.z-sn*c.back+cs*c.side
        };
        const float dx=want.x-focus.x,dy=want.y-focus.y,dz=want.z-focus.z;
        const float length=std::sqrt(dx*dx+dy*dy+dz*dz);
        const float free=world_.cameraRayFraction(focus,want,0.10f);
        // End the camera sphere 8 cm before the first obstructing surface.
        const float t=free>=0.99999f?1.0f:
            std::max(0.0f,free-0.08f/std::max(length,0.001f));
        positions[i]=Vec3{focus.x+dx*t,focus.y+dy*t,focus.z+dz*t};
        distances[i]=length*t;
        // Prioritize an unobstructed body-length view. A shorter center
        // camera gets a much worse score than a clear sideways viewpoint.
        float score=distances[i]*1.65f-c.preference;
        if(t<0.99f)score-=0.45f;
        if(distances[i]<1.40f)score-=3.20f*(1.40f-distances[i]);
        // Hysteresis holds the previous shoulder while similarly suitable,
        // avoiding left/right flicker as the player steps across a railing.
        if(chase_initialized_ && i==chase_candidate_)score+=0.33f;
        scores[i]=score;
        if(score>highest) { highest=score;best=i; }
    }

    // Do not let a marginal score change flip the shoulder each frame.
    // A truly blocked current option will still lose to a clear one.
    const Vec3 target=positions[best];
    const float selected_distance=distances[best];
    chase_candidate_=best;
    chase_shifted_=best!=0;
    chase_constrained_=chase_shifted_ || selected_distance<2.35f;

    if(!chase_initialized_) {
        chase_eye_=target;
        chase_target_=aim;
        chase_distance_=selected_distance;
        chase_initialized_=true;
        return;
    }

    // Smooth the camera's *position*, not merely its scalar distance.
    // Restrict interpolated positions to an unobstructed ray every frame.
    const float a=1.0f-std::exp(-8.5f*safe_dt);
    Vec3 stepped{
        chase_eye_.x+(target.x-chase_eye_.x)*a,
        chase_eye_.y+(target.y-chase_eye_.y)*a,
        chase_eye_.z+(target.z-chase_eye_.z)*a
    };
    if(world_.cameraRayFraction(focus,stepped,0.10f)<0.999f)
        stepped=target; // snap to already verified safe shoulder
    chase_eye_=stepped;

    const float focus_dx=chase_eye_.x-focus.x;
    const float focus_dy=chase_eye_.y-focus.y;
    const float focus_dz=chase_eye_.z-focus.z;
    chase_distance_=std::sqrt(focus_dx*focus_dx+
                             focus_dy*focus_dy+focus_dz*focus_dz);

    const float b=1.0f-std::exp(-10.0f*safe_dt);
    chase_target_.x+=(aim.x-chase_target_.x)*b;
    chase_target_.y+=(aim.y-chase_target_.y)*b;
    chase_target_.z+=(aim.z-chase_target_.z)*b;

    // If the smoothed position approaches the player's helmet, immediately
    // use the best unobstructed lateral/elevated view rather than a near
    // first-person angle with a third-person body filling the frame.
    if(chase_distance_<1.1f && selected_distance>chase_distance_+0.20f) {
        chase_eye_=target;
        chase_distance_=selected_distance;
    }
}

CameraView3D Game3D::renderCamera() const {
    const float cs=std::cos(yaw_),sn=std::sin(yaw_);
    const float cp=std::cos(pitch_),sp=std::sin(pitch_);
    CameraView3D view;
    view.eye=camera_;
    view.target=Vec3{
        camera_.x+cs*cp,camera_.y+sp,camera_.z+sn*cp
    };
    view.show_body=false;
    view.fov_degrees=68.0f;
    view.constrained=false;
    if(!third_person_) return view;

    // The body remains visible even when the chase boom contracts at a
    // stairway or wall. A wider lens in close quarters prevents the helmet
    // from filling the whole screen; there is NO automatic 1P view switch.
    view.show_body=true;
    view.eye=chase_initialized_?chase_eye_:camera_;
    view.target=chase_initialized_?chase_target_:view.target;
    view.fov_degrees=clamp(76.0f+(1.8f-chase_distance_)*5.0f,76.0f,91.0f);
    view.constrained=chase_constrained_;
    view.shifted=chase_shifted_;
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
                  third_person_?(chase_distance_<1.45f?"3P CLOSE":
                                chase_shifted_?"3P SHIFT":"3P"):"1P");
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
    if(panel_==Pumps) {
        if(pending_pump_)
            prompt="REACHING BUTTON - CONTACT WILL APPLY COMMAND";
        else if(!world_.canReachSocket(World3D::pumpMenuSocket(selected_),
                                       camera_,yaw_))
            prompt="O CLOSE - STEP TO THE FRONT OF LO CONTROL PANEL";
    }
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
