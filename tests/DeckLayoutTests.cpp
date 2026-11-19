#include "scene/World3D.hpp"
#include "scene/DeckLayout.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace enginewatch;

static bool coversTriangleXZ(const Vertex3D& a,const Vertex3D& b,
                             const Vertex3D& c,float x,float z) {
    const float u=(b.x-a.x)*(z-a.z)-(b.z-a.z)*(x-a.x);
    const float v=(c.x-b.x)*(z-b.z)-(c.z-b.z)*(x-b.x);
    const float w=(a.x-c.x)*(z-c.z)-(a.z-c.z)*(x-c.x);
    return (u>=-0.00001f && v>=-0.00001f && w>=-0.00001f) ||
           (u<=0.00001f && v<=0.00001f && w<=0.00001f);
}

static bool solidTopTriangleAt(const World3D& w,float x,float z,float y) {
    const std::vector<Vertex3D>& mesh=w.vertices();
    for(std::size_t i=0;i+2<mesh.size();i+=3) {
        const Vertex3D& a=mesh[i];
        const Vertex3D& b=mesh[i+1];
        const Vertex3D& c=mesh[i+2];
        if(std::fabs(a.y-y)>0.0008f ||
           std::fabs(b.y-y)>0.0008f ||
           std::fabs(c.y-y)>0.0008f) continue;
        if(coversTriangleXZ(a,b,c,x,z)) return true;
    }
    return false;
}

static const MarinePartPlacement* findPart(const World3D& w,MarinePartId id,
                                          float x,float y,float z) {
    const std::vector<MarinePartPlacement>& parts=w.parts();
    for(std::size_t i=0;i<parts.size();++i) {
        const MarinePartPlacement& p=parts[i];
        const Vec3& at=p.transform.position;
        if(p.part==id && std::fabs(at.x-x)<0.0008f &&
           std::fabs(at.y-y)<0.001f &&
           std::fabs(at.z-z)<0.0008f) return &p;
    }
    return nullptr;
}

