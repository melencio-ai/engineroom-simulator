#include "scene/Game3D.hpp"
#include "scene/PlayerCharacter.hpp"
#include "scene/World3D.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace enginewatch;

namespace {
const float PI=3.14159265358979323846f;

bool finiteMesh(const std::vector<Vertex3D>& v) {
    if(v.empty() || v.size()%3!=0 ||
       v.size()>PlayerCharacter::MaxRuntimeVertices)
        return false;
    for(std::size_t i=0;i<v.size();++i) {
        const Vertex3D& p=v[i];
        if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
           ((p.color>>24)&255u)!=255u)
            return false;
    }
    return true;
}

float horizontalExtent(const std::vector<Vertex3D>& v,int axis) {
    float a=1000000.0f,b=-1000000.0f;
    for(std::size_t i=0;i<v.size();++i) {
        const float val=axis==0?v[i].x:axis==1?v[i].y:v[i].z;
        if(val<a)a=val;
        if(val>b)b=val;
    }
    return b-a;
}

bool changedMesh(const std::vector<Vertex3D>& a,
                 const std::vector<Vertex3D>& b) {
    if(a.size()!=b.size())return true;
    for(std::size_t i=0;i<a.size();++i) {
        if(std::fabs(a[i].x-b[i].x)>0.002f ||
           std::fabs(a[i].y-b[i].y)>0.002f ||
           std::fabs(a[i].z-b[i].z)>0.002f)
            return true;
    }
    return false;
}

void geometryChecks() {
    assert(std::fabs(PlayerCharacter::EyeHeight-1.65f)<0.001f);
    assert(std::fabs(PlayerCharacter::BodyHeight-1.82f)<0.001f);
    const Vec3 feet{0,0,0},eye{0,1.65f,0};
    CharacterPose idle;
    std::vector<Vertex3D> body,hands,walk,reach;
    PlayerCharacter::buildBody(feet,0,idle,body);
    PlayerCharacter::buildFirstPersonHands(eye,0,-0.16f,idle,hands);
    assert(finiteMesh(body) && finiteMesh(hands));
    assert(body.size()>500);
    assert(hands.size()>100 && hands.size()<body.size());
    assert(horizontalExtent(body,0)<1.30f);
    assert(horizontalExtent(body,2)<1.10f);
    assert(horizontalExtent(body,1)>1.70f);
    float bottom=100,top=-100;
    for(std::size_t i=0;i<body.size();++i) {
        bottom=std::fmin(bottom,body[i].y);
        top=std::fmax(top,body[i].y);
    }
    assert(bottom>-0.065f && "Static boots must not sink into the deck");
    assert(top<=PlayerCharacter::BodyHeight+0.035f);
    assert(top>1.79f);

    CharacterPose walking=idle;
    walking.walk_phase=PI/2.0f;
    walking.motion=1.0f;
    PlayerCharacter::buildBody(feet,0,walking,walk);
    assert(finiteMesh(walk));
    assert(walk.size()==body.size());
    assert(changedMesh(body,walk));
    // Both left and right gloved hands must remain visible while advancing.
    PlayerCharacter::buildFirstPersonHands(eye,0,0,walking,reach);
    assert(finiteMesh(reach));
    assert(reach.size()==hands.size());

    CharacterPose press=idle;
    press.action=CharacterPressControl;
    press.action_progress=0.5f;
    PlayerCharacter::buildFirstPersonHands(eye,0,0,press,reach);
    std::vector<Vertex3D> atRest;
    PlayerCharacter::buildFirstPersonHands(eye,0,0,idle,atRest);
    assert(changedMesh(atRest,reach));

    PlayerCharacter::buildBody(feet,1.1f,press,reach);
    assert(finiteMesh(reach));
    assert(reach.size()==body.size());
    assert(changedMesh(body,reach));

    // Camera-space hands must never be behind the 0.25-m near plane;
    // render from the eye at nonzero yaw/pitch and test the view dot product.
    const float yaw=0.7f,pitch=-0.25f;
    PlayerCharacter::buildFirstPersonHands(eye,yaw,pitch,idle,hands);
    const Vec3 dir{std::cos(yaw)*std::cos(pitch),
                   std::sin(pitch),
                   std::sin(yaw)*std::cos(pitch)};
    for(std::size_t i=0;i<hands.size();++i) {
        const float dx=hands[i].x-eye.x;
        const float dy=hands[i].y-eye.y;
        const float dz=hands[i].z-eye.z;
        const float ahead=dx*dir.x+dy*dir.y+dz*dir.z;
        assert(ahead>=0.27f && "First-person glove crossed near clip plane");
    }
}

