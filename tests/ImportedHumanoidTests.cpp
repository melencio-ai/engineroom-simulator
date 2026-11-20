#include "scene/ImportedHumanoid.hpp"
#include "scene/PlayerCharacter.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>

using namespace enginewatch;

namespace {
void uint32(FILE* f,uint32_t v) {
    unsigned char b[4]={
        (unsigned char)(v&255),
        (unsigned char)((v>>8)&255),
        (unsigned char)((v>>16)&255),
        (unsigned char)((v>>24)&255)
    };
    assert(std::fwrite(b,1,4,f)==4);
}
void float32(FILE* f,float v) {
    uint32_t bits=0;std::memcpy(&bits,&v,4);
    uint32(f,bits);
}
void fixture(const char* filename,bool corrupt) {
    FILE* f=std::fopen(filename,"wb");
    assert(f);
    assert(std::fwrite(corrupt?"BAD!":"EWHM",1,4,f)==4);
    uint32(f,1); // Version
    uint32(f,3); // Exactly 1 triangle per frame
    uint32(f,2);uint32(f,2);uint32(f,2); // idle/walk/interact
    float32(f,1.82f);float32(f,12.0f);
    for(int frame=0;frame<6;++frame) {
        for(int j=0;j<3;++j) {
            const uint32_t abgr=0xff8899aau;
            uint32(f,abgr);
            float32(f,0.1f*j+0.2f*(frame>=2&&frame<4));
            float32(f,j==0?0.1f:j==1?1.82f:0.8f);
            float32(f,j==2?0.4f:0.0f);
        }
    }
    assert(std::fclose(f)==0);
}
void verify() {
    PlayerCharacter::clearImportedBody();
    assert(!PlayerCharacter::hasImportedBody());
    CharacterPose p;
    std::vector<Vertex3D> out;
    PlayerCharacter::buildBody(Vec3{0,0,0},0,p,out);
    assert(out.size()>900 && out.size()<1800);
    assert(!PlayerCharacter::loadImportedBody("no_such_file.ewmh"));
    fixture("test-engineer.ewmh",false);
    assert(PlayerCharacter::loadImportedBody("test-engineer.ewmh"));
    assert(PlayerCharacter::hasImportedBody());
    assert(ImportedHumanoid::triangleCount()==1);
    assert(ImportedHumanoid::storedBytes()==6*3*sizeof(Vertex3D));
    PlayerCharacter::buildBody(Vec3{2,3,4},0,p,out);
    assert(out.size()==3 && out[0].color==0xff8899aau);
    assert(std::fabs(out[0].x-2.0f)<0.00001f);
    assert(std::fabs(out[0].y-3.1f)<0.00001f);
    assert(std::fabs(out[2].z-4.4f)<0.00001f);

    // Real distance-driven walk switches to frames of Quaternius animation,
    // preserving the user's camera motion and 3P body origin.
    p.motion=1.0f;p.walk_phase=3.14159265358979323846f/2.0f;
    PlayerCharacter::buildBody(Vec3{2,3,4},0,p,out);
    assert(std::fabs(out[0].x-2.2f)<0.00001f);
    PlayerCharacter::buildBody(Vec3{2,3,4},1.57079632679f,p,out);
    assert(std::fabs(out[0].z-4.2f)<0.00001f);

    // ECR "Interact" action can use imported motion without adding any new
    // PSP skinned-mesh or material API.
    p.action=CharacterInspectInstrument;
    p.has_target=false;
    p.action_progress=0.5f;
    PlayerCharacter::buildBody(Vec3{0,0,0},0,p,out);
    assert(out.size()==3);
    // G3.6 physical contact retains its proven two-link procedural reach:
    p.action=CharacterPressControl;
    p.has_target=true;
    p.target_world=Vec3{0.5f,1.3f,0.5f};
    PlayerCharacter::buildBody(Vec3{0,0,0},0,p,out);
    assert(out.size()>900 && out.size()<1800);

    fixture("test-engineer-bad.ewmh",true);
    assert(!PlayerCharacter::loadImportedBody("test-engineer-bad.ewmh"));
    assert(PlayerCharacter::hasImportedBody() &&
           "Corrupt resource must not unload valid active humanoid");
    PlayerCharacter::clearImportedBody();
    assert(!PlayerCharacter::hasImportedBody());
    PlayerCharacter::buildBody(Vec3{0,0,0},0,CharacterPose{},out);
    assert(out.size()>900); // Working procedural fallback
    std::remove("test-engineer.ewmh");
    std::remove("test-engineer-bad.ewmh");
}
}

int main() {
    verify();
    std::puts("G3.7 PASS: validated Quaternius baked-clip loader, idle/walk/"
              "interact sampling, joint contact fallback and corruption guard");
    return 0;
}
