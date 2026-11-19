#include "scene/PlayerCharacter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace enginewatch {
namespace {
const float PI=3.14159265358979323846f;
const float TWOPI=6.2831853071795864769f;

float clampf(float v,float lo,float hi) {
    return std::max(lo,std::min(hi,v));
}
Vec3 plus(const Vec3& a,const Vec3& b) {
    return Vec3{a.x+b.x,a.y+b.y,a.z+b.z};
}
Vec3 minus(const Vec3& a,const Vec3& b) {
    return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};
}
Vec3 rotateXY(const Vec3& p,float a) {
    const float c=std::cos(a),s=std::sin(a);
    return Vec3{p.x*c-p.y*s,p.x*s+p.y*c,p.z};
}
Vec3 hinge(const Vec3& point,const Vec3& pivot,float angle) {
    return plus(pivot,rotateXY(minus(point,pivot),angle));
}
uint32_t rgb(unsigned r,unsigned g,unsigned b) {
    return 0xff000000u|(b<<16)|(g<<8)|r;
}
uint32_t shade(uint32_t abgr,float factor) {
    const unsigned r=static_cast<unsigned>(clampf((abgr&255)*factor,0,255));
    const unsigned g=static_cast<unsigned>(clampf(((abgr>>8)&255)*factor,0,255));
    const unsigned b=static_cast<unsigned>(clampf(((abgr>>16)&255)*factor,0,255));
    return rgb(r,g,b);
}
const uint32_t SUIT=0xff60392au;       // Navy industrial cotton suit (ABGR)
const uint32_t SUIT_SHADOW=0xff4b2b21u;
const uint32_t BOOTS=0xff29282cu;
const uint32_t GLOVE=0xff5e6661u;
const uint32_t HELMET=0xff54add5u;     // Warm yellow hard hat
const uint32_t REFLECT=0xffc8dad9u;
const uint32_t SKIN=0xff9dbdc9u;
const uint32_t BADGE=0xfff4e6c3u;

struct MeshBuilder {
    std::vector<Vertex3D>& verts;
    Vec3 origin;
    Vec3 forward;
    Vec3 up;
    Vec3 right;

    Vec3 toWorld(const Vec3& p) const {
        return Vec3{
            origin.x+forward.x*p.x+up.x*p.y+right.x*p.z,
            origin.y+forward.y*p.x+up.y*p.y+right.y*p.z,
            origin.z+forward.z*p.x+up.z*p.y+right.z*p.z
        };
    }
    void vertex(const Vec3& p,uint32_t color) {
        const Vec3 w=toWorld(p);
        verts.push_back(Vertex3D{color,w.x,w.y,w.z});
    }
    void face(const Vec3& a,const Vec3& b,const Vec3& c,
              const Vec3& d,uint32_t color) {
        vertex(a,color);vertex(b,color);vertex(c,color);
        vertex(a,color);vertex(c,color);vertex(d,color);
    }
    void box(const Vec3& center,const Vec3& half,float angle,
             uint32_t color) {
        const float x=half.x,y=half.y,z=half.z;
        Vec3 p[8]={
            Vec3{-x,-y,-z}, Vec3{x,-y,-z},
            Vec3{x,y,-z}, Vec3{-x,y,-z},
            Vec3{-x,-y,z}, Vec3{x,-y,z},
            Vec3{x,y,z}, Vec3{-x,y,z}
        };
        for(int i=0;i<8;++i)
            p[i]=plus(center,rotateXY(p[i],angle));
        face(p[0],p[1],p[2],p[3],shade(color,0.80f));
        face(p[5],p[4],p[7],p[6],shade(color,1.03f));
        face(p[4],p[0],p[3],p[7],shade(color,0.70f));
        face(p[1],p[5],p[6],p[2],shade(color,1.17f));
        face(p[3],p[2],p[6],p[7],shade(color,1.22f));
        face(p[4],p[5],p[1],p[0],shade(color,0.60f));
    }
    void hingedBox(const Vec3& center,const Vec3& half,
                   const Vec3& joint,float radians,uint32_t color) {
        box(hinge(center,joint,radians),half,radians,color);
    }
};
MeshBuilder bodyBuilder(std::vector<Vertex3D>& out,const Vec3& feet,float yaw) {
    const float cs=std::cos(yaw),sn=std::sin(yaw);
    return MeshBuilder{out,feet,Vec3{cs,0,sn},Vec3{0,1,0},Vec3{-sn,0,cs}};
}
MeshBuilder handBuilder(std::vector<Vertex3D>& out,const Vec3& eye,
                        float yaw,float pitch) {
    const float cs=std::cos(yaw),sn=std::sin(yaw);
    const float cp=std::cos(pitch),sp=std::sin(pitch);
    return MeshBuilder{out,eye,
                       Vec3{cs*cp,sp,sn*cp},
                       Vec3{-cs*sp,cp,-sn*sp},
                       Vec3{-sn,0,cs}};
}
float reach(const CharacterPose& pose) {
    if(pose.action==CharacterIdle) return 0;
    return std::sin(PI*clampf(pose.action_progress,0.0f,1.0f));
}
}

