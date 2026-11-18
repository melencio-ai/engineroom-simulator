#include "scene/Game3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace enginewatch {
namespace {
float clamp(float v, float low, float high) {
    return std::max(low, std::min(high, v));
}
const char* pumpText(const PumpState& p) {
    return p.fault ? "FAULT" : p.running ? "RUN" : p.requested ? "STARTING" : "STOP";
}
const char* onOff(bool b) { return b ? "OPEN" : "SHUT"; }
}

Game3D::Game3D(const World3D& world) : world_(world) { reset(); }

void Game3D::reset() {
    sim_.reset();
    camera_ = Vec3{-7.35f,1.65f,0.0f};
    yaw_ = 3.14159265358979323846f;
    panel_ = Closed;
    selected_ = 0;
    paused_ = false;
}

void Game3D::move(float dt, float forward, float strafe, float turn) {
    yaw_ += clamp(turn,-1.0f,1.0f) * dt * 2.15f;
    if(yaw_ > 6.28318530718f) yaw_ -= 6.28318530718f;
    if(yaw_ < -6.28318530718f) yaw_ += 6.28318530718f;
    forward=clamp(forward,-1.0f,1.0f);
    strafe=clamp(strafe,-1.0f,1.0f);
    const float magnitude=std::sqrt(forward*forward+strafe*strafe);
    if(magnitude>1.0f) { forward/=magnitude; strafe/=magnitude; }
    const float cs=std::cos(yaw_), sn=std::sin(yaw_);
    const float dx=(cs*forward-sn*strafe)*dt*3.20f;
    const float dz=(sn*forward+cs*strafe)*dt*3.20f;
    // Independent axis tests let the player slide along handrails and walls.
    if(world_.canStand(camera_.x+dx,camera_.z)) camera_.x+=dx;
    if(world_.canStand(camera_.x,camera_.z+dz)) camera_.z+=dz;
}

void Game3D::operate() {
    const MachineryState& s=sim_.state();
    switch(selected_) {
    case 0: sim_.requestPump(MainLoPump,!s.main_pump.requested); break;
    case 1: sim_.requestPump(StandbyLoPump,!s.standby_pump.requested); break;
    case 2: sim_.setValve(MainLoPump,SuctionValve,!s.main_pump.suction_open); break;
    case 3: sim_.setValve(MainLoPump,DischargeValve,!s.main_pump.discharge_open); break;
    case 4: sim_.setValve(StandbyLoPump,SuctionValve,!s.standby_pump.suction_open); break;
    case 5: sim_.setValve(StandbyLoPump,DischargeValve,!s.standby_pump.discharge_open); break;
    }
}

void Game3D::use() {
    if(panel_==Help || panel_==Strainer) { panel_=Closed; return; }
    if(panel_==Pumps) { operate(); return; }
    if(panel_==Console) {
        const MachineryState& s=sim_.state();
        if(s.mission==1) {
            if(s.objective_complete) { sim_.beginMissionTwo(); panel_=Closed; }
            else sim_.inspectEcrConsole();
        } else if(s.lo_pressure_alarm && !s.alarm_acknowledged) {
            sim_.acknowledgeAlarm();
        } else {
            sim_.verifyMissionTwo();
        }
        return;
    }
    switch(world_.lookAtStation(camera_,yaw_)) {
    case EcrConsoleStation: panel_=Console; break;
    case LoPumpStation: panel_=Pumps; selected_=0; break;
    case StrainerStation: sim_.inspectStrainer(); panel_=Strainer; break;
    default: break;
    }
}

void Game3D::update(float dt_seconds, const Input3D& in) {
    if(in.pause) { paused_=!paused_; return; }
    if(paused_) {
        if(in.interact) reset();
        return;
    }
    if(in.help) { panel_=panel_==Help ? Closed : Help; return; }
    if(in.back) { panel_=Closed; return; }
    if(panel_==Pumps) {
        if(in.up) selected_=(selected_+5)%6;
        if(in.down) selected_=(selected_+1)%6;
    }
    if(in.interact) use();
    if(!(dt_seconds>0.0f) || !std::isfinite(dt_seconds)) return;
    const float dt=clamp(dt_seconds,0.0f,0.05f);
    sim_.step(dt);
    if(panel_==Closed) move(dt,in.forward,in.strafe,in.turn);
}

