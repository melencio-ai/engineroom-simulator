#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"
#include "scene/PlayerCharacter.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace enginewatch;
namespace {
const float PI=3.14159265358979323846f;

float distance(const Vec3& a,const Vec3& b) {
    const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
    return std::sqrt(x*x+y*y+z*z);
}

float closestVertex(const std::vector<Vertex3D>& vertices,
                    const Vec3& target) {
    float closest=10000.0f;
    for(std::size_t i=0;i<vertices.size();++i) {
        const Vertex3D& p=vertices[i];
        const float dx=p.x-target.x,dy=p.y-target.y,dz=p.z-target.z;
        closest=std::min(closest,std::sqrt(dx*dx+dy*dy+dz*dz));
    }
    return closest;
}

void socketCatalogChecks(const World3D& w) {
    assert(w.interactionSockets().size()==InteractionSocketCount-1);
    assert(World3D::pumpMenuSocket(-1)==NoInteractionSocket);
    assert(World3D::pumpMenuSocket(6)==NoInteractionSocket);
    for(int i=1;i<InteractionSocketCount;++i) {
        const InteractionSocketId id=static_cast<InteractionSocketId>(i);
        const InteractionSocket* s=w.interactionSocket(id);
        assert(s && s->id==id);
        assert(std::isfinite(s->point.x) && std::isfinite(s->point.y) &&
               std::isfinite(s->point.z));
        const float normal=distance(s->outward,Vec3{0,0,0});
        assert(std::fabs(normal-1.0f)<0.001f);
        assert(s->max_eye_distance>=1.20f && s->max_eye_distance<=1.55f);
        for(int j=i+1;j<InteractionSocketCount;++j) {
            const InteractionSocket* other=w.interactionSocket(
                static_cast<InteractionSocketId>(j));
            assert(other && other->id!=id);
            assert(distance(s->point,other->point)>0.08f);
        }
    }

    // Six physically separate pushbuttons all share the EAST face of the
    // already rendered LO station cabinet (its external surface is x~2.05).
    const Vec3 operatorEye{2.64f,World3D::floorHeight(PumpFlatDeck)+1.65f,
                           -2.81f};
    for(int i=0;i<6;++i) {
        const InteractionSocket* s=w.interactionSocket(World3D::pumpMenuSocket(i));
        assert(s && s->station==LoPumpStation &&
               s->kind==PanelPushbutton);
        assert(std::fabs(s->point.x-2.119f)<0.001f);
        assert(s->point.y>4.20f && s->point.y<5.0f);
        assert(w.canReachSocket(s->id,operatorEye,PI));
        assert(!w.canReachSocket(s->id,operatorEye,0.0f));
        assert(!w.canReachSocket(s->id,Vec3{1.35f,4.85f,-2.81f},PI));
        assert(!w.canReachSocket(s->id,Vec3{4.15f,4.85f,-2.81f},PI));
        assert(!w.canReachSocket(s->id,Vec3{2.64f,8.05f,-2.81f},PI));
    }
    const InteractionSocket* ecr=w.interactionSocket(EcrVerifyButton);
    assert(ecr && ecr->station==EcrConsoleStation);
    assert(w.canReachSocket(EcrVerifyButton,Vec3{-8.12f,14.45f,0},PI));
    assert(!w.canReachSocket(EcrVerifyButton,Vec3{-7.35f,14.45f,0},PI));

    const InteractionSocket* gauge=w.interactionSocket(LoMainPressureGauge);
    assert(gauge && gauge->kind==GaugeInspection);
    assert(std::fabs(gauge->point.z+4.38f)<0.001f);
    assert(w.canReachSocket(LoMainPressureGauge,
                            Vec3{1.0f,4.85f,-4.38f},PI));
    assert(w.interactionSocket(LoStandbyPressureGauge)->kind==GaugeInspection);

    const InteractionSocket* strainer=w.interactionSocket(StrainerInspectionHandle);
    assert(strainer && strainer->kind==EquipmentHandle);
    assert(w.canReachSocket(StrainerInspectionHandle,
                            Vec3{2.65f,4.85f,-5.87f},PI));
    assert(!w.canReachSocket(NoInteractionSocket,operatorEye,PI));
}

void handContactChecks(const World3D& w) {
    const InteractionSocket* panel=w.interactionSocket(LoMainStartButton);
    assert(panel);
    const Vec3 eye{2.64f,4.85f,-2.81f};
    const Vec3 feet{eye.x,eye.y-PlayerCharacter::EyeHeight,eye.z};
    CharacterAnimator a;
    a.trigger(CharacterPressControl,&panel->point);
    assert(a.pose().has_target);
    assert(a.pose().action_progress==0.0f);
    for(int frame=0;frame<7;++frame) a.advance(0.05f,0.0f,false);
    assert(a.pose().action_progress>0.47f);
    assert(a.pose().action==CharacterPressControl);
    const CharacterPose& pose=a.pose();

    std::vector<Vertex3D> body,hands;
    PlayerCharacter::buildBody(feet,PI,pose,body);
    PlayerCharacter::buildFirstPersonHands(eye,PI,0,pose,hands);
    assert(body.size()>400 && body.size()<PlayerCharacter::MaxRuntimeVertices);
    assert(hands.size()>400 && hands.size()<PlayerCharacter::MaxRuntimeVertices);
    assert(body.size()%3==0 && hands.size()%3==0);
    // Low-poly target alignment is a coarse glove/fingertip approximation,
    // not millimetre-accurate manufacturer CAD or robotic-grade inverse IK.
    assert(closestVertex(body,panel->point)<0.18f);
    assert(closestVertex(hands,panel->point)<0.18f);
    for(std::size_t i=0;i<hands.size();++i) {
        assert(std::isfinite(hands[i].x) && std::isfinite(hands[i].y) &&
               std::isfinite(hands[i].z));
        assert(hands[i].x < eye.x+0.10f &&
               "Facing WEST: working glove must stay in front of camera");
    }
    for(int frame=0;frame<11;++frame)a.advance(0.05f,0.0f,false);
    assert(a.pose().action==CharacterIdle);
    assert(!a.pose().has_target);
}

} // namespace

int main() {
    World3D world;
    const std::size_t before=world.vertices().size();
    socketCatalogChecks(world);
    handContactChecks(world);
    assert(world.vertices().size()==before);
    assert(before<130000u && "10 action sockets must not exceed PSP static budget");
    std::fprintf(stderr,
        "G3.6 PASS: %zu visible equipment sockets, physical reach and facing, "
        "3D glove contact, zero mission or static-scene budget side effects\n",
        world.interactionSockets().size());
    return 0;
}
