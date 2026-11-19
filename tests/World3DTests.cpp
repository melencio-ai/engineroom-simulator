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

static void driveTo(Game3D& game,float x,float z,int max_frames=240) {
    for(int i=0;i<max_frames;++i) {
        const float dx=x-game.camera().x,dz=z-game.camera().z;
        const float dist=std::sqrt(dx*dx+dz*dz);
        if(dist<0.088f) return;
        Input3D in;
        // Spawn yaw is zero; forward is +X, strafe is +Z.
        in.forward=dx/dist;
        in.strafe=dz/dist;
        step(game,in);
    }
    std::fprintf(stderr,"Stuck trying (%.2f,%.2f) from (%.2f,%.2f), deck %d, y %.2f\n",
                 x,z,game.camera().x,game.camera().z,
                 game.currentDeck(),game.camera().y);
    assert(false && "player must be able to walk the planned level route");
}

static void geometryChecks(const World3D& w) {
    const std::vector<Vertex3D>& mesh=w.vertices();
    static_assert(sizeof(Vertex3D)==16,"PSP GU expects colour + XYZ vertex");
    assert(mesh.size()>16000);
    assert(mesh.size()%3==0);
    assert(mesh.size()<130000); // Guardrail/stair/chequer geometry budget.
    float miny=999.0f,maxy=-999.0f;
    bool green_deck=false,light_bulkhead=false;
    for(std::size_t i=0;i<mesh.size();++i) {
        const Vertex3D& v=mesh[i];
        assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));
        assert((v.color>>24)==255u);
        miny=std::fmin(miny,v.y);
        maxy=std::fmax(maxy,v.y);
        const unsigned red=v.color&255u;
        const unsigned green=(v.color>>8)&255u;
        const unsigned blue=(v.color>>16)&255u;
        if(green>red*1.6f && green>blue*1.2f) green_deck=true;
        if(red>170 && green>170 && blue>160) light_bulkhead=true;
    }
    assert(miny<0.0f && maxy>17.0f);
    assert(green_deck && light_bulkhead);
    assert(w.stairCount()==2);
    assert(w.stair(0).top_x!=w.stair(1).top_x);
    std::printf("v0.5 embossed green mesh %zu vertices / %zu triangles, Y %.1f..%.1f\n",
                mesh.size(),mesh.size()/3,miny,maxy);
}

static void deckChecks(const World3D& w) {
    for(int d=0;d<DeckCount;++d) {
        assert(std::fabs(World3D::floorHeight(d)-3.2f*d)<0.01f);
        assert(std::strlen(World3D::deckName(d))>5);
        assert(World3D::cameraDeck(World3D::floorHeight(d)+1.65f)==d);
        assert(w.canStand(-2.35f,5.00f,d)); // Service stair upper entry landing.
        assert(w.canStand(1.35f,5.00f,d));  // Service stair lower exit landing.
        assert(w.canStand(13.50f,-5.05f,d)); // Aft alternate upper landing.
        assert(w.canStand(13.50f,-1.31f,d)); // Aft alternate lower landing.
        assert(w.canStand(7.0f,4.0f,d));
        assert(!w.canStand(7.0f,0.0f,d)); // Engine void/foundation.
        assert(!w.canStand(16.0f,0.0f,d));
        assert(!w.canStand(-0.75f,8.0f,d));
        assert(!w.canStand(-0.75f,5.0f,d)); // Inclined stairway, not flat plate.
        assert(!w.canStand(13.50f,-3.00f,d));
        if(d>TankTopDeck)
            assert(!w.canStand(6.5f,1.8f,d)); // Central engine well.
    }
    assert(w.canStand(-7.35f,0.0f,EcrDeck));
    assert(!w.canStand(-7.35f,0.0f,PumpFlatDeck));
    assert(!w.canStand(-3.20f,2.50f,EcrDeck));
    assert(w.canStand(-3.20f,0.0f,EcrDeck));
    assert(!w.canStand(0.65f,-4.38f,PumpFlatDeck));
    assert(!w.canStand(1.50f,0.0f,-1));
}

