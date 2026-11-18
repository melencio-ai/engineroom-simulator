#include "scene/Game3D.hpp"
#include "scene/World3D.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace enginewatch;

static void step(Game3D& game,Input3D input,int frames=1) {
    for(int i=0;i<frames;++i) {
        if(i>0) {
            input.interact=false;
            input.back=false;
            input.help=false;
            input.pause=false;
            input.up=false;
            input.down=false;
        }
        game.update(0.05f,input);
    }
}

static void geometryChecks(const World3D& w) {
    const std::vector<Vertex3D>& mesh=w.vertices();
    static_assert(sizeof(Vertex3D)==16,"PSP GU expects colour + XYZ vertex");
    assert(mesh.size()>4000);
    assert(mesh.size()%3==0);
    assert(mesh.size()<90000);
    float miny=999.0f,maxy=-999.0f;
    for(std::size_t i=0;i<mesh.size();++i) {
        const Vertex3D& v=mesh[i];
        assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));
        assert((v.color>>24)==255u);
        miny=std::fmin(miny,v.y);
        maxy=std::fmax(maxy,v.y);
    }
    assert(miny<0.0f && maxy>17.0f);
    assert(w.ladderCount()==2);
    assert(w.ladder(0).shaft_x!=w.ladder(1).shaft_x);
    std::printf("3D vertices %zu, triangles %zu, height %.1f..%.1f\n",
                mesh.size(),mesh.size()/3,miny,maxy);
}

static void deckChecks(const World3D& w) {
    for(int d=0;d<DeckCount;++d) {
        assert(std::fabs(World3D::floorHeight(d)-3.2f*d)<0.01f);
        assert(std::strlen(World3D::deckName(d))>5);
        assert(World3D::cameraDeck(World3D::floorHeight(d)+1.65f)==d);
        assert(w.canStand(-0.75f,3.70f,d));
        assert(w.canStand(12.20f,-5.00f,d));
        assert(w.canStand(7.0f,4.0f,d));
        assert(!w.canStand(7.0f,0.0f,d)); // Multi-level engine void/foundation.
        assert(!w.canStand(16.0f,0.0f,d));
        assert(!w.canStand(-0.75f,8.0f,d));
        if(d>TankTopDeck) {
            assert(!w.canStand(-0.75f,5.0f,d)); // Open ladder shaft.
            assert(!w.canStand(13.50f,-5.0f,d)); // Alternate shaft.
            assert(!w.canStand(6.5f,1.8f,d)); // Not a solid floor across engine.
        }
    }
    assert(w.canStand(-7.35f,0.0f,EcrDeck));
    assert(!w.canStand(-7.35f,0.0f,PumpFlatDeck));
    assert(!w.canStand(-3.20f,2.50f,EcrDeck)); // No walking through the ECR wall.
    assert(w.canStand(-3.20f,0.0f,EcrDeck)); // Actual ECR door.
    assert(!w.canStand(0.65f,-4.38f,PumpFlatDeck));
    assert(!w.canStand(1.50f,0.0f,-1));
}

static void stationChecks(const World3D& w) {
    for(int d=0;d<DeckCount;++d) {
        const float y=World3D::floorHeight(d)+1.65f;
        assert(w.lookAtStation(Vec3{-0.75f,y,3.70f},0.0f)==MainLadderStation);
        assert(w.lookAtStation(Vec3{12.20f,y,-5.00f},0.0f)==EscapeLadderStation);
    }
    const float ecr=World3D::floorHeight(EcrDeck)+1.65f;
    const float pump=World3D::floorHeight(PumpFlatDeck)+1.65f;
    assert(w.lookAtStation(Vec3{-7.35f,ecr,0.0f},3.14159265f)==EcrConsoleStation);
    assert(w.lookAtStation(Vec3{-7.35f,ecr,0.0f},0.0f)==NoStation);
    assert(w.lookAtStation(Vec3{2.64f,pump,-2.81f},3.14159265f)==LoPumpStation);
    assert(w.lookAtStation(Vec3{2.80f,pump,-5.87f},3.14159265f)==StrainerStation);
    assert(w.lookAtStation(Vec3{2.64f,ecr,-2.81f},3.14159265f)!=LoPumpStation);
    assert(w.lookAtStation(Vec3{2.80f,ecr,-5.87f},3.14159265f)!=StrainerStation);
}

static void gameplayChecks(const World3D& world) {
    Game3D game(world);
    assert(game.currentDeck()==EcrDeck);
    assert(game.camera().y>14.40f);
    assert(game.camera().y<14.50f);
    assert(game.pitch()<0.0f);
    assert(std::strstr(game.hud().title,"ECR / UPPER"));
    assert(!game.hud().panel_visible);
    assert(world.canStand(game.camera().x,game.camera().z,game.currentDeck()));

    Input3D in;
    in.forward=1.0f;
    step(game,in,43); // ECR -> upper walkway, through physical doorway.
    assert(game.camera().x>-1.10f && game.camera().x< -0.45f);
    assert(game.currentDeck()==EcrDeck);
    assert(world.canStand(game.camera().x,game.camera().z,EcrDeck));

    in=Input3D{};
    in.turn=1.0f;
    step(game,in,15);
    in=Input3D{};
    in.forward=1.0f;
    step(game,in,23); // Walk toward main ladder landing.
    assert(world.lookAtStation(game.camera(),game.yaw())==MainLadderStation);

    for(int next=EcrDeck-1;next>=TankTopDeck;--next) {
        in=Input3D{};
        in.interact=true;
        step(game,in); // Open ladder panel.
        Hud3D hud=game.hud();
        assert(hud.panel_visible && std::strstr(hud.panel_title,"SERVICE"));

        if(game.currentDeck()!=EcrDeck) {
            in=Input3D{};
            in.down=true;
            step(game,in); // Select DOWN when deck has UP as default.
        }

        in=Input3D{};
        in.interact=true;
        step(game,in);
        assert(game.isClimbing());
        const float beforeY=game.camera().y;
        in=Input3D{};
        step(game,in,26);
        assert(game.isClimbing());
        assert(std::fabs(game.camera().y-beforeY)>0.20f);
        step(game,in,30);
        assert(!game.isClimbing());
        assert(game.currentDeck()==next);
        assert(world.canStand(game.camera().x,game.camera().z,next));
        assert(std::fabs(game.camera().y-World3D::floorHeight(next)-1.65f)<0.01f);
    }
    assert(game.currentDeck()==TankTopDeck);
    // The bottom landing cannot descend outside the world.
    in=Input3D{};
    in.interact=true;
    step(game,in);
    in=Input3D{};
    in.down=true;
    step(game,in);
    in=Input3D{};
    in.interact=true;
    step(game,in);
    assert(!game.isClimbing());
    assert(game.currentDeck()==TankTopDeck);

    in=Input3D{};
    in.pause=true;
    step(game,in);
    assert(game.isPaused());
    in=Input3D{};
    in.interact=true;
    step(game,in);
    assert(!game.isPaused());
    assert(game.currentDeck()==EcrDeck);
    assert(game.simulation().state().mission==1);
}

int main() {
    World3D world;
    geometryChecks(world);
    deckChecks(world);
    stationChecks(world);
    gameplayChecks(world);
    std::puts("Multi-deck geometry, collision, stations and continuous ladders: OK");
    return 0;
}