int main() {
    World3D w;
    const DeckPlan& p=w.deckPlan();
    assert(p.validate());
    const std::vector<DeckCell>& cells=p.cells();
    const std::vector<DeckOpening>& holes=p.openings();
    const std::vector<DeckGratingBand>& bands=p.gratingBands();

    int solid=0,grate=0,cover=0;
    for(std::size_t i=0;i<cells.size();++i) {
        const DeckCell& c=cells[i];
        const float x=(c.x0+c.x1)*0.5f;
        const float z=(c.z0+c.z1)*0.5f;
        const float y=World3D::floorHeight(c.level);
        assert(p.surfaceAt(x,z,c.level)==c.surface);
        if(c.surface==SolidSteelPlate) {
            ++solid;
            assert(solidTopTriangleAt(w,x,z,y) &&
                   "Every logical steel plate must emit an actual physical top face");
        } else if(c.surface==OpenSteelGrate) {
            ++grate;
            const MarinePartPlacement* placement=findPart(w,OpenSteelGrating,
                                                          x,y-0.0216f,z);
            assert(placement && "Every grate cell must have one actual mesh");
            // The local grating spans exactly +/-0.49 in X and Z.
            const PartTransform& t=placement->transform;
            assert(std::fabs(0.98f*t.scale.x-(c.x1-c.x0))<0.0005f &&
                   "Adjacent grating panels must close without longitudinal gaps");
            assert(std::fabs(0.98f*t.scale.z-(c.z1-c.z0))<0.0005f &&
                   "A grate must cover the whole structural band width");
            assert(std::fabs(t.position.y+0.030f*t.scale.y-y)<0.0001f &&
                   "Grate metal top must be flush with adjacent solid deck");
        } else if(c.surface==InsetServiceCover) {
            ++cover;
            const MarinePartPlacement* placement=findPart(w,CheckerPlate,
                                                          x,y-0.035f,z);
            assert(placement && "The service hatch has to be physically present");
            assert(solidTopTriangleAt(w,x,z,y));
        } else {
            assert(false && "Unsupported floor kind");
        }
        // Any slab midpoint away from the stair footprint must be supported
        // by its own physical topology regardless of old hard-coded bounds.
        if(!p.insideStairLane(x,z,0.0f)) {
            assert(p.supportsFootprint(x,z,c.level,0.0f));
        }
    }
    assert(cells.size()==79);
    assert(solid==46);
    assert(grate==32);
    assert(cover==1);
    assert(bands.size()==8);
    assert(holes.size()==12);

    // The physical centre of every shaft / well remains unsupported. L0
    // instead has a true solid tank top but blocks the stair sweep.
    for(int deck=PumpFlatDeck;deck<=EcrDeck;++deck) {
        assert(p.surfaceAt(7.50f,0.0f,deck)==NoDeckSurface);
        assert(p.surfaceAt(-0.75f,5.0f,deck)==NoDeckSurface);
        assert(p.surfaceAt(13.50f,-3.0f,deck)==NoDeckSurface);
        assert(!p.supportsFootprint(7.50f,0.0f,deck,0.25f));
        assert(!p.supportsFootprint(-0.75f,5.0f,deck,0.25f));
        assert(!p.supportsFootprint(13.50f,-3.0f,deck,0.25f));
    }
    assert(p.surfaceAt(-0.75f,5.0f,TankTopDeck)==SolidSteelPlate);
    assert(!p.supportsFootprint(-0.75f,5.0f,TankTopDeck,0.25f));

    // A player must be able to walk ACROSS internal construction joints.
    // Collision cannot reject a seam because neither individual panel alone
    // is wide enough for their entire circular body footprint.
    for(int d=PumpFlatDeck;d<=EcrDeck;++d) {
        const float y=World3D::floorHeight(d);
        for(float x=3.45f;x<=11.95f;x+=0.23f) {
            for(int side=-1;side<=1;side+=2) {
                const float z=side*3.64f;
                float surface=0;
                assert(w.canStand(x,z,d,0.25f));
                assert(w.walkSurface(x,z,y,surface,0.25f));
                assert(std::fabs(surface-y)<0.002f);
            }
        }
        for(int bay=1;bay<4;++bay) {
            const float x=3.10f+2.275f*bay;
            for(int side=-1;side<=1;side+=2) {
                assert(w.canStand(x,side*2.80f,d,0.25f) &&
                       "Bay-to-bay grating seams must be continuously walkable");
                assert(w.canStand(x,side*3.20f,d,0.25f) &&
                       "Grating and plate join must not cause an invisible wall");
            }
        }
        assert(w.canStand(3.10f,4.65f,d,0.25f));
    }
    assert(w.canStand(2.10f,-0.20f,PumpFlatDeck,0.25f));
    assert(w.canStand(2.70f,-0.20f,PumpFlatDeck,0.25f));
    assert(w.canStand(-3.35f,0.0f,EcrDeck,0.25f));
    assert(!w.canStand(-3.15f,2.35f,EcrDeck));

    // Regression for shipped screenshots: the center gallery solid floor
    // physically contains its top surface when viewed from *all* levels.
    for(int d=TankTopDeck;d<=EcrDeck;++d) {
        const float y=World3D::floorHeight(d);
        for(float x=3.55f;x<11.80f;x+=0.55f) {
            assert(p.surfaceAt(x,4.85f,d)==SolidSteelPlate);
            assert(solidTopTriangleAt(w,x,4.85f,y));
        }
    }

    std::fprintf(stderr,
                 "G3 PASS: %zu deck cells (%d steel / %d grating / %d access cover); "
                 "%zu genuine openings; %zu supported grating bands\n",
                 cells.size(),solid,grate,cover,holes.size(),bands.size());
    return 0;
}