void animationChecks() {
    CharacterAnimator anim;
    assert(anim.pose().action==CharacterIdle);
    assert(anim.pose().motion==0);
    anim.advance(0.05f,0.10f,false);
    assert(anim.pose().motion>0 && anim.pose().motion<=1);
    assert(anim.pose().walk_phase>0);
    const float walkPhase=anim.pose().walk_phase;
    anim.advance(0.05f,0.0f,false);
    assert(anim.pose().walk_phase==walkPhase &&
           "Blocked movement cannot cause foot stepping");
    anim.advance(0.05f,0.05f,true);
    assert(anim.pose().climbing);
    assert(anim.pose().walk_phase>walkPhase);
    anim.trigger(CharacterPressControl);
    assert(anim.pose().action==CharacterPressControl);
    anim.advance(0.05f,0,false);
    assert(anim.pose().action_progress>0.0f);
    for(int i=0;i<18;++i)anim.advance(0.05f,0,false);
    assert(anim.pose().action==CharacterIdle);
    anim.trigger(CharacterInspectInstrument);
    for(int i=0;i<25;++i)anim.advance(0.05f,0,false);
    assert(anim.pose().action==CharacterIdle);
    anim.reset();
    assert(anim.pose().walk_phase==0);
    assert(anim.pose().motion==0);
    assert(anim.pose().action==CharacterIdle);
}

void polishedHandChecks() {
    const Vec3 eye{0,1.65f,0};
    CharacterPose idle;
    std::vector<Vertex3D> hands,working;
    PlayerCharacter::buildFirstPersonHands(eye,0,0,idle,hands);
    assert(finiteMesh(hands));
    // Six distinct gloved segments per hand (palm + four finger pads +
    // thumb) should be visibly richer than the previous 72-triangle blocks.
    assert(hands.size()>=320 && hands.size()<=720);

    // With the standard 68-degree vertical FOV, the main glove/palm is
    // positioned in the lower-middle screen, not behind the 226px HUD and
    // not on top of the crosshair at (240,136).
    // Test the center of each glove from its three-dimensional vertices:
    const float t=std::tan(68.0f*PI/360.0f);
    int visible_lower=0,blocked_crosshair=0;
    for(std::size_t i=0;i<hands.size();++i) {
        const Vertex3D& p=hands[i];
        if(p.x<=0.26f) continue;
        const float screenX=240.0f+136.0f*(p.z/p.x)/t;
        const float screenY=136.0f-136.0f*((p.y-eye.y)/p.x)/t;
        if(screenY>139 && screenY<226 &&
           screenX>30 && screenX<450)++visible_lower;
        if(std::fabs(screenX-240.0f)<18.0f &&
           std::fabs(screenY-136.0f)<18.0f)++blocked_crosshair;
    }
    assert(visible_lower>140 && "Hands must be visible above the bottom HUD");
    assert(blocked_crosshair==0 && "Working gloves must leave the reticle clear");

    CharacterPose reaching=idle;
    reaching.action=CharacterPressControl;
    reaching.action_progress=0.5f;
    PlayerCharacter::buildFirstPersonHands(eye,0,0,reaching,working);
    assert(finiteMesh(working) && working.size()==hands.size());
    assert(changedMesh(hands,working));
}

void stairCameraChecks() {
    World3D world;
    Game3D game(world);
    Input3D toggle;toggle.toggle_view=true;
    game.update(0.05f,toggle);
    assert(game.isThirdPerson());
    assert(game.renderCamera().show_body);
    assert(game.renderCamera().fov_degrees>=76.0f);
    assert(game.renderCamera().fov_degrees<=91.0f);

    // Walk the actual ECR bridge and descend one full inclined ladder.
    // The previous renderer switched to FP arms while still displaying 3P
    // as soon as its chasing camera encountered the open stair well.
    const float goals[3][2]={{-2.35f,0.0f},{-2.35f,5.0f},{1.35f,5.0f}};
    bool sawStairs=false;
    bool sawClose=false;
    for(int goal=0;goal<3;++goal) {
        bool arrived=false;
        for(int frame=0;frame<550;++frame) {
            const float dx=goals[goal][0]-game.camera().x;
            const float dz=goals[goal][1]-game.camera().z;
            const float dist=std::sqrt(dx*dx+dz*dz);
            if(dist<0.07f) {arrived=true;break;}
            Input3D motion;
            motion.forward=dx/dist;
            motion.strafe=dz/dist;  // Start view facing positive X.
            game.update(0.05f,motion);
            const CameraView3D view=game.renderCamera();
            assert(view.show_body &&
                   "Inclined stairs must not silently switch 3P into FP");
            assert(std::isfinite(view.eye.x) && std::isfinite(view.eye.y)
                   && std::isfinite(view.eye.z));
            assert(view.fov_degrees>=76.0f && view.fov_degrees<=91.0f);
            if(view.constrained) sawClose=true;
            assert(std::strstr(game.hud().status,"VIEW 3P")!=nullptr &&
                   "The displayed camera mode must agree with rendered geometry");
            if(game.isClimbing()) sawStairs=true;
        }
        assert(arrived && "ECR to first stair failed navigation");
    }
    assert(sawStairs && "The camera walk test must include actual stair motion");
    assert(sawClose && "The camera must detect constrained bridge/stair geometry");
    assert(game.currentDeck()==AuxiliaryDeck);
    assert(game.renderCamera().show_body);
}

