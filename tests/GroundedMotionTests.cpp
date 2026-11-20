#include "scene/Game3D.hpp"
#include "scene/PlayerCharacter.hpp"
#include "scene/World3D.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace enginewatch;

namespace {
bool finiteBody(const std::vector<Vertex3D>& vertices) {
    if(vertices.empty() || vertices.size()%3 ||
       vertices.size()>PlayerCharacter::MaxRuntimeVertices) return false;
    for(std::size_t i=0;i<vertices.size();++i) {
        if(!std::isfinite(vertices[i].x) ||
           !std::isfinite(vertices[i].y) ||
           !std::isfinite(vertices[i].z)) return false;
    }
    return true;
}
void slopeAnimatorChecks() {
    CharacterAnimator animator;
    animator.advance(0.05f,0.10f,true,-0.06f,0.12f,-0.08f);
    assert(animator.pose().climbing);
    assert(animator.pose().stair_blend>0.0f);
    assert(animator.pose().stair_direction<-0.05f);
    assert(animator.pose().left_foot_offset>0.0f);
    assert(animator.pose().right_foot_offset<0.0f);
    // Real stair pace gives full gait intensity, not the flat-deck ratio.
    assert(animator.pose().motion>0.4f);
    const float phase=animator.pose().walk_phase;
    animator.advance(0.05f,0.0f,true);
    assert(animator.pose().walk_phase==phase);
    assert(animator.pose().motion<1.0f);
    for(int i=0;i<12;++i)
        animator.advance(0.05f,0.10f,true,0.06f,0.05f,0.12f);
    assert(animator.pose().stair_direction>0.10f);
    assert(animator.pose().stair_blend>0.9f);

    for(int i=0;i<14;++i)
        animator.advance(0.05f,0.0f,false);
    assert(animator.pose().stair_blend<0.005f);
    assert(std::fabs(animator.pose().left_foot_offset)<0.005f);
    assert(std::fabs(animator.pose().right_foot_offset)<0.005f);
    animator.reset();
    assert(animator.pose().stair_blend==0.0f);
    assert(animator.pose().stair_direction==0.0f);
}
void contactGeometryChecks() {
    PlayerCharacter::clearImportedBody();
    const Vec3 feet{0,0,0};
    CharacterPose flat,stair;
    stair.stair_blend=1.0f;
    stair.stair_direction=0.80f;
    stair.left_foot_offset=0.12f;
    stair.right_foot_offset=-0.08f;
    std::vector<Vertex3D> original,corrected;
    PlayerCharacter::buildBody(feet,0.0f,flat,original);
    PlayerCharacter::buildBody(feet,0.0f,stair,corrected);
    assert(finiteBody(original) && finiteBody(corrected));
    assert(original.size()==corrected.size());
    float left_total=0,right_total=0,head_total=0;
    int left_count=0,right_count=0,head_count=0;
    for(std::size_t i=0;i<original.size();++i) {
        const Vertex3D& o=original[i];
        const Vertex3D& p=corrected[i];
        if(o.y<0.22f && o.z<-0.08f) {
            left_total+=p.y-o.y;++left_count;
        }
        if(o.y<0.22f && o.z>0.08f) {
            right_total+=p.y-o.y;++right_count;
        }
        if(o.y>1.55f) {
            head_total+=p.x-o.x;++head_count;
        }
    }
    assert(left_count>5 && right_count>5 && head_count>5);
    assert(left_total/left_count>0.06f);
    assert(right_total/right_count<-0.03f);
    assert(head_total/head_count>0.03f);
    // A camera turn does not change contact by rotating in world space.
    PlayerCharacter::buildBody(feet,1.57079632679f,stair,corrected);
    assert(finiteBody(corrected));
}
void actualStairChecks() {
    World3D world;
    Game3D game(world);
    Input3D toggle;toggle.toggle_view=true;
    game.update(0.05f,toggle);
    assert(game.renderCamera().show_body);
    const float goals[3][2]={{-2.35f,0.0f},{-2.35f,5.0f},{1.35f,5.0f}};
    bool used_real_stair=false,saw_descent=false,saw_contact=false;
    for(int goal=0;goal<3;++goal) {
        bool arrived=false;
        for(int frame=0;frame<550;++frame) {
            const float dx=goals[goal][0]-game.camera().x;
            const float dz=goals[goal][1]-game.camera().z;
            const float distance=std::sqrt(dx*dx+dz*dz);
            if(distance<0.07f){arrived=true;break;}
            Input3D input;
            input.forward=dx/distance;
            input.strafe=dz/distance;
            game.update(0.05f,input);
            const CharacterPose& pose=game.characterPose();
            if(game.isClimbing()) {
                used_real_stair=true;
                if(pose.stair_direction<-0.03f) saw_descent=true;
                if(std::fabs(pose.left_foot_offset-pose.right_foot_offset)>0.005f)
                    saw_contact=true;
                const CameraView3D camera=game.renderCamera();
                std::vector<Vertex3D> body;
                game.buildCharacterGeometry(camera,body);
                assert(finiteBody(body) && camera.show_body);
            }
        }
        assert(arrived);
    }
    assert(used_real_stair && saw_descent);
    // At least one pair of foot probes must resolve different physical
    // tread heights as the source walk clip travels the inclined flight.
    assert(saw_contact);
    assert(game.currentDeck()==AuxiliaryDeck);
}
}
int main() {
    slopeAnimatorChecks();
    contactGeometryChecks();
    actualStairChecks();
    std::puts("G3.8 PASS: real slope direction, individual tread contacts, PSP geometry bounds and full ECR stair descent");
    return 0;
}
