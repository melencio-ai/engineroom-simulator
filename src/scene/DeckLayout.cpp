#include "scene/DeckLayout.hpp"

#include <algorithm>
#include <cmath>

namespace enginewatch {
namespace {

const float PI = 3.14159265358979323846f;
const float EPS = 0.00002f;

bool contains(const DeckCell& r,float x,float z) {
    return x >= r.x0-EPS && x <= r.x1+EPS &&
           z >= r.z0-EPS && z <= r.z1+EPS;
}

bool overlapInterior(float a0,float a1,float b0,float b1) {
    return a0 < b1-EPS && b0 < a1-EPS;
}

bool overlap(const DeckCell& a,const DeckCell& b) {
    return overlapInterior(a.x0,a.x1,b.x0,b.x1) &&
           overlapInterior(a.z0,a.z1,b.z0,b.z1);
}

bool overlap(const DeckCell& a,const DeckOpening& b) {
    return overlapInterior(a.x0,a.x1,b.x0,b.x1) &&
           overlapInterior(a.z0,a.z1,b.z0,b.z1);
}

bool finite(float a,float b,float c,float d) {
    return std::isfinite(a) && std::isfinite(b) &&
           std::isfinite(c) && std::isfinite(d);
}

} // namespace

void DeckPlan::cell(int level,DeckSurface kind,
                    float x0,float x1,float z0,float z1) {
    cells_.push_back(DeckCell{level,kind,x0,x1,z0,z1});
}

void DeckPlan::plate(int level,float x0,float x1,float z0,float z1) {
    cell(level,SolidSteelPlate,x0,x1,z0,z1);
}

void DeckPlan::opening(int level,OpeningPurpose kind,
                       float x0,float x1,float z0,float z1) {
    openings_.push_back(DeckOpening{level,kind,x0,x1,z0,z1});
}

void DeckPlan::gratingBand(int level,float x0,float x1,
                           float z0,float z1,int bays) {
    grating_bands_.push_back(DeckGratingBand{level,x0,x1,z0,z1,bays});
    for(int i=0;i<bays;++i) {
        const float xa=x0+(x1-x0)*i/bays;
        const float xb=x0+(x1-x0)*(i+1)/bays;
        cell(level,OpenSteelGrate,xa,xb,z0,z1);
    }
}

DeckPlan::DeckPlan() {
    // Construction stations are now defined once. Scene stair meshes, floor
    // openings, solid landing panels and collision use these same coordinates.
    stairs_[0]=Stair3D{-1.82f,5.00f,0.86f,5.00f,1.00f,false};
    stairs_[1]=Stair3D{13.50f,-4.54f,13.50f,-1.88f,1.00f,true};

    // L0: fully plated tank top under the lower ladder flights. Stair
    // footprints are collision-excluded independently of the visible plate.
    plate(TankTopDeck,-2.85f,15.30f,-6.65f,6.65f);

    for(int d=PumpFlatDeck;d<DeckCount;++d) {
        // The north service-ladder shaft has a rectangular physical cutout
        // in every working flat, with solid landing zones on both ends.
        plate(d,-2.85f,-1.82f,-6.65f,6.65f);
        plate(d,-1.82f,0.86f,-6.65f,4.34f);
        plate(d,-1.82f,0.86f,5.66f,6.65f);

        // Pump-flat centre maintenance plate is actually inset into the
        // construction, not a second checker skin placed above a solid slab.
        if(d==PumpFlatDeck) {
            plate(d,0.86f,2.10f,-6.65f,6.65f);
            plate(d,2.70f,3.10f,-6.65f,6.65f);
            plate(d,2.10f,2.70f,-6.65f,-0.70f);
            plate(d,2.10f,2.70f,0.30f,6.65f);
            cell(d,InsetServiceCover,2.10f,2.70f,-0.70f,0.30f);
        } else {
            plate(d,0.86f,3.10f,-6.65f,6.65f);
        }

        // Port and starboard gallery outer sides are solid green plate.
        // Both inner edges are actual grating (NO solid layer underneath).
        plate(d,3.10f,12.20f,-6.65f,-3.20f);
        plate(d,3.10f,12.20f,3.20f,6.65f);
        gratingBand(d,3.10f,12.20f,-3.20f,-2.38f,4);
        gratingBand(d,3.10f,12.20f,2.38f,3.20f,4);

        // Aft escape-ladder cross-shaft, also physically open on each flat.
        plate(d,12.20f,12.84f,-6.65f,6.65f);
        plate(d,12.84f,14.16f,-6.65f,-4.54f);
        plate(d,12.84f,14.16f,-1.88f,6.65f);
        plate(d,14.16f,15.30f,-6.65f,6.65f);

        opening(d,EngineMachineryWell,3.10f,12.20f,-2.38f,2.38f);
        opening(d,ForwardStairwell,-1.82f,0.86f,4.34f,5.66f);
        opening(d,AftEscapeStairwell,12.84f,14.16f,-4.54f,-1.88f);
    }

    // ECR upper floor and the guarded bridge share the EcrDeck topology.
    plate(EcrDeck,-9.80f,-3.35f,-3.94f,3.94f);
    plate(EcrDeck,-3.35f,-2.85f,-1.18f,1.18f);
}

DeckSurface DeckPlan::surfaceAt(float x,float z,int level) const {
    if(level<0 || level>=DeckCount || !std::isfinite(x) || !std::isfinite(z))
        return NoDeckSurface;
    for(std::size_t i=0;i<cells_.size();++i) {
        const DeckCell& r=cells_[i];
        if(r.level==level && contains(r,x,z))
            return r.surface;
    }
    return NoDeckSurface;
}

bool DeckPlan::insideStairLane(float x,float z,float radius) const {
    if(!std::isfinite(x) || !std::isfinite(z) ||
       !std::isfinite(radius) || radius<0.0f) return true;
    for(int i=0;i<2;++i) {
        const Stair3D& s=stairs_[i];
        // 16-cm side clearance accommodates painted stair stringers.
        // Both inclined flights currently run along one principal axis.
        const float x0=std::min(s.top_x,s.bottom_x)-
                       (std::fabs(s.top_z-s.bottom_z)>0.01f
                           ?s.width*0.5f+0.16f:0.0f);
        const float x1=std::max(s.top_x,s.bottom_x)+
                       (std::fabs(s.top_z-s.bottom_z)>0.01f
                           ?s.width*0.5f+0.16f:0.0f);
        const float z0=std::min(s.top_z,s.bottom_z)-
                       (std::fabs(s.top_x-s.bottom_x)>0.01f
                           ?s.width*0.5f+0.16f:0.0f);
        const float z1=std::max(s.top_z,s.bottom_z)+
                       (std::fabs(s.top_x-s.bottom_x)>0.01f
                           ?s.width*0.5f+0.16f:0.0f);
        if(x>x0-radius && x<x1+radius &&
           z>z0-radius && z<z1+radius) return true;
    }
    return false;
}

bool DeckPlan::supportsFootprint(float x,float z,int level,float radius) const {
    if(level<0 || level>=DeckCount || !std::isfinite(x) || !std::isfinite(z) ||
       !std::isfinite(radius) || radius<0.0f || radius>0.65f)
        return false;
    if(insideStairLane(x,z,radius))
        return false;
    if(surfaceAt(x,z,level)==NoDeckSurface)
        return false;
    // The discs cross internal slab/grating seams. Test support against the
    // UNION of tiles, never require the disc to fit inside one tile rectangle.
    // A true stair shaft/engine well remains a gap in that union.
    for(int i=0;i<16;++i) {
        const float angle=2.0f*PI*i/16.0f;
        const float cs=std::cos(angle),sn=std::sin(angle);
        if(surfaceAt(x+radius*cs,z+radius*sn,level)==NoDeckSurface)
            return false;
    }
    // Extra mid-ring samples catch narrow rectangles and unsupported corners.
    for(int i=0;i<8;++i) {
        const float angle=2.0f*PI*(i+0.5f)/8.0f;
        if(surfaceAt(x+radius*0.53f*std::cos(angle),
                     z+radius*0.53f*std::sin(angle),level)==NoDeckSurface)
            return false;
    }
    return true;
}

bool DeckPlan::validate() const {
    for(std::size_t i=0;i<cells_.size();++i) {
        const DeckCell& a=cells_[i];
        if(a.level<0 || a.level>=DeckCount || a.surface==NoDeckSurface ||
           !finite(a.x0,a.x1,a.z0,a.z1) ||
           a.x1-a.x0<0.01f || a.z1-a.z0<0.01f) return false;
        for(std::size_t j=i+1;j<cells_.size();++j) {
            const DeckCell& b=cells_[j];
            if(a.level==b.level && overlap(a,b)) return false;
        }
        for(std::size_t j=0;j<openings_.size();++j) {
            const DeckOpening& b=openings_[j];
            if(a.level==b.level && overlap(a,b)) return false;
        }
    }
    for(std::size_t i=0;i<openings_.size();++i) {
        const DeckOpening& op=openings_[i];
        if(op.level<=TankTopDeck || op.level>=DeckCount ||
           !finite(op.x0,op.x1,op.z0,op.z1) ||
           op.x1-op.x0<0.20f || op.z1-op.z0<0.20f) return false;
        const float cx=(op.x0+op.x1)/2,cz=(op.z0+op.z1)/2;
        if(surfaceAt(cx,cz,op.level)!=NoDeckSurface) return false;
    }
    for(std::size_t i=0;i<grating_bands_.size();++i) {
        const DeckGratingBand& b=grating_bands_[i];
        if(b.level<=TankTopDeck || b.level>=DeckCount ||
           b.bay_count<1 || b.bay_count>8 ||
           !finite(b.x0,b.x1,b.z0,b.z1) ||
           b.x1<=b.x0 || b.z1<=b.z0) return false;
        // Verify exact contiguous bay coverage, not merely the number of
        // grating objects: a missing 20-mm strip must fail validation.
        int count=0;
        for(int bay=0;bay<b.bay_count;++bay) {
            const float expected_x0=b.x0+(b.x1-b.x0)*bay/b.bay_count;
            const float expected_x1=b.x0+(b.x1-b.x0)*(bay+1)/b.bay_count;
            int matches=0;
            for(std::size_t j=0;j<cells_.size();++j) {
                const DeckCell& c=cells_[j];
                if(c.level!=b.level || c.surface!=OpenSteelGrate) continue;
                if(std::fabs(c.x0-expected_x0)<0.0002f &&
                   std::fabs(c.x1-expected_x1)<0.0002f &&
                   std::fabs(c.z0-b.z0)<0.0002f &&
                   std::fabs(c.z1-b.z1)<0.0002f)
                    ++matches;
            }
            if(matches!=1) return false;
            ++count;
        }
        if(count!=b.bay_count) return false;
    }
    return true;
}

} // namespace enginewatch
