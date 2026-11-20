#include "scene/ImportedHumanoid.hpp"
#include "scene/PlayerCharacter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace enginewatch {
namespace {
struct Cache {
    unsigned per_frame=0;
    unsigned idle=0,walk=0,interact=0;
    float height=0,rate=0;
    std::vector<Vertex3D> frames;
};
Cache& cache() {
    static Cache data;
    return data;
}
bool finite(float v) { return std::isfinite(v); }
float unit(float v) { return std::max(0.0f,std::min(1.0f,v)); }
unsigned readUInt(FILE* f,bool& ok) {
    unsigned char b[4];
    if(std::fread(b,1,4,f)!=4){ok=false;return 0;}
    return (unsigned)b[0]|((unsigned)b[1]<<8)|
           ((unsigned)b[2]<<16)|((unsigned)b[3]<<24);
}
float floatBits(unsigned u) {
    float v=0;
    static_assert(sizeof(float)==4,"PSP needs IEEE754 32-bit float");
    std::memcpy(&v,&u,4);
    return v;
}
float phaseWrap(float v) {
    return v-std::floor(v);
}
void interpolate(const Cache& c,unsigned clip_start,unsigned count,
                 float time,unsigned vertex,Vec3& out) {
    const float frame=phaseWrap(time)*count;
    const unsigned i=static_cast<unsigned>(frame)%count;
    const unsigned next=(i+1)%count;
    const float mix=frame-std::floor(frame);
    const Vertex3D& a=c.frames[(clip_start+i)*c.per_frame+vertex];
    const Vertex3D& b=c.frames[(clip_start+next)*c.per_frame+vertex];
    out.x=a.x+(b.x-a.x)*mix;
    out.y=a.y+(b.y-a.y)*mix;
    out.z=a.z+(b.z-a.z)*mix;
}
}
bool ImportedHumanoid::load(const char* filename) {
    if(!filename || !filename[0])return false;
    FILE* f=std::fopen(filename,"rb");
    if(!f)return false;
    char magic[4]={};
    bool ok=std::fread(magic,1,4,f)==4 &&
            std::memcmp(magic,"EWHM",4)==0;
    unsigned ver=0,per=0,idle=0,walk=0,interact=0;
    float height=0,rate=0;
    if(ok){
        ver=readUInt(f,ok);
        per=readUInt(f,ok);
        idle=readUInt(f,ok);
        walk=readUInt(f,ok);
        interact=readUInt(f,ok);
        height=floatBits(readUInt(f,ok));
        rate=floatBits(readUInt(f,ok));
    }
    // Fail closed on corrupt or excessively large files. Decimation and
    // source provenance are validated before this file reaches the PSP.
    const unsigned frames=idle+walk+interact;
    if(!ok || ver!=1 || per<3 || per>MaxVertices || per%3 ||
       !idle || !walk || !interact || frames>40 ||
       idle>16 || walk>16 || interact>16 ||
       !finite(height) || height<1.0f || height>2.20f ||
       !finite(rate) || rate<1 || rate>40) {
        std::fclose(f);return false;
    }
    const std::size_t total=(std::size_t)frames*per;
    Cache next;
    next.per_frame=per;next.idle=idle;next.walk=walk;next.interact=interact;
    next.height=height;next.rate=rate;
    next.frames.resize(total);
    // C++11 and PSP both use ABGR + float XYZ, 16 bytes per triangle vertex.
    static_assert(sizeof(Vertex3D)==16,"Vertex3D ABI changed");
    const bool data_ok=std::fread(next.frames.data(),sizeof(Vertex3D),
                                  total,f)==total;
    const int trailing=std::fgetc(f);
    std::fclose(f);
    if(!data_ok || trailing!=EOF)return false;
    for(std::size_t i=0;i<total;++i){
        const Vertex3D& v=next.frames[i];
        if(!finite(v.x)||!finite(v.y)||!finite(v.z)||
           std::fabs(v.x)>4.0f || v.y < -1.5f || v.y>3.5f ||
           std::fabs(v.z)>4.0f || (v.color>>24)!=255u)
            return false;
    }
    // A failed reload leaves the previous valid character active.
    cache()=next;
    return true;
}
void ImportedHumanoid::clear(){cache()=Cache();}
bool ImportedHumanoid::available(){return cache().per_frame!=0;}
std::size_t ImportedHumanoid::storedBytes(){
    return cache().frames.size()*sizeof(Vertex3D);
}
unsigned ImportedHumanoid::triangleCount(){return cache().per_frame/3;}
bool ImportedHumanoid::sample(const Vec3& feet,float yaw,
                               const CharacterPose& pose,
                               std::vector<Vertex3D>& out) {
    const Cache& c=cache();
    if(!c.per_frame || !finite(yaw))return false;
    const float ct=std::cos(yaw),st=std::sin(yaw);
    out.clear();out.reserve(c.per_frame);
    const float blend=unit(pose.motion);
    const bool action=pose.action!=CharacterIdle && !pose.has_target;
    for(unsigned i=0;i<c.per_frame;++i){
        Vec3 local{0,0,0};
        if(action){
            interpolate(c,c.idle+c.walk,c.interact,
                        pose.action_progress,i,local);
        } else {
            Vec3 idle{0,0,0},walking{0,0,0};
            interpolate(c,0,c.idle,pose.clock_seconds*.31f,i,idle);
            interpolate(c,c.idle,c.walk,
                        pose.walk_phase/(2.0f*3.14159265358979323846f),
                        i,walking);
            local=Vec3{idle.x+(walking.x-idle.x)*blend,
                       idle.y+(walking.y-idle.y)*blend,
                       idle.z+(walking.z-idle.z)*blend};
        }
        const uint32_t abgr=c.frames[i].color;
        out.push_back(Vertex3D{abgr,
            feet.x+ct*local.x-st*local.z,
            feet.y+local.y,
            feet.z+st*local.x+ct*local.z});
    }
    return true;
}
} // namespace enginewatch
