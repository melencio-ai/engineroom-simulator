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
    std::fprintf(stderr,"PSP mesh budget: %zu vertices (%zu triangles), limit 129999 vertices\n",
                 mesh.size(),mesh.size()/3);
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

// Triangle-projected deck checks catch holes that collision-only tests miss.
// A walkable logical surface must have actual horizontal polygons underneath it.
static bool visibleFlatFloorAt(const World3D& w,float x,float z,float height) {
    const std::vector<Vertex3D>& vertices=w.vertices();
    for(std::size_t i=0;i+2<vertices.size();i+=3) {
        const Vertex3D& a=vertices[i],&b=vertices[i+1],&c=vertices[i+2];
        if(std::fabs(a.y-height)>0.001f ||
           std::fabs(b.y-height)>0.001f ||
           std::fabs(c.y-height)>0.001f) continue;
        const float area=(b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);
        if(std::fabs(area)<0.000001f) continue;
        const float p=(b.x-a.x)*(z-a.z)-(b.z-a.z)*(x-a.x);
        const float q=(c.x-b.x)*(z-b.z)-(c.z-b.z)*(x-b.x);
        const float t=(a.x-c.x)*(z-c.z)-(a.z-c.z)*(x-c.x);
        if((p>=-0.0001f && q>=-0.0001f && t>=-0.0001f) ||
           (p<=0.0001f && q<=0.0001f && t<=0.0001f)) return true;
    }
    return false;
}

static void physicalFloorChecks(const World3D& w) {
    // The original repeated checkerplate meshes were raised by 8 cm and
    // separated by 0.55 m gaps. Only a single inset maintenance cover remains.
    int checker_covers=0,working_grates=0;
    for(const MarinePartPlacement& p:w.parts()) {
        const Vec3& pos=p.transform.position;
        if(p.part==CheckerPlate) {
            ++checker_covers;
            assert(std::fabs(pos.x-2.40f)<0.001f);
            assert(std::fabs(pos.z+0.20f)<0.001f);
            const float cover_top=pos.y+0.035f*p.transform.scale.y;
            assert(std::fabs(cover_top-World3D::floorHeight(PumpFlatDeck))<0.001f);
        }
        if(p.part==OpenSteelGrating) {
            ++working_grates;
            // Every grate is a working-level panel, not a phantom panel
            // laid over the Level 00 steel tank top.
            const float top=pos.y+0.030f*p.transform.scale.y;
            const int level=World3D::cameraDeck(top+1.65f);
            assert(level>=PumpFlatDeck && level<=EcrDeck);
            assert(std::fabs(top-World3D::floorHeight(level))<0.001f);
            assert(std::fabs(std::fabs(pos.z)-2.79f)<0.001f);
        }
    }
    assert(checker_covers==1);
    assert(working_grates==4*2*4);

    // A continuous physical top surface exists below the main solid side
    // galleries at every deck. A rendered hole here is not a valid stairwell.
    const float samples[]={3.45f,4.65f,6.55f,8.45f,10.35f,11.65f};
    for(int d=TankTopDeck;d<DeckCount;++d) {
        const float floor=World3D::floorHeight(d);
        for(int side=-1;side<=1;side+=2)
            for(unsigned j=0;j<sizeof(samples)/sizeof(samples[0]);++j)
                assert(visibleFlatFloorAt(w,samples[j],side*4.85f,floor));
    }
    // The dedicated chequer access plate replaces (not overlaps) the one
    // 0.60 x 1.00 metre Pump Flat floor cutout.
    assert(visibleFlatFloorAt(w,2.40f,-0.20f,
                             World3D::floorHeight(PumpFlatDeck)));
}

