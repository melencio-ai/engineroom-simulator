#include "scene/MarineParts.hpp"
#include "scene/World3D.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace enginewatch;

static void catalogChecks() {
    MarinePartLibrary catalog;
    assert(catalog.templateCount()==33u);
    std::size_t total_vertices=0u;
    for(int i=0;i<MarinePartCount;++i) {
        const MarinePartId id=static_cast<MarinePartId>(i);
        const std::size_t n=catalog.templateVertexCount(id);
        assert(n>0 && "Every advertised marine part must generate real triangles");
        assert(n%3==0 && "PSP draw batches are triangles");
        assert(std::strlen(catalog.partName(id))>3);
        total_vertices+=n;
    }
    assert(total_vertices<25000u);
    for(int i=0;i<MarineMaterialCount;++i) {
        const MarineFinish& finish=catalog.finish(static_cast<MarineMaterialId>(i));
        assert(std::strlen(finish.name)>5);
        assert((finish.abgr>>24)==255u);
        assert(finish.roughness>=0.0f && finish.roughness<=1.0f);
        assert(finish.metallic>=0.0f && finish.metallic<=1.0f);
    }
    assert(catalog.templateVertexCount(static_cast<MarinePartId>(-1))==0);
    assert(std::strcmp(catalog.partName(static_cast<MarinePartId>(-1)),"Invalid component")==0);

    std::vector<Vertex3D> geometry;
    assert(catalog.append(geometry,PipeSpool,PipeInsulation,
                          PartTransform(7.0f,2.0f,-4.0f,3.0f,0.8f,0.8f)));
    const std::size_t n=geometry.size();
    assert(n==catalog.templateVertexCount(PipeSpool));
    float lo=999.0f,hi=-999.0f;
    for(std::size_t i=0;i<n;++i) {
        const Vertex3D& p=geometry[i];
        lo=std::min(lo,p.x);
        hi=std::max(hi,p.x);
        assert(p.x>=5.4f && p.x<=8.6f);
        assert(std::fabs(p.y-2.0f)<0.3f);
        assert(std::fabs(p.z+4.0f)<0.3f);
    }
    assert(lo<5.60f && hi>8.40f); // x scaling is genuine

    assert(catalog.append(geometry,PipeSpool,PipeInsulation,
                          PartTransform(0,0,0,2,1,1,1.57079632679f)));
    const std::size_t rotated_end=geometry.size();
    assert(rotated_end==2*n);
    float maxz=0;
    for(std::size_t i=n;i<rotated_end;++i)
        maxz=std::max(maxz,std::fabs(geometry[i].z));
    assert(maxz>0.96f); // yaw rotates local +X to world +Z

    assert(catalog.append(geometry,HexBolt,GalvanizedSteel,
                          PartTransform(1,2,3,1,1,1,0,0,1.57079632679f)));
    assert(geometry.size()>2*n);
    const std::size_t saved=geometry.size();
    assert(!catalog.append(geometry,PipeSpool,MarineMaterialCount,PartTransform(0,0,0)));
    assert(!catalog.append(geometry,MarinePartCount,GreenEpoxy,PartTransform(0,0,0)));
    assert(!catalog.append(geometry,PipeSpool,GreenEpoxy,PartTransform(0,0,0,-1,1,1)));
    assert(!catalog.append(geometry,PipeSpool,GreenEpoxy,
                           PartTransform(0,0,0,NAN,1,1)));
    assert(geometry.size()==saved);
    std::printf("Marine part catalog: %d templates, %d finishes, %zu cached vertices\n",
                MarinePartCount,MarineMaterialCount,total_vertices);
}

static void worldAssemblyChecks() {
    World3D world;
    const std::vector<MarinePartPlacement>& parts=world.parts();
    bool used[MarinePartCount]={};
    bool finishes_used[MarineMaterialCount]={};
    assert(parts.size()>=120);
    for(std::size_t i=0;i<parts.size();++i) {
        const MarinePartPlacement& p=parts[i];
        assert(p.part>=0 && p.part<MarinePartCount);
        assert(p.finish>=0 && p.finish<MarineMaterialCount);
        assert(p.transform.scale.x>0 && p.transform.scale.y>0 && p.transform.scale.z>0);
        used[p.part]=true;
        finishes_used[p.finish]=true;
    }
    for(int id=0;id<MarinePartCount;++id) {
        if(!used[id]) std::fprintf(stderr,"Missing world instance: %s\n",
                                    world.partCatalog().partName(static_cast<MarinePartId>(id)));
        assert(used[id] && "Every marine component should be used at least once");
    }
    // World is still assembled from the original marine equipment and the
    // new reusable parts, not a showroom replacing LO interaction locations.
    assert(world.canStand(-7.35f,0.0f,EcrDeck));
    assert(world.lookAtStation(Vec3{-7.35f,14.45f,0.0f},3.1415926535f)==EcrConsoleStation);
    assert(world.lookAtStation(Vec3{2.64f,4.85f,-2.81f},3.1415926535f)==LoPumpStation);
    assert(world.vertices().size()%3==0);
    std::printf("Marine component placements: %zu; total world triangles: %zu\n",
                parts.size(),world.vertices().size()/3);
}
int main() {
    catalogChecks();
    worldAssemblyChecks();
    std::puts("Reusable marine 3D components, transforms, materials and world assembly: OK");
    return 0;
}