const float PlayerCharacter::EyeHeight=1.65f;
const float PlayerCharacter::BodyHeight=1.82f;

void CharacterAnimator::reset() {
    pose_=CharacterPose{};
    action_elapsed_=0;
}

void CharacterAnimator::advance(float dt,float distance,bool climbing) {
    dt=clampf(dt,0.0f,0.05f);
    if(!std::isfinite(distance) || distance<0.0f) distance=0.0f;
    pose_.clock_seconds+=dt;
    if(pose_.clock_seconds>3600.0f) pose_.clock_seconds-=3600.0f;
    pose_.climbing=climbing;
    // Use ACTUAL travelled metres instead of input axes. Blocked movement
    // must not animate the engineer walking through a bulkhead.
    const float target=clampf(distance/(std::max(dt,0.001f)*2.55f),0,1);
    pose_.motion+=clampf(dt*12.0f,0,1)*(target-pose_.motion);
    if(distance>0.0001f)
        pose_.walk_phase=std::fmod(pose_.walk_phase+
            distance*TWOPI/(climbing?0.88f:1.35f),TWOPI);
    if(pose_.action!=CharacterIdle) {
        action_elapsed_+=dt;
        const float duration=pose_.action==CharacterPressControl?0.68f:1.15f;
        pose_.action_progress=clampf(action_elapsed_/duration,0,1);
        if(action_elapsed_>=duration) {
            pose_.action=CharacterIdle;
            pose_.action_progress=0;
            action_elapsed_=0;
        }
    }
}

void CharacterAnimator::trigger(CharacterAction action) {
    if(action==CharacterIdle) return;
    pose_.action=action;
    pose_.action_progress=0.0f;
    action_elapsed_=0;
}