void gameChecks() {
    World3D world;
    const std::size_t static_before=world.vertices().size();
    Game3D game(world);
    assert(!game.isThirdPerson());
    const CameraView3D first=game.renderCamera();
    assert(!first.show_body);
    assert(std::fabs(first.eye.x-game.camera().x)<0.0001f);
    std::vector<Vertex3D> dynamic;
    game.buildCharacterGeometry(first,dynamic);
    assert(finiteMesh(dynamic));
    assert(world.vertices().size()==static_before &&
           "First-person hands must not inflate static PSP scene");

    Input3D toggle;
    toggle.toggle_view=true;
    game.update(1.0f/60.0f,toggle);
    assert(game.isThirdPerson());
    const CameraView3D third=game.renderCamera();
    assert(third.show_body &&
           "ECR starting point should allow a visible third-person engineer");
    assert(third.eye.x<game.camera().x);
    assert(std::fabs(third.eye.x-first.eye.x)>0.5f);
    assert(third.eye.y>game.camera().y);
    game.buildCharacterGeometry(third,dynamic);
    assert(finiteMesh(dynamic));
    assert(dynamic.size()>500);
    assert(world.vertices().size()==static_before &&
           "Third-person body must stay in transient mesh");

    // Switching views must not move the character, reset missions or
    // alter the established first-person 1.65-m eye height.
    const Vec3 before=game.camera();
    const float pressure=game.simulation().state().lo_pressure_bar;
    game.update(1.0f/60.0f,toggle);
    assert(!game.isThirdPerson());
    const Vec3 after=game.camera();
    assert(before.x==after.x && before.y==after.y && before.z==after.z);
    assert(std::fabs(pressure-game.simulation().state().lo_pressure_bar)<0.05f);

    Input3D forward;
    forward.forward=1.0f;
    game.update(0.05f,forward);
    assert(game.camera().x>before.x);
    assert(game.characterPose().motion>0);
    for(int i=0;i<15;++i)game.update(0.05f,Input3D{});
    assert(game.characterPose().motion<0.05f);

    // ECR control interaction: face rear console and inspect. An animation
    // is triggered, but existing mission simulation remains authoritative.
    game.reset();
    Input3D turn;
    turn.turn=1.0f;
    for(int i=0;i<29;++i)game.update(0.05f,turn);
    assert(world.lookAtStation(game.camera(),game.yaw())==EcrConsoleStation);
    Input3D use;
    use.interact=true;
    game.update(0.05f,use);
    assert(game.hud().panel_visible);
    assert(game.characterPose().action==CharacterInspectInstrument);
    game.update(0.05f,use);
    assert(game.characterPose().action==CharacterPressControl);
    assert(game.simulation().state().mission==1);
    assert(world.vertices().size()==static_before);
    assert(std::strstr(game.hud().status,"VIEW 1P")!=nullptr);

    game.update(0.05f,toggle);
    assert(game.isThirdPerson());
    Input3D pause;pause.pause=true;
    game.update(0.05f,pause);
    assert(game.isPaused());
    Input3D reset;reset.interact=true;
    game.update(0.05f,reset);
    assert(!game.isPaused());
    assert(!game.isThirdPerson());
    assert(game.characterPose().action==CharacterIdle);
    assert(game.currentDeck()==EcrDeck);
}

} // namespace

int main() {
    geometryChecks();
    animationChecks();
    polishedHandChecks();
    gameChecks();
    stairCameraChecks();
    std::puts("G3.5 PASS: low-poly marine engineer, real-travel gait, PPE, "
              "first-person gloves, 3P chase camera, pump/ECR animation and mission preservation");
    return 0;
}