Hud3D Game3D::hud() const {
    Hud3D h{};
    const MachineryState& s=sim_.state();
    h.paused=paused_;
    h.alarm=s.lo_pressure_alarm;
    std::snprintf(h.title,sizeof(h.title),"ENGINE WATCH 3D  |  MISSION %03d",s.mission);
    std::snprintf(h.status,sizeof(h.status),"LO %.2f BAR   MAIN %s   STBY %s",
                  s.lo_pressure_bar,pumpText(s.main_pump),pumpText(s.standby_pump));
    if(s.mission==1) {
        std::snprintf(h.objective,sizeof(h.objective),"%s",
          s.objective_complete ? "001 COMPLETE - USE ECR CONSOLE TO CONTINUE" :
          s.lo_pressure_bar>=3.0f ? "RETURN TO ECR AND VERIFY LO PRESSURE" :
          "START MAIN LO PUMP AT LOCAL STATION");
    } else {
        std::snprintf(h.objective,sizeof(h.objective),"%s",
          s.mission_two_complete ? "002 COMPLETE - HEADER RESTORED" :
          s.main_strainer_dp_bar>1.0f ? "HIGH STRAINER DP - TRANSFER TO STANDBY" :
          "INVESTIGATE FALLING LO HEADER PRESSURE");
    }
    const StationId station=world_.lookAtStation(camera_,yaw_);
    const char* prompt="STICK WALK   D-PAD L/R TURN   X INTERACT";
    if(station==EcrConsoleStation) prompt="X OPEN ECR CONSOLE";
    if(station==LoPumpStation) prompt="X OPERATE LOCAL LO STATION";
    if(station==StrainerStation) prompt="X INSPECT LO SUCTION STRAINER";
    if(panel_!=Closed) prompt="D-PAD UP/DOWN SELECT   X OPERATE   O CLOSE";
    std::snprintf(h.prompt,sizeof(h.prompt),"%s",prompt);
    h.panel_visible=panel_!=Closed || paused_;
    if(paused_) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"SIMULATION PAUSED");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),"START  RESUME");
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),"X      RESET BOTH MISSIONS");
        h.panel_count=2;
    } else if(panel_==Help) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"CONTROLS / MISSION GUIDE");
        const char* lines[]={
            "ANALOG STICK: WALK AND STRAFE",
            "D-PAD LEFT/RIGHT OR L/R: TURN",
            "X: INTERACT / OPERATE",
            "O: CLOSE PANEL",
            "D-PAD UP/DOWN: SELECT CONTROL",
            "TRIANGLE: HELP    START: PAUSE",
            "GO TO LO STATION, THEN VERIFY IN ECR"
        };
        h.panel_count=7;
        for(int i=0;i<h.panel_count;++i)
            std::snprintf(h.panel_rows[i],sizeof(h.panel_rows[i]),"%s",lines[i]);
    } else if(panel_==Pumps) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"LOCAL LO PUMPS AND VALVES");
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
        std::snprintf(h.panel_title,sizeof(h.panel_title),"ECR - LO SYSTEM MONITOR");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),
                      "LO HEADER: %.2f BAR  /  %.0f C",s.lo_pressure_bar,s.lo_oil_temp_c);
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),
                      "MAIN %s   STANDBY %s",pumpText(s.main_pump),pumpText(s.standby_pump));
        std::snprintf(h.panel_rows[2],sizeof(h.panel_rows[2]),
                      "MAIN STRAINER DP: %.2f BAR",s.main_strainer_dp_bar);
        std::snprintf(h.panel_rows[3],sizeof(h.panel_rows[3]),"%s",
                      s.lo_pressure_alarm ? (s.alarm_acknowledged?"LO LOW PRESSURE: ACK":"LO LOW PRESSURE: ALARM") :
                      "NO ACTIVE LO ALARM");
        if(s.mission==1) {
            std::snprintf(h.panel_rows[4],sizeof(h.panel_rows[4]),"%s",
                          s.objective_complete?"X: BEGIN MISSION 002":"X: VERIFY PRESSURE");
        } else {
            std::snprintf(h.panel_rows[4],sizeof(h.panel_rows[4]),"%s",
                          s.lo_pressure_alarm&&!s.alarm_acknowledged ? "X: ACKNOWLEDGE ALARM" :
                          s.mission_two_complete ? "MISSION 002 VERIFIED" : "X: VERIFY RESTORATION");
        }
        h.panel_count=5;
    } else if(panel_==Strainer) {
        std::snprintf(h.panel_title,sizeof(h.panel_title),"MAIN LO SUCTION STRAINER");
        std::snprintf(h.panel_rows[0],sizeof(h.panel_rows[0]),
                      "DIFFERENTIAL: %.2f BAR",s.main_strainer_dp_bar);
        std::snprintf(h.panel_rows[1],sizeof(h.panel_rows[1]),
                      "MAIN INLET: %.2f BAR",s.main_pump.suction_pressure_bar);
        std::snprintf(h.panel_rows[2],sizeof(h.panel_rows[2]),"%s",
                      s.main_strainer_dp_bar>1.0f?"HIGH DP - SUCTION RESTRICTION":"DP BELOW ALARM LIMIT");
        std::snprintf(h.panel_rows[3],sizeof(h.panel_rows[3]),"%s",
                      s.strainer_inspected?"INSPECTION RECORDED":"MONITOR THE SUCTION PATH");
        h.panel_count=4;
    }
    return h;
}

} // namespace enginewatch