void PlayerCharacter::buildBody(const Vec3& feet,float yaw,
                               const CharacterPose& pose,
                               std::vector<Vertex3D>& out) {
    out.clear();
    out.reserve(1152);
    MeshBuilder b=bodyBuilder(out,feet,yaw);
    const float breathing=0.007f*std::sin(pose.clock_seconds*2.10f);
    const float gait=clampf(pose.motion,0,1);
    const float cycle=std::sin(pose.walk_phase);
    const float reach_amount=reach(pose);

    // Boiler suit, collar, belt, helmet, name badge, radio and face.
    b.box(Vec3{0,1.190f+breathing,0},Vec3{0.17f,0.29f,0.215f},0,SUIT);
    b.box(Vec3{0,0.887f,0},Vec3{0.165f,0.130f,0.208f},0,SUIT_SHADOW);
    b.box(Vec3{0.0f,0.916f,0},Vec3{0.177f,0.027f,0.216f},0,BOOTS);
    b.box(Vec3{0,1.492f,0},Vec3{0.080f,0.065f,0.095f},0,SUIT_SHADOW);
    b.box(Vec3{0.018f,1.646f,0},Vec3{0.112f,0.132f,0.119f},0,SKIN);
    b.box(Vec3{0.012f,1.778f,0},Vec3{0.150f,0.046f,0.150f},0,HELMET);
    b.box(Vec3{0.090f,1.739f,0},Vec3{0.115f,0.018f,0.170f},0,HELMET);
    b.box(Vec3{0.171f,1.390f,-0.125f},Vec3{0.009f,0.050f,0.060f},0,BADGE);
    b.box(Vec3{-0.020f,1.410f,0.240f},Vec3{0.060f,0.080f,0.029f},0,BOOTS);
    b.box(Vec3{0.180f,1.078f,0},Vec3{0.007f,0.021f,0.192f},0,REFLECT);

    for(int side=-1;side<=1;side+=2) {
        const float z=side*0.123f;
        const float leg_swing=(side<0?1.0f:-1.0f)*0.49f*gait*cycle;
        const float knee_bend=(side<0?1.0f:-1.0f)*
                              0.12f*gait*std::max(0.0f,-cycle);
        const Vec3 hip{0.0f,0.837f,z};
        b.hingedBox(Vec3{0,0.653f,z},Vec3{0.124f,0.204f,0.104f},
                    hip,leg_swing,SUIT);
        const Vec3 knee=hinge(Vec3{0,0.464f,z},hip,leg_swing);
        const float lower_angle=leg_swing+knee_bend;
        b.box(plus(knee,rotateXY(Vec3{0,-0.171f,0},lower_angle)),
              Vec3{0.106f,0.188f,0.093f},lower_angle,SUIT_SHADOW);
        b.box(plus(knee,rotateXY(Vec3{0,-0.275f,0},lower_angle)),
              Vec3{0.112f,0.024f,0.100f},lower_angle,REFLECT);
        b.box(plus(knee,rotateXY(Vec3{0.080f,-0.375f,0},lower_angle)),
              Vec3{0.173f,0.075f,0.111f},lower_angle,BOOTS);

        const float shoulder_z=side*0.281f;
        const Vec3 shoulder{0,1.387f,shoulder_z};
        float arm_swing=-leg_swing*0.79f;
        if(pose.climbing)
            arm_swing=0.66f+0.23f*(side<0?cycle:-cycle);
        if(side>0 && pose.action!=CharacterIdle)
            arm_swing+=(pose.action==CharacterPressControl?
                         1.07f:0.73f)*reach_amount;
        b.hingedBox(Vec3{0,1.222f,shoulder_z},
                    Vec3{0.106f,0.186f,0.091f},
                    shoulder,arm_swing,SUIT);
        const Vec3 elbow=hinge(Vec3{0,1.055f,shoulder_z},
                               shoulder,arm_swing);
        const float forearm=arm_swing+(side>0?0.16f*reach_amount:0.0f);
        b.box(plus(elbow,rotateXY(Vec3{0,-0.150f,0},forearm)),
              Vec3{0.094f,0.167f,0.081f},forearm,SUIT_SHADOW);
        b.box(plus(elbow,rotateXY(Vec3{0,-0.200f,0},forearm)),
              Vec3{0.099f,0.020f,0.085f},forearm,REFLECT);
        b.box(plus(elbow,rotateXY(Vec3{0,-0.320f,0},forearm)),
              Vec3{0.078f,0.073f,0.082f},forearm,GLOVE);
    }
}

void PlayerCharacter::buildFirstPersonHands(const Vec3& eye,float yaw,float pitch,
                                            const CharacterPose& pose,
                                            std::vector<Vertex3D>& out) {
    out.clear();
    out.reserve(450);
    MeshBuilder b=handBuilder(out,eye,yaw,pitch);
    const float gait=clampf(pose.motion,0,1);
    const float w=std::sin(pose.walk_phase);
    const float reach_amount=reach(pose);
    for(int side=-1;side<=1;side+=2) {
        const float sway=0.022f*gait*(side<0?w:-w);
        const float climb=pose.climbing?
                          0.16f+0.055f*(side<0?w:-w):0.0f;
        const float reach_x=(side>0?0.24f*reach_amount:0);
        const float reach_y=(side>0?0.27f*reach_amount:0);
        const float reach_z=(side>0?-0.27f*reach_amount:0);
        const float zz=side*0.39f+reach_z;
        const float yy=-0.45f+climb+reach_y+sway;
        // Short, camera-space sleeves and working gloves. First-person
        // geometry starts beyond the 0.25-m near plane and follows pitch,
        // but remains depth-tested against machinery/console surfaces.
        b.box(Vec3{0.52f+reach_x*0.45f,yy-0.055f,zz},
              Vec3{0.225f,0.090f,0.098f},0,SUIT);
        b.box(Vec3{0.675f+reach_x*0.73f,yy-0.029f,zz},
              Vec3{0.041f,0.091f,0.101f},0,REFLECT);
        b.box(Vec3{0.765f+reach_x,yy+0.014f,zz},
              Vec3{0.125f,0.077f,0.092f},0,GLOVE);
    }
}

} // namespace enginewatch
