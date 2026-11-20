#include "scene/PlayerCharacter.hpp"
#include "scene/ImportedHumanoid.hpp"

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
    void taperedTorso(const Vec3& center,float halfHeight,
                      float chestHalfWidth,float waistHalfWidth,
                      float halfDepth,uint32_t color) {
        // One six-faced tapered prism, not an enormous rectangular cuboid.
        // Its shoulders naturally extend beyond the waist in third person.
        const float y0=center.y-halfHeight,y1=center.y+halfHeight;
        const float xf=center.x+halfDepth,xb=center.x-halfDepth;
        const float z0=center.z-waistHalfWidth,z1=center.z+waistHalfWidth;
        const float z2=center.z-chestHalfWidth,z3=center.z+chestHalfWidth;
        const Vec3 p[8]={
            Vec3{xb,y0,z0},Vec3{xf,y0,z0},
            Vec3{xf,y1,z2},Vec3{xb,y1,z2},
            Vec3{xb,y0,z1},Vec3{xf,y0,z1},
            Vec3{xf,y1,z3},Vec3{xb,y1,z3}
        };
        face(p[0],p[1],p[2],p[3],shade(color,.83f));
        face(p[5],p[4],p[7],p[6],shade(color,1.06f));
        face(p[4],p[0],p[3],p[7],shade(color,.73f));
        face(p[1],p[5],p[6],p[2],shade(color,1.16f));
        face(p[3],p[2],p[6],p[7],shade(color,1.21f));
        face(p[4],p[5],p[1],p[0],shade(color,.63f));
    }
    void facetedHead(const Vec3& center,const Vec3& radius,uint32_t color) {
        // Hexagonal tapered skull with two facial-height bands. Flat
        // triangle shading keeps PSP-friendly recognizable anatomy.
        const int sides=8;
        for(int band=0;band<2;++band) {
            const float lower=band==0?-0.77f:0.0f;
            const float upper=band==0?0.0f:0.78f;
            const float lower_scale=band==0?0.82f:1.0f;
            const float upper_scale=band==0?1.0f:0.80f;
            for(int k=0;k<sides;++k) {
                const float a=TWOPI*k/sides,b=TWOPI*(k+1)/sides;
                const Vec3 p0{center.x+radius.x*lower_scale*std::cos(a),
                              center.y+radius.y*lower,
                              center.z+radius.z*lower_scale*std::sin(a)};
                const Vec3 p1{center.x+radius.x*lower_scale*std::cos(b),
                              center.y+radius.y*lower,
                              center.z+radius.z*lower_scale*std::sin(b)};
                const Vec3 p2{center.x+radius.x*upper_scale*std::cos(b),
                              center.y+radius.y*upper,
                              center.z+radius.z*upper_scale*std::sin(b)};
                const Vec3 p3{center.x+radius.x*upper_scale*std::cos(a),
                              center.y+radius.y*upper,
                              center.z+radius.z*upper_scale*std::sin(a)};
                face(p0,p1,p2,p3,shade(color,k%3==0?1.05f:.94f));
            }
        }
        // Low-poly cheekbone/forehead caps; never use a full sphere mesh.
        for(int k=0;k<sides;++k) {
            const float a=TWOPI*k/sides,b=TWOPI*(k+1)/sides;
            const Vec3 top{center.x,center.y+radius.y,center.z};
            const Vec3 lo{center.x,center.y-radius.y,center.z};
            vertex(top,color);
            vertex(Vec3{center.x+radius.x*.80f*std::cos(a),
                        center.y+radius.y*.78f,
                        center.z+radius.z*.80f*std::sin(a)},color);
            vertex(Vec3{center.x+radius.x*.80f*std::cos(b),
                        center.y+radius.y*.78f,
                        center.z+radius.z*.80f*std::sin(b)},color);
            vertex(lo,shade(color,.80f));
            vertex(Vec3{center.x+radius.x*.82f*std::cos(b),
                        center.y-radius.y*.77f,
                        center.z+radius.z*.82f*std::sin(b)},shade(color,.80f));
            vertex(Vec3{center.x+radius.x*.82f*std::cos(a),
                        center.y-radius.y*.77f,
                        center.z+radius.z*.82f*std::sin(a)},shade(color,.80f));
        }
    }
    void beam(const Vec3& a,const Vec3& b,float halfA,float halfB,
              uint32_t color) {
        // Two-bone arm segment with a true 3D axis. Cheaper than skinning,
        // but it can reach a real socket that is above/below or beside
        // the character rather than moving only in the shoulder XY plane.
        Vec3 v=minus(b,a);
        const float len=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
        if(len<0.005f) return;
        v=Vec3{v.x/len,v.y/len,v.z/len};
        const Vec3 reference=std::fabs(v.y)<0.86f?
                             Vec3{0,1,0}:Vec3{0,0,1};
        Vec3 side=Vec3{v.y*reference.z-v.z*reference.y,
                       v.z*reference.x-v.x*reference.z,
                       v.x*reference.y-v.y*reference.x};
        const float sl=std::sqrt(side.x*side.x+side.y*side.y+side.z*side.z);
        side=Vec3{side.x/sl,side.y/sl,side.z/sl};
        const Vec3 other{v.y*side.z-v.z*side.y,
                         v.z*side.x-v.x*side.z,
                         v.x*side.y-v.y*side.x};
        const auto pt=[&](const Vec3& center,float sa,float sb) -> Vec3 {
            return Vec3{center.x+side.x*halfA*sa+other.x*halfB*sb,
                        center.y+side.y*halfA*sa+other.y*halfB*sb,
                        center.z+side.z*halfA*sa+other.z*halfB*sb};
        };
        const Vec3 p[8]={
            pt(a,-1,-1),pt(a,1,-1),pt(a,1,1),pt(a,-1,1),
            pt(b,-1,-1),pt(b,1,-1),pt(b,1,1),pt(b,-1,1)
        };
        face(p[0],p[4],p[5],p[1],shade(color,.83f));
        face(p[1],p[5],p[6],p[2],shade(color,1.16f));
        face(p[2],p[6],p[7],p[3],shade(color,1.04f));
        face(p[3],p[7],p[4],p[0],shade(color,.75f));
        face(p[0],p[1],p[2],p[3],shade(color,.67f));
        face(p[5],p[4],p[7],p[6],shade(color,1.20f));
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

bool PlayerCharacter::loadImportedBody(const char* path) {
    return ImportedHumanoid::load(path);
}
void PlayerCharacter::clearImportedBody() { ImportedHumanoid::clear(); }
bool PlayerCharacter::hasImportedBody() { return ImportedHumanoid::available(); }


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
            pose_.has_target=false;
            action_elapsed_=0;
        }
    }
}