static void stairSlopeChecks(const World3D& w) {
    for(int i=0;i<w.stairCount();++i) {
        const Stair3D& st=w.stair(i);
        const float run=std::sqrt((st.top_x-st.bottom_x)*(st.top_x-st.bottom_x)
                                 +(st.top_z-st.bottom_z)*(st.top_z-st.bottom_z));
        const float slopeDegrees=std::atan2(3.2f,run)*180.0f/3.141592653589793f;
        assert(slopeDegrees>=48.0f && slopeDegrees<60.0f);
        assert(st.width>=0.90f);
        for(int d=0;d<DeckCount-1;++d) {
            for(int j=0;j<=10;++j) {
                const float t=j/10.0f;
                const float x=st.top_x+(st.bottom_x-st.top_x)*t;
                const float z=st.top_z+(st.bottom_z-st.top_z)*t;
                const float expected=World3D::floorHeight(d)+(1.0f-t)*3.2f;
                float height=-1.0f;
                assert(w.walkSurface(x,z,expected,height));
                assert(std::fabs(height-expected)<0.04f);
            }
            float bad=-1.0f;
            const float midx=(st.top_x+st.bottom_x)*0.5f;
            const float midz=(st.top_z+st.bottom_z)*0.5f;
            // Cannot snap from one level to a remote suspended stair.
            assert(!w.walkSurface(midx,midz,World3D::floorHeight(d)+0.15f,bad));
            assert(w.onStair(midx,midz,World3D::floorHeight(d)+1.6f));
        }
    }
}

static void headroomChecks(const World3D& w) {
    assert(w.hasHeadroom(-7.35f,0.0f,World3D::floorHeight(EcrDeck)));
    assert(w.hasHeadroom(3.5f,4.0f,World3D::floorHeight(PumpFlatDeck)));
    assert(!w.hasHeadroom(NAN,0.0f,0.0f));
    assert(!w.hasHeadroom(1.0f,0.0f,0.0f,5.0f));
    for(int i=0;i<w.stairCount();++i) {
        const Stair3D& st=w.stair(i);
        const float t=0.70f;
        const float x=st.top_x+(st.bottom_x-st.top_x)*t;
        const float z=st.top_z+(st.bottom_z-st.top_z)*t;
        for(int deck=TankTopDeck;deck<EcrDeck;++deck) {
            const float lower=World3D::floorHeight(deck);
            const float flight=lower+(1.0f-t)*3.2f;
            assert(!w.hasHeadroom(x,z,lower)); // Under the low stair underside.
            assert(w.hasHeadroom(x,z,flight)); // On top of the same flight.
        }
    }
}

static void stationChecks(const World3D& w) {
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
    assert(game.camera().y>14.40f && game.camera().y<14.50f);
    assert(game.pitch()<0.0f);
    assert(std::strstr(game.hud().title,"ECR / UPPER"));
    assert(!game.hud().panel_visible);
    assert(world.canStand(game.camera().x,game.camera().z,game.currentDeck()));

    // Walk from ECR to upper service stair head (not via a menu/teleport).
    driveTo(game,-2.35f,0.0f);
    driveTo(game,-2.35f,5.00f);
    assert(game.currentDeck()==EcrDeck);
    for(int target=EcrDeck-1;target>=TankTopDeck;--target) {
        const float beforeY=game.camera().y;
        driveTo(game,1.35f,5.0f); // Walk down the flight from top to bottom.
        assert(game.currentDeck()==target);
        assert(std::fabs(game.camera().y-World3D::floorHeight(target)-1.65f)<0.03f);
        assert(game.camera().y<beforeY-2.5f);
        assert(!game.isClimbing());
        if(target>TankTopDeck) {
            // Use the clear side bypass to reach the next descent landing.
            driveTo(game,1.35f,3.94f);
            driveTo(game,-2.35f,3.94f);
            driveTo(game,-2.35f,5.00f);
        }
    }

    // The entire flight is reversible: walk up L0 -> L1.
    driveTo(game,-2.35f,5.00f);
    assert(game.currentDeck()==PumpFlatDeck);
    assert(std::fabs(game.camera().y-4.85f)<0.04f);
    driveTo(game,-2.35f,3.94f);
    driveTo(game,1.35f,3.94f);
    driveTo(game,1.35f,5.00f);
    driveTo(game,-2.35f,5.00f);
    assert(game.currentDeck()==MainEngineDeck);

    Input3D in;
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
    stairSlopeChecks(world);
    headroomChecks(world);
    stationChecks(world);
    gameplayChecks(world);
    std::puts("v0.5: raised deck, steel stair gradient, collisions, walkable travel and LO missions OK");
    return 0;
}