static void screenshotGeometryChecks(const World3D& w) {
    // Screenshot-driven guard: the machinery's upper forward bays must
    // contain bulkhead geometry, not unbounded background clear-colour.
    int forward_upper_panels=0,bridge_rail_left=0,bridge_rail_right=0;
    const float top=World3D::floorHeight(EcrDeck);
    for(const Vertex3D& v : w.vertices()) {
        if(std::fabs(v.x+2.98f)<0.16f &&
           std::fabs(v.z)>4.20f && std::fabs(v.z)<6.88f &&
           v.y>top+0.40f && v.y<top+4.10f)
            ++forward_upper_panels;
        if(v.x>-3.34f && v.x<-2.83f && v.y>top+0.40f &&
           v.y<top+1.21f) {
            if(v.z<-1.08f && v.z>-1.21f) ++bridge_rail_left;
            if(v.z>1.08f && v.z<1.21f) ++bridge_rail_right;
        }
    }
    assert(forward_upper_panels>=30);
    assert(bridge_rail_left>=10 && bridge_rail_right>=10);

    // Four contiguous actual grating bays on BOTH sides of each working flat,
    // aligned above the engine well rather than decoratively floating over
    // unbroken deck slabs.
    for(int deck=PumpFlatDeck;deck<=EcrDeck;++deck) {
        const float y=World3D::floorHeight(deck);
        for(int side=-1;side<=1;side+=2) {
            for(int bay=0;bay<4;++bay) {
                const float x=4.2375f+bay*2.275f;
                float surface=-100.0f;
                assert(w.canStand(x,side*2.80f,deck));
                assert(w.walkSurface(x,side*2.80f,y,surface));
                assert(std::fabs(surface-y)<0.03f);
                int grate_mesh=0;
                for(const Vertex3D& v:w.vertices()) {
                    // Grating bars span nearly the full panel width:
                    // x half-length is 1.1375 m and z half-width 0.41 m.
                    // Count all support/bar edge vertices, not only a small
                    // central patch (the latter accidentally excluded them).
                    if(std::fabs(v.x-x)<1.15f &&
                       std::fabs(v.y-y)<0.095f &&
                       std::fabs(v.z-side*2.80f)<0.43f)
                        ++grate_mesh;
                }
                assert(grate_mesh>=40);
            }
        }
    }
}

// PSP GU has no complete X/Y clipper. Huge triangles projected beyond its
// 4096px guard band can disappear even with near-plane clipping enabled.
// These fixtures ensure the exposed hull/roof faces are actually tiled and
// that each test-view wall point is covered by physical, small triangles.
static bool coveredShellPoint(const World3D& w,int axis,float plane,
                              float u,float v,int& found,float& largest) {
    bool hit=false;
    const std::vector<Vertex3D>& verts=w.vertices();
    for(std::size_t i=0;i+2<verts.size();i+=3) {
        const Vertex3D& a=verts[i],&b=verts[i+1],&c=verts[i+2];
        const float coord[3]={a.x,a.y,a.z};
        const float coordB[3]={b.x,b.y,b.z};
        const float coordC[3]={c.x,c.y,c.z};
        if(std::fabs(coord[axis]-plane)>0.0006f ||
           std::fabs(coordB[axis]-plane)>0.0006f ||
           std::fabs(coordC[axis]-plane)>0.0006f) continue;
        const int i1=(axis+1)%3,i2=(axis+2)%3;
        const float p=(coordB[i1]-coord[i1])*(v-coord[i2])-
                      (coordB[i2]-coord[i2])*(u-coord[i1]);
        const float q=(coordC[i1]-coordB[i1])*(v-coordB[i2])-
                      (coordC[i2]-coordB[i2])*(u-coordB[i1]);
        const float t=(coord[i1]-coordC[i1])*(v-coordC[i2])-
                      (coord[i2]-coordC[i2])*(u-coordC[i1]);
        const float side1=std::fmax(std::fmax(coord[i1],coordB[i1]),coordC[i1])-
                          std::fmin(std::fmin(coord[i1],coordB[i1]),coordC[i1]);
        const float side2=std::fmax(std::fmax(coord[i2],coordB[i2]),coordC[i2])-
                          std::fmin(std::fmin(coord[i2],coordB[i2]),coordC[i2]);
        if(side1>0.03f && side2>0.03f) {
            ++found;
            // Only bound the triangle at this particular sample point.
            // Other foreground steel objects can share the same X/Y/Z
            // plane, and some have deliberately long faces unrelated to
            // the tiled hull enclosure being checked here.
            if((p>=-0.0001f && q>=-0.0001f && t>=-0.0001f) ||
               (p<=0.0001f && q<=0.0001f && t<=0.0001f)) {
                hit=true;
                largest=std::fmax(largest,std::fmax(side1,side2));
            }
        }
    }
    return hit;
}