void CharacterAnimator::trigger(CharacterAction action,const Vec3* target) {
    if(action==CharacterIdle) return;
    pose_.action=action;
    pose_.action_progress=0.0f;
    pose_.has_target=target!=nullptr;
    if(target) pose_.target_world=*target;
    action_elapsed_=0;
}

void PlayerCharacter::buildBody(const Vec3& feet,float yaw,
                               const CharacterPose& pose,
                               std::vector<Vertex3D>& out) {
    // The shipped Quaternius worker is used for 3P idle/walk and its own
    // Interact clip. For an accurately aimed *physical* G3.6 pushbutton,
    // retain the existing two-link right-arm socket IK while retargeting of
    // the imported skeleton remains a separate engineering milestone.
    if(!(pose.action==CharacterPressControl && pose.has_target) &&
       ImportedHumanoid::sample(feet,yaw,pose,out))
        return;
    out.clear();
    out.reserve(1152);
    MeshBuilder b=bodyBuilder(out,feet,yaw);
    const float breathing=0.007f*std::sin(pose.clock_seconds*2.10f);
    const float gait=clampf(pose.motion,0,1);
    const float cycle=std::sin(pose.walk_phase);
    const float reach_amount=reach(pose);

    // Boiler suit, collar, belt, helmet, name badge, radio and face.
    b.taperedTorso(Vec3{0,1.190f+breathing,0},
                   0.291f,0.228f,0.178f,0.158f,SUIT);
    b.box(Vec3{0,0.887f,0},Vec3{0.165f,0.130f,0.208f},0,SUIT_SHADOW);
    b.box(Vec3{0.0f,0.916f,0},Vec3{0.177f,0.027f,0.216f},0,BOOTS);
    b.box(Vec3{0,1.492f,0},Vec3{0.080f,0.065f,0.095f},0,SUIT_SHADOW);
    b.facetedHead(Vec3{0.020f,1.641f,0},
                  Vec3{0.105f,0.133f,0.112f},SKIN);
    // Compact helmet bowl and forward brim follow the faceted head.
    b.facetedHead(Vec3{0.014f,1.770f,0},
                  Vec3{0.143f,0.054f,0.147f},HELMET);
    b.box(Vec3{0.105f,1.746f,0},Vec3{0.112f,0.014f,0.162f},0,HELMET);
    b.box(Vec3{0.171f,1.390f,-0.125f},Vec3{0.009f,0.050f,0.060f},0,BADGE);
    b.box(Vec3{-0.020f,1.410f,0.240f},Vec3{0.060f,0.080f,0.029f},0,BOOTS);
    b.box(Vec3{0.180f,1.078f,0},Vec3{0.007f,0.021f,0.192f},0,REFLECT);

    for(int side=-1;side<=1;side+=2) {
        const float z=side*0.123f;
        const float phase=(side<0?1.0f:-1.0f)*cycle;
        const float leg_swing=0.37f*gait*phase;
        const float knee_bend=0.22f*gait*std::max(0.0f,phase);
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
        if(side>0 && pose.has_target &&
           pose.action==CharacterPressControl) {
            // Analytic two-link right arm. World socket -> model local
            // coordinates, then elbow triangle solution (0.34m + 0.36m).
            // At peak extension the glove fingertip reaches the socket
            // without adding high-poly IK bones or altering the PSP renderer.
            const float cs=std::cos(yaw),sn=std::sin(yaw);
            const float wx=pose.target_world.x-feet.x;
            const float wz=pose.target_world.z-feet.z;
            const Vec3 goal{
                wx*cs+wz*sn,
                pose.target_world.y-feet.y,
                -wx*sn+wz*cs
            };
            const Vec3 still{0.095f,0.93f,shoulder_z};
            const float strength=reach_amount;
            const Vec3 fingertip{
                still.x+(goal.x-still.x)*strength,
                still.y+(goal.y-still.y)*strength,
                still.z+(goal.z-still.z)*strength
            };
            Vec3 dir=minus(fingertip,shoulder);
            const float reachDist=std::sqrt(
                dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
            const float distance=clampf(reachDist,0.16f,0.69f);
            if(reachDist>0.0001f)
                dir=Vec3{dir.x/reachDist,dir.y/reachDist,dir.z/reachDist};
            else dir=Vec3{1,0,0};
            // Move the palm back about 7cm from the button face; the short
            // end glove section finishes at the exact contact location.
            const Vec3 palm{
                shoulder.x+dir.x*(distance-0.055f),
                shoulder.y+dir.y*(distance-0.055f),
                shoulder.z+dir.z*(distance-0.055f)
            };
            const float upper=0.355f,lower=0.355f;
            const float axial=(distance*distance+upper*upper-lower*lower)/
                               (2.0f*distance);
            const float bend=std::sqrt(std::max(0.0f,upper*upper-axial*axial));
            // Downward elbow preference, projected perpendicular to reach.
            Vec3 bendDir{
                -dir.x*dir.y,1.0f-dir.y*dir.y,-dir.z*dir.y
            };
            const float bendLength=std::sqrt(bendDir.x*bendDir.x+
                bendDir.y*bendDir.y+bendDir.z*bendDir.z);
            if(bendLength<0.05f)bendDir=Vec3{-dir.z,0,dir.x};
            else bendDir=Vec3{bendDir.x/bendLength,
                              bendDir.y/bendLength,bendDir.z/bendLength};
            const Vec3 elbow{
                shoulder.x+dir.x*axial-bendDir.x*bend,
                shoulder.y+dir.y*axial-bendDir.y*bend,
                shoulder.z+dir.z*axial-bendDir.z*bend
            };
            b.beam(shoulder,elbow,0.098f,0.088f,SUIT);
            b.beam(elbow,palm,0.085f,0.078f,SUIT_SHADOW);
            b.beam(palm,fingertip,0.065f,0.057f,GLOVE);
            continue;
        }
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
        const Vec3 wrist=plus(elbow,rotateXY(Vec3{0,-0.316f,0},forearm));
        b.box(wrist,Vec3{0.072f,0.068f,0.075f},forearm,GLOVE);
        // Visible glove/thumb silhouette from profile and rear views.
        b.box(plus(wrist,rotateXY(Vec3{0.069f,-0.033f,
              -side*0.046f},forearm)),
              Vec3{0.046f,0.038f,0.040f},forearm,GLOVE);
        b.box(plus(wrist,rotateXY(Vec3{0,-0.070f,0},forearm)),
              Vec3{0.055f,0.040f,0.067f},forearm,GLOVE);
    }
}

void PlayerCharacter::buildFirstPersonHands(const Vec3& eye,float yaw,float pitch,
                                            const CharacterPose& pose,
                                            std::vector<Vertex3D>& out) {
    out.clear();
    out.reserve(690);
    MeshBuilder b=handBuilder(out,eye,yaw,pitch);
    const float gait=clampf(pose.motion,0,1);
    const float w=std::sin(pose.walk_phase);
    const float reach_amount=reach(pose);
    for(int side=-1;side<=1;side+=2) {
        // Hands rest inside the lower corners ABOVE the PSP 226px HUD.
        // No forearm begins at the camera near plane (0.25m).
        const float sidef=static_cast<float>(side);
        const float sway=0.015f*gait*(side<0?w:-w);
        const float climb=pose.climbing?
                          0.075f+0.070f*(side<0?w:-w):0.0f;
        const float active=(side>0?reach_amount:0.0f);
        float xx=0.03f*std::fabs(sway)+active*0.25f;
        float yy=-0.265f+climb+active*0.155f+sway;
        float zz=sidef*(0.305f-0.095f*active);
        if(side>0 && pose.has_target &&
           pose.action==CharacterPressControl) {
            // Project the same contact socket into the first-person
            // camera basis. Our hand/palm/individual fingers translate
            // together, so the fingertip coincides at peak reach.
            const float cs=std::cos(yaw),sn=std::sin(yaw);
            const float cp=std::cos(pitch),sp=std::sin(pitch);
            const Vec3 v=minus(pose.target_world,eye);
            const float tx=(v.x*cs+v.z*sn)*cp+v.y*sp;
            const float ty=v.y*cp-(v.x*cs+v.z*sn)*sp;
            const float tz=-v.x*sn+v.z*cs;
            if(tx>0.34f && tx<1.80f) {
                // Center fingertip ends near x=1.07f and palm y~yy.
                // Limit translation to the safe PSP near-plane region.
                xx+=(tx-(1.075f+xx))*active;
                yy+=(ty-(yy+0.006f))*active;
                zz+=(tz-zz)*active;
            }
        }

        // Narrow rolled sleeves are diagonally offset and smaller than the
        // original oversized rectangular blocks; reflective wrist cuff is
        // slim enough to be visible without obscuring the glove.
        b.box(Vec3{0.673f+xx*.33f,yy-0.062f,zz+sidef*.040f},
              Vec3{0.188f,0.066f,0.077f},-.09f,SUIT);
        b.box(Vec3{0.803f+xx*.67f,yy-0.026f,zz+sidef*.014f},
              Vec3{0.028f,0.069f,0.071f},0,REFLECT);
        b.box(Vec3{0.901f+xx,yy+0.006f,zz},
              Vec3{0.106f,0.057f,0.083f},-.06f,GLOVE);

        // Four distinct low-poly gloved fingers; slightly unequal lengths.
        // Finger pairs are separated so the palm is actually recognizable
        // in silhouette instead of being one large rectangular claw.
        const float finger_lengths[]={0.063f,0.079f,0.075f,0.056f};
        for(int j=0;j<4;++j) {
            const float lateral=(static_cast<float>(j)-1.5f)*0.040f;
            b.box(Vec3{1.007f+xx+finger_lengths[j]*.32f,
                       yy+0.006f-0.008f*std::fabs(lateral),
                       zz+lateral},
                  Vec3{finger_lengths[j],0.027f,0.016f},-.04f,GLOVE);
        }
        // Opposable thumb projects toward center rather than parallel to
        // the fingers. A slight angle is enough for a convincing PSP shape.
        b.box(Vec3{0.944f+xx,yy-0.031f,zz-sidef*.104f},
              Vec3{0.072f,0.028f,0.033f},-.28f,GLOVE);
        b.box(Vec3{0.996f+xx,yy-0.048f,zz-sidef*.117f},
              Vec3{0.035f,0.026f,0.031f},-.42f,shade(GLOVE,1.09f));
    }
}

} // namespace enginewatch
