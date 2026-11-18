#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace enginewatch;

static void geometryChecks(const World3D& world) {
    const std::vector<Vertex3D>& triangles=world.vertices();
    static_assert(sizeof(Vertex3D)==16,"PSP GU vertex layout must be color + XYZ");
    assert(triangles.size()>2500);
    assert(triangles.size()%3==0);
    assert(triangles.size()<65000);
    for(std::size_t i=0;i<triangles.size();++i) {
        assert(std::isfinite(triangles[i].x));
        assert(std::isfinite(triangles[i].y));
        assert(std::isfinite(triangles[i].z));
        assert((triangles[i].color>>24)==255u);
    }
    std::printf("3D triangles: %zu\n",triangles.size()/3);
}
static void collisionChecks(const World3D& w) {
    assert(w.canStand(-7.35f,0.0f));
    assert(w.canStand(0.0f,0.0f));
    assert(w.canStand(13.0f,3.0f));
    assert(!w.canStand(-11.0f,0.0f));
    assert(!w.canStand(16.0f,0.0f));
    assert(!w.canStand(-3.0f,3.0f)); // ECR divider is not traversable.
    assert(w.canStand(-3.0f,0.0f));  // Open doorway to engine room.
    assert(!w.canStand(7.0f,0.0f));  // Main engine is solid.
    assert(!w.canStand(6.0f,4.80f)); // Diesel generator is solid.
    assert(!w.canStand(0.6f,-3.7f)); // Pump station is solid.
}
static void interactionChecks(const World3D& w) {
    assert(w.lookAtStation(Vec3{-7.35f,1.65f,0.0f},3.14159265f)==EcrConsoleStation);
    assert(w.lookAtStation(Vec3{-7.35f,1.65f,0.0f},0.0f)==NoStation);
    assert(w.lookAtStation(Vec3{2.63f,1.65f,-2.79f},3.14159265f)==LoPumpStation);
    assert(w.lookAtStation(Vec3{3.10f,1.65f,-5.90f},3.14159265f)==StrainerStation);
}
static void gameplayChecks(const World3D& world) {
    Game3D game(world);
    Hud3D hud=game.hud();
    assert(std::strstr(hud.title,"MISSION 001"));
    assert(!hud.panel_visible);
    Input3D in;
    in.interact=true;
    game.update(0.016f,in); // Facing ECR console from the spawn point.
    hud=game.hud();
    assert(hud.panel_visible && std::strstr(hud.panel_title,"ECR"));
    in=Input3D{};
    in.back=true;
    game.update(0.016f,in);
    assert(!game.hud().panel_visible);

    in=Input3D{};
    in.turn=1.0f;
    for(int i=0;i<29;++i) game.update(0.05f,in); // Turn to face engine-room door.
    in=Input3D{};
    in.forward=1.0f;
    for(int i=0;i<45;++i) game.update(0.05f,in);
    assert(game.camera().x>-3.0f); // Walk through a physical doorway, no teleport.
    assert(world.isInEcr(-7.35f));
    assert(!world.isInEcr(game.camera().x));
    assert(world.canStand(game.camera().x,game.camera().z));

    in=Input3D{};
    in.pause=true;
    game.update(0.02f,in);
    assert(game.isPaused());
    in=Input3D{};
    in.interact=true;
    game.update(0.02f,in);
    assert(!game.isPaused());
    assert(game.camera().x==-7.35f);
    assert(game.simulation().state().mission==1);
}

int main() {
    World3D world;
    geometryChecks(world);
    collisionChecks(world);
    interactionChecks(world);
    gameplayChecks(world);
    std::puts("World geometry, doorway collision, interactions and game state: OK");
    return 0;
}