static void guardbandShellChecks(const World3D& w) {
    // For a constant Z plane (hull), the free coordinates are X and Y.
    for(int side=-1;side<=1;side+=2) {
        const float z=side*6.75f;
        for(int ix=0;ix<5;++ix) {
            for(int iy=0;iy<4;++iy) {
                const float x=-1.5f+3.73f*ix;
                const float y=1.1f+4.8f*iy;
                int faces=0;float largest=0.0f;
                assert(coveredShellPoint(w,2,z,x,y,faces,largest));
                assert(faces>=60);
                assert(largest<2.02f &&
                       "PSP hull polygons must fit inside 2-m guardband panels");
            }
        }
    }
    // Constant X planes: free coordinates Y,Z.
    for(int ix=0;ix<2;++ix) {
        const float x=ix==0?-2.84f:15.36f;
        for(int iy=0;iy<3;++iy) {
            for(int iz=0;iz<3;++iz) {
                const float y=1.0f+4.2f*iy;
                const float z=-5.2f+4.3f*iz;
                int faces=0;float largest=0.0f;
                assert(coveredShellPoint(w,0,x,y,z,faces,largest));
                assert(faces>=45);
                assert(largest<2.02f);
            }
        }
    }
    // Constant Y overhead face: free coordinates Z, X. The extremely
    // long old roof triangles also exceeded the PSP guard band near beams.
    for(int ix=0;ix<3;++ix) {
        const float x=-1.0f+7.0f*ix;
        for(int iz=0;iz<3;++iz) {
            const float z=-5.0f+5.0f*iz;
            int faces=0;float largest=0.0f;
            assert(coveredShellPoint(w,1,16.97f,z,x,faces,largest));
            assert(faces>=80);
            assert(largest<2.02f);
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
    assert(!std::strchr(game.hud().title,'?')); // Unsupported '|' formerly rendered '?'
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

    // Regression: walk down to the real Pump Flat, face the local LO panel,
    // open it and start the main pump. This catches correct collision routes
    // AND the interaction/HUD path (not only isolated process-model tests).
    // The preceding ascent ends exactly at the L2 upper stair head.
    driveTo(game,1.35f,5.00f);
    assert(game.currentDeck()==PumpFlatDeck);
    driveTo(game,2.64f,5.00f);
    driveTo(game,2.64f,-2.81f);
    Input3D turn_to_panel;
    turn_to_panel.turn=1.0f;
    step(game,turn_to_panel,29);
    assert(world.lookAtStation(game.camera(),game.yaw())==LoPumpStation);
    Input3D use_panel;
    use_panel.interact=true;
    step(game,use_panel); // Open.
    assert(game.hud().panel_visible);
    step(game,use_panel); // Selected MAIN PUMP -> STARTING.
    assert(game.simulation().state().main_pump.requested);
    step(game,Input3D{},16);
    assert(game.simulation().state().main_pump.running);
    assert(std::strstr(game.hud().objective,"RUNNING"));
    step(game,Input3D{},200);
    assert(game.simulation().state().lo_pressure_bar>=3.0f);
    assert(std::strstr(game.hud().objective,"RETURN TO DECK 04"));

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
    physicalFloorChecks(world);
    screenshotGeometryChecks(world);
    guardbandShellChecks(world);
    headroomChecks(world);
    stationChecks(world);
    gameplayChecks(world);
    std::puts("v0.5: raised deck, steel stair gradient, collisions, walkable travel and LO missions OK");
    return 0;
}
