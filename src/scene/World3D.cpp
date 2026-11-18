#include "scene/World3D.hpp"

#include <algorithm>
#include <cmath>

namespace enginewatch {
namespace {
const float PI = 3.14159265358979323846f;

uint32_t rgb(unsigned r, unsigned g, unsigned b) {
    return 0xff000000u | (b << 16) | (g << 8) | r;
}
uint32_t shade(uint32_t c, float amount) {
    const unsigned r = static_cast<unsigned>((c & 255u) * amount);
    const unsigned g = static_cast<unsigned>(((c >> 8) & 255u) * amount);
    const unsigned b = static_cast<unsigned>(((c >> 16) & 255u) * amount);
    return rgb(std::min(r, 255u), std::min(g, 255u), std::min(b, 255u));
}
// Composite merchant-ship coating palette: painted marine green tread plate,
// warm off-white bulkheads, grey-green structure and insulated pipework.
const uint32_t steel = rgb(135, 151, 144);
const uint32_t deck = rgb(40, 93, 67);       // green marine epoxy
const uint32_t wall = rgb(213, 220, 211);     // off-white protected steel
const uint32_t machinery = rgb(112, 129, 132);
const uint32_t engineBlue = rgb(69, 105, 116);
const uint32_t yellow = rgb(229, 180, 63);
const uint32_t red = rgb(161, 67, 54);
const uint32_t pipe = rgb(153, 162, 158);      // painted/metallic, with ID bands
const uint32_t bright = rgb(184, 202, 193);
const uint32_t green = rgb(87, 174, 110);
const uint32_t grating = rgb(65, 113, 82);
const uint32_t handrail = rgb(222, 226, 209);
const uint32_t dark = rgb(48, 67, 64);
}

World3D::World3D() {
    // ~50 degree steel stair-ladders, 3.2 m rise between adjacent platforms.
    // The flight repeats by deck with clear bypass walkways around each well.
    stairs_[0]=Stair3D{-1.82f,5.00f,0.86f,5.00f,1.00f,false};
    stairs_[1]=Stair3D{13.50f,-4.54f,13.50f,-1.88f,1.00f,true};
    build();
}

void World3D::triangle(const Vec3& a, const Vec3& b, const Vec3& c, uint32_t color) {
    vertices_.push_back(Vertex3D{color, a.x, a.y, a.z});
    vertices_.push_back(Vertex3D{color, b.x, b.y, b.z});
    vertices_.push_back(Vertex3D{color, c.x, c.y, c.z});
}
void World3D::quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, uint32_t color) {
    triangle(a, b, c, color);
    triangle(a, c, d, color);
}
void World3D::box(float x, float y, float z, float hx, float hy, float hz, uint32_t c) {
    const float x0=x-hx, x1=x+hx, y0=y-hy, y1=y+hy, z0=z-hz, z1=z+hz;
    // All six faces exist: depth testing removes obscured sections of equipment.
    quad(Vec3{x0,y1,z0}, Vec3{x1,y1,z0}, Vec3{x1,y1,z1}, Vec3{x0,y1,z1}, shade(c,1.28f));
    quad(Vec3{x0,y0,z1}, Vec3{x1,y0,z1}, Vec3{x1,y0,z0}, Vec3{x0,y0,z0}, shade(c,0.58f));
    quad(Vec3{x0,y0,z0}, Vec3{x1,y0,z0}, Vec3{x1,y1,z0}, Vec3{x0,y1,z0}, shade(c,0.80f));
    quad(Vec3{x1,y0,z1}, Vec3{x0,y0,z1}, Vec3{x0,y1,z1}, Vec3{x1,y1,z1}, shade(c,0.95f));
    quad(Vec3{x0,y0,z1}, Vec3{x0,y0,z0}, Vec3{x0,y1,z0}, Vec3{x0,y1,z1}, shade(c,0.75f));
    quad(Vec3{x1,y0,z0}, Vec3{x1,y0,z1}, Vec3{x1,y1,z1}, Vec3{x1,y1,z0}, shade(c,1.02f));
}
void World3D::cylinderX(float x0, float x1, float y, float z, float r, int n, uint32_t c) {
    for (int i=0; i<n; ++i) {
        const float a=2.0f*PI*i/n, b=2.0f*PI*(i+1)/n;
        const Vec3 p{x0,y+r*std::cos(a),z+r*std::sin(a)};
        const Vec3 q{x0,y+r*std::cos(b),z+r*std::sin(b)};
        const Vec3 u{x1,p.y,p.z}, v{x1,q.y,q.z};
        quad(p, u, v, q, shade(c,0.75f+0.42f*std::max(0.0f,std::cos(a))));
        triangle(Vec3{x0,y,z},q,p,shade(c,0.65f));
        triangle(Vec3{x1,y,z},u,v,shade(c,1.10f));
    }
}
void World3D::cylinderY(float x, float y0, float y1, float z, float r, int n, uint32_t c) {
    for (int i=0; i<n; ++i) {
        const float a=2.0f*PI*i/n, b=2.0f*PI*(i+1)/n;
        const Vec3 p{x+r*std::cos(a),y0,z+r*std::sin(a)};
        const Vec3 q{x+r*std::cos(b),y0,z+r*std::sin(b)};
        const Vec3 u{p.x,y1,p.z}, v{q.x,y1,q.z};
        quad(p,q,v,u,shade(c,0.65f+0.40f*std::max(0.0f,std::sin(a))));
        triangle(Vec3{x,y1,z},u,v,shade(c,1.18f));
    }
}
void World3D::pipeZ(float z0,float z1,float x,float y,float r,uint32_t c) {
    // Short horizontal bends are drawn as faceted 3D tubular sections.
    for (int i=0;i<10;++i) {
        const float a=2.0f*PI*i/10,b=2.0f*PI*(i+1)/10;
        const Vec3 p{x+r*std::cos(a),y+r*std::sin(a),z0};
        const Vec3 q{x+r*std::cos(b),y+r*std::sin(b),z0};
        const Vec3 u{p.x,p.y,z1},v{q.x,q.y,z1};
        quad(p,q,v,u,shade(c,0.75f+0.30f*std::max(0.0f,std::sin(a))));
    }
}

void World3D::obstacle(float x0,float x1,float z0,float z1,unsigned deck_mask) {
    obstacles_.push_back(Footprint{x0,x1,z0,z1,deck_mask});
}

float World3D::floorHeight(int deck_id) {
    const int clamped=std::max(0,std::min(DeckCount-1,deck_id));
    return 3.2f*static_cast<float>(clamped);
}
const char* World3D::deckName(int deck_id) {
    static const char* names[DeckCount]={
        "00 TANK TOP","01 PUMP FLAT","02 MAIN ENGINE","03 AUXILIARIES","04 ECR / UPPER"
    };
    return names[std::max(0,std::min(DeckCount-1,deck_id))];
}
int World3D::cameraDeck(float eye_height) {
    const int d=static_cast<int>(std::floor((eye_height-1.65f)/3.2f+0.5f));
    return std::max(0,std::min(DeckCount-1,d));
}


void World3D::checkerTread(float x,float y,float z,uint32_t color) {
    // Two diagonal raised bars, representing marine diamond/checker-pattern
    // steel tread. These cast a vertex-colour highlight without textures.
    for(int sign=-1;sign<=1;sign+=2) {
        const float cx=x+sign*0.105f,cz=z+sign*0.085f;
        quad(Vec3{cx-0.075f,y+0.014f,cz-0.052f},
             Vec3{cx-0.052f,y+0.014f,cz-0.066f},
             Vec3{cx+0.075f,y+0.014f,cz+0.052f},
             Vec3{cx+0.052f,y+0.014f,cz+0.066f},color);
    }
}
void World3D::plateSeams(float x0,float x1,float z0,float z1,float y) {
    const uint32_t seam=shade(deck,0.72f);
    for(float x=std::ceil(x0/2.0f)*2.0f;x<x1;x+=2.0f) {
        quad(Vec3{x-0.011f,y+0.004f,z0},Vec3{x+0.011f,y+0.004f,z0},
             Vec3{x+0.011f,y+0.004f,z1},Vec3{x-0.011f,y+0.004f,z1},seam);
    }
    for(float z=std::ceil(z0/2.0f)*2.0f;z<z1;z+=2.0f) {
        quad(Vec3{x0,y+0.004f,z-0.011f},Vec3{x1,y+0.004f,z-0.011f},
             Vec3{x1,y+0.004f,z+0.011f},Vec3{x0,y+0.004f,z+0.011f},seam);
    }
    for(float x=x0+0.42f;x<x1-0.22f;x+=0.74f) {
        for(float z=z0+0.42f;z<z1-0.22f;z+=0.74f) {
            checkerTread(x,y+0.007f,z,shade(deck,1.23f));
        }
    }
}
void World3D::deckSlab(float x0,float x1,float z0,float z1,float height,uint32_t color) {
    if(x1<=x0 || z1<=z0) return;
    // Marine-painted steel plate with visible thickness, 2 m panel joints
    // and small embossed tread features. No expensive full-screen texture.
    box((x0+x1)*0.5f,height-0.11f,(z0+z1)*0.5f,
        (x1-x0)*0.5f,0.11f,(z1-z0)*0.5f,color);
    plateSeams(x0,x1,z0,z1,height);
}
void World3D::railing(float x0,float x1,float z,float height,uint32_t color) {
    if(x1<=x0) return;
    cylinderX(x0,x1,height+1.06f,z,0.036f,6,color);
    cylinderX(x0,x1,height+0.54f,z,0.030f,6,color);
    for(float x=x0;x<=x1+0.01f;x+=1.88f)
        box(x,height+0.55f,z,0.032f,0.55f,0.035f,color);
    box(x1,height+0.55f,z,0.035f,0.55f,0.035f,color);
    // Low toe board at an exposed engine-side platform edge.
    box((x0+x1)*0.5f,height+0.10f,z,0.5f*(x1-x0),0.10f,0.027f,shade(deck,1.20f));
}
void World3D::lineBar(const Vec3& a,const Vec3& b,float half_width,uint32_t color) {
    const float dx=b.x-a.x,dz=b.z-a.z;
    const float horizontal=std::sqrt(dx*dx+dz*dz);
    if(horizontal<0.01f) return;
    const float nx=-dz/horizontal,nz=dx/horizontal,h=half_width;
    const Vec3 a0{a.x+nx*h,a.y-h,a.z+nz*h};
    const Vec3 a1{a.x-nx*h,a.y-h,a.z-nz*h};
    const Vec3 a2{a.x-nx*h,a.y+h,a.z-nz*h};
    const Vec3 a3{a.x+nx*h,a.y+h,a.z+nz*h};
    const Vec3 b0{b.x+nx*h,b.y-h,b.z+nz*h};
    const Vec3 b1{b.x-nx*h,b.y-h,b.z-nz*h};
    const Vec3 b2{b.x-nx*h,b.y+h,b.z-nz*h};
    const Vec3 b3{b.x+nx*h,b.y+h,b.z+nz*h};
    quad(a0,b0,b1,a1,shade(color,0.70f));
    quad(a3,a2,b2,b3,shade(color,1.22f));
    quad(a0,a3,b3,b0,color);
    quad(a1,b1,b2,a2,shade(color,0.92f));
    quad(a0,a1,a2,a3,color);
    quad(b0,b3,b2,b1,color);
}
void World3D::stairGeometry(const Stair3D& st) {
    const float dx=st.top_x-st.bottom_x;
    const float dz=st.top_z-st.bottom_z;
    const float run=std::sqrt(dx*dx+dz*dz);
    const float nx=-dz/run,nz=dx/run;
    const bool along_x=std::fabs(dx)>std::fabs(dz);
    const float width=st.width;
    for(int d=0;d<DeckCount-1;++d) {
        const float base=floorHeight(d);
        // A 50-degree flight with 16 individual chequered steel treads.
        for(int j=0;j<16;++j) {
            const float t=(j+0.5f)/16.0f;
            const float x=st.bottom_x+dx*t,z=st.bottom_z+dz*t;
            const float y=base+3.2f*t;
            const float hx=along_x?run/(16.0f*2.0f)*0.93f:width*0.49f;
            const float hz=along_x?width*0.49f:run/(16.0f*2.0f)*0.93f;
            box(x,y-0.025f,z,hx,0.035f,hz,shade(deck,1.07f));
            box(x,y+0.017f,z,hx*0.92f,0.012f,hz*0.92f,shade(grating,1.14f));
            // Bright metal anti-slip nosing on the tread front edge.
            if(along_x)
                box(x+dx/std::fabs(dx)*hx*0.82f,y+0.036f,z,
                    0.018f,0.014f,hz*0.91f,shade(bright,0.85f));
            else
                box(x,y+0.036f,z+dz/std::fabs(dz)*hz*0.82f,
                    hx*0.91f,0.014f,0.018f,shade(bright,0.85f));
        }
        const Vec3 basePoint{st.bottom_x,base,st.bottom_z};
        const Vec3 topPoint{st.top_x,base+3.2f,st.top_z};
        for(int side=-1;side<=1;side+=2) {
            const float offset=side*(width*0.5f+0.05f);
            const Vec3 lo{basePoint.x+nx*offset,basePoint.y,basePoint.z+nz*offset};
            const Vec3 hi{topPoint.x+nx*offset,topPoint.y,topPoint.z+nz*offset};
            lineBar(Vec3{lo.x,lo.y-0.17f,lo.z},
                    Vec3{hi.x,hi.y-0.17f,hi.z},0.070f,steel);
            // Pale-painted two-rail handrail system.
            lineBar(Vec3{lo.x,lo.y+1.00f,lo.z},
                    Vec3{hi.x,hi.y+1.00f,hi.z},0.029f,handrail);
            lineBar(Vec3{lo.x,lo.y+0.52f,lo.z},
                    Vec3{hi.x,hi.y+0.52f,hi.z},0.023f,handrail);
            for(int j=0;j<=4;++j) {
                const float t=j/4.0f;
                const float x=lo.x+(hi.x-lo.x)*t,y=lo.y+3.2f*t;
                const float z=lo.z+(hi.z-lo.z)*t;
                box(x,y+0.55f,z,0.030f,0.55f,0.032f,handrail);
            }
        }
        // Shielded underside (the upper face is intentionally stepped).
        const Vec3 la{st.bottom_x+nx*width*0.5f,base-0.20f,
                      st.bottom_z+nz*width*0.5f};
        const Vec3 lb{st.bottom_x-nx*width*0.5f,base-0.20f,
                      st.bottom_z-nz*width*0.5f};
        const Vec3 ta{st.top_x+nx*width*0.5f,base+3.0f,
                      st.top_z+nz*width*0.5f};
        const Vec3 tb{st.top_x-nx*width*0.5f,base+3.0f,
                      st.top_z-nz*width*0.5f};
        quad(la,ta,tb,lb,shade(steel,0.70f));
    }
    for(int d=0;d<DeckCount;++d) {
        const float y=floorHeight(d);
        // Tread edge hazard markers: avoid continuously yellow floor plates.
        for(int edge=0;edge<2;++edge) {
            const float x=edge==0?st.top_x:st.bottom_x;
            const float z=edge==0?st.top_z:st.bottom_z;
            if((edge==0 && d==0) || (edge==1 && d==EcrDeck)) continue;
            if(along_x)
                box(x,y+0.028f,z,0.045f,0.028f,width*0.52f,yellow);
            else
                box(x,y+0.028f,z,width*0.52f,0.028f,0.045f,yellow);
        }
    }
}

void World3D::part(MarinePartId type,MarineMaterialId finish,
                   const PartTransform& position) {
    if(parts_library_.append(vertices_,type,finish,position))
        parts_.push_back(MarinePartPlacement(type,finish,position));
}

void World3D::build() {
    vertices_.reserve(85000);
    const unsigned all=(1u<<DeckCount)-1u;
    // No sealed stacked rooms: the central main-engine well is open through
    // four partial working flats. Below is a continuous tank top with engine seating.
    deckSlab(-2.85f,15.30f,-6.65f,6.65f,0.0f,deck);
    for(int d=1;d<DeckCount;++d) {
        const float y=floorHeight(d);
        const uint32_t floor_col=(d%2==0)?shade(deck,1.08f):deck;
        // Forward 50-degree service stair, recessed into an open partial deck.
        deckSlab(-2.85f,-1.82f,-6.65f,6.65f,y,floor_col);
        deckSlab(-1.82f,0.86f,-6.65f,4.34f,y,floor_col);
        deckSlab(-1.82f,0.86f,5.66f,6.65f,y,floor_col);
        deckSlab(0.86f,3.10f,-6.65f,6.65f,y,floor_col);
        // Two long catwalks. The omitted center is a vertical machinery well.
        deckSlab(3.10f,12.20f,-6.65f,-2.38f,y,floor_col);
        deckSlab(3.10f,12.20f,2.38f,6.65f,y,floor_col);
        // Aft escape stair runs athwartships (along Z) to preserve deck access.
        deckSlab(12.20f,12.84f,-6.65f,6.65f,y,floor_col);
        deckSlab(12.84f,14.16f,-6.65f,-4.54f,y,floor_col);
        deckSlab(12.84f,14.16f,-1.88f,6.65f,y,floor_col);
        deckSlab(14.16f,15.30f,-6.65f,6.65f,y,floor_col);

        // Readable deck grating and alternating stripe rhythm, without textures.
        for(float x=3.40f;x<12.06f;x+=1.10f) {
            for(int side=-1;side<=1;side+=2)
                box(x,y+0.019f,side*4.53f,0.026f,0.016f,2.05f,shade(bright,0.51f));
        }
        for(int side=-1;side<=1;side+=2) {
            for(float z=2.86f;z<6.56f;z+=1.07f)
                box(7.66f,y+0.021f,side*z,4.41f,0.017f,0.020f,shade(bright,0.49f));
            railing(3.16f,12.14f,side*2.38f,y,handrail);
        }
        // Painted green checker plate is bordered by open side grating around
        // machinery. Horizontal grating strips remain lighter and readable.
        for(float x=3.45f;x<12.0f;x+=0.52f) {
            for(int side=-1;side<=1;side+=2)
                box(x,y+0.025f,side*2.76f,0.021f,0.020f,0.32f,shade(grating,1.05f));
        }
        // Structural lighting bands and bright exit-route marks.
        box(-2.65f,y+2.27f,-6.44f,0.07f,0.08f,0.31f,
            d==PumpFlatDeck?green:d==MainEngineDeck?yellow:d==AuxiliaryDeck?red:pipe);
        box(-2.65f,y+2.27f,6.44f,0.07f,0.08f,0.31f,bright);
    }

    // Upper ECR lives outside the engine well, overlooking the main engine.
    const float top=floorHeight(EcrDeck);
    deckSlab(-9.80f,-3.35f,-3.94f,3.94f,top,shade(deck,1.15f));
    deckSlab(-3.35f,-2.85f,-1.18f,1.18f,top,shade(deck,1.15f));
    box(-6.55f,top+4.10f,0.0f,3.45f,0.18f,4.05f,dark);
    box(-10.02f,top+1.98f,0.0f,0.16f,2.02f,4.06f,wall);
    box(-6.55f,top+1.98f,-4.02f,3.45f,2.02f,0.15f,wall);
    box(-6.55f,top+1.98f,4.02f,3.45f,2.02f,0.15f,wall);
    // ECR-front bulkhead with two clear look-through windows and central door.
    for(int side=-1;side<=1;side+=2) {
        const float z=side*2.65f;
        box(-3.38f,top+0.30f,z,0.12f,0.30f,1.34f,wall);
        box(-3.38f,top+3.62f,z,0.12f,0.38f,1.34f,wall);
        box(-3.38f,top+2.05f,side*1.32f,0.12f,1.76f,0.08f,steel);
        box(-3.38f,top+2.05f,side*3.98f,0.12f,1.76f,0.08f,steel);
        box(-3.35f,top+2.55f,z,0.07f,0.065f,1.28f,bright);
    }
    box(-3.38f,top+3.60f,0.0f,0.15f,0.40f,1.20f,wall);
    box(-3.36f,top+1.10f,-1.22f,0.07f,1.10f,0.075f,yellow);
    box(-3.36f,top+1.10f,1.22f,0.07f,1.10f,0.075f,yellow);
    // ECR control wall behind player on spawn; turned-to using proximity.
    box(-9.41f,top+0.94f,0.0f,0.34f,0.92f,2.30f,steel);
    box(-9.12f,top+2.12f,0.0f,0.13f,0.72f,2.25f,dark);
    for(int i=0;i<5;++i) {
        const float z=-1.68f+0.84f*i;
        box(-8.95f,top+2.23f,z,0.03f,0.30f,0.32f,engineBlue);
        box(-8.91f,top+2.36f,z,0.018f,0.065f,0.15f,i<2?green:yellow);
    }
    for(int i=0;i<3;++i)
        box(-8.25f+1.70f*i,top+3.85f,0.0f,0.56f,0.034f,0.34f,rgb(171,195,188));
    obstacle(-9.75f,-8.70f,-2.35f,2.35f,1u<<EcrDeck);

    // Hull, stern bulkhead, forward machinery boundary, roof beams and framing.
    box(6.24f,8.48f,-6.91f,9.28f,8.53f,0.16f,wall);
    box(6.24f,8.48f, 6.91f,9.28f,8.53f,0.16f,wall);
    box(15.53f,8.48f,0.0f,0.17f,8.53f,6.88f,wall);
    box(-2.98f,6.35f,0.0f,0.14f,6.35f,6.86f,wall);
    box(6.24f,17.14f,0.0f,9.28f,0.17f,6.92f,dark);
    for(float x=-2.0f;x<15.0f;x+=3.0f) {
        box(x,16.85f,0.0f,0.13f,0.30f,6.75f,steel);
        for(int side=-1;side<=1;side+=2)
            box(x,8.40f,side*6.66f,0.16f,8.45f,0.12f,steel);
    }
    for(int i=0;i<5;++i) {
        const float y=floorHeight(i);
        for(int side=-1;side<=1;side+=2) {
            box(6.0f,y+2.94f,side*6.34f,8.85f,0.13f,0.12f,steel);
            box(3.0f+i*2.15f,y+2.80f,side*6.38f,0.52f,0.030f,0.10f,bright);
        }
    }

    // Longitudinal stiffeners and small welded gusset details express actual
    // ship structural continuity rather than unbroken industrial box walls.
    for(float x=-1.35f;x<15.05f;x+=2.60f) {
        for(int side=-1;side<=1;side+=2) {
            const float z=side*6.73f;
            box(x,8.15f,z,0.080f,8.22f,0.070f,shade(wall,0.82f));
            for(int d=1;d<DeckCount;++d) {
                const float y=floorHeight(d);
                triangle(Vec3{x-0.36f,y+0.11f,z-side*0.02f},
                         Vec3{x+0.36f,y+0.11f,z-side*0.02f},
                         Vec3{x,y+0.55f,z-side*0.02f},shade(steel,0.96f));
            }
        }
    }
    // Luminaires suspended below deck beams: work areas stay visually bright.
    for(int d=0;d<DeckCount;++d) {
        const float y=floorHeight(d)+2.72f;
        for(int side=-1;side<=1;side+=2) {
            for(float x=0.5f;x<14.0f;x+=4.2f) {
                box(x,y,side*5.74f,0.65f,0.045f,0.18f,bright);
                box(x,y-0.05f,side*5.74f,0.56f,0.016f,0.13f,
                    rgb(235,238,211));
            }
        }
    }

    // One slow-speed six-cylinder main engine from foundation to top platform.
    box(7.70f,0.43f,0.0f,4.25f,0.43f,1.88f,dark);
    box(7.60f,2.62f,0.0f,3.84f,1.79f,1.62f,engineBlue);
    box(7.60f,5.72f,0.0f,3.63f,1.35f,1.49f,shade(engineBlue,1.06f));
    box(7.60f,8.43f,0.0f,3.55f,1.42f,1.45f,machinery);
    box(7.60f,10.82f,0.0f,3.53f,0.75f,1.35f,steel);
    for(int i=0;i<6;++i) {
        const float x=4.70f+i*1.14f;
        box(x,11.76f,0.0f,0.48f,0.48f,1.14f,engineBlue);
        box(x,12.27f,0.0f,0.51f,0.13f,1.13f,bright);
        cylinderY(x,12.41f,13.25f,0.0f,0.24f,10,red);
        box(x,4.61f,-1.62f,0.35f,0.20f,0.10f,steel);
        box(x,7.35f,-1.58f,0.33f,0.25f,0.09f,bright);
        box(x,9.20f,1.59f,0.32f,0.29f,0.10f,pipe);
    }
    // Pipe bodies are mostly metallic or insulated; identification appears as
    // service-colour bands (orange LO, blue freshwater, red heated exhaust).
    // Their exact markings are illustrative, not vessel-specific ISO mapping.
    const uint32_t loBand=rgb(209,128,62);
    const uint32_t coolingBand=rgb(69,134,177);
    const uint32_t exhaustBand=rgb(169,66,54);
    cylinderX(4.25f,11.62f,13.62f,-0.87f,0.29f,10,bright);
    cylinderX(4.16f,11.60f,8.15f,1.66f,0.13f,10,pipe);
    cylinderX(4.03f,11.92f,2.03f,1.75f,0.13f,10,pipe);
    for(int i=0;i<6;++i) {
        const float x=4.80f+1.09f*i;
        // Short flange collars and inspection/flow identification bands.
        cylinderX(x,x+0.08f,13.62f,-0.87f,0.32f,10,exhaustBand);
        cylinderX(x,x+0.07f,8.15f,1.66f,0.16f,8,coolingBand);
        cylinderX(x,x+0.07f,2.03f,1.75f,0.16f,8,loBand);
        for(int side=-1;side<=1;side+=2)
            box(x,0.22f,side*1.89f,0.10f,0.14f,0.13f,steel);
    }
    cylinderX(11.73f,12.15f,2.28f,0.0f,1.22f,16,steel);
    cylinderX(12.16f,12.29f,2.28f,0.0f,0.31f,12,yellow);
    cylinderX(11.16f,12.35f,11.42f,-0.45f,0.76f,14,steel);
    cylinderX(12.36f,12.71f,11.42f,-0.45f,0.30f,12,red);
    for(int i=0;i<6;++i) {
        const float x=4.70f+1.14f*i;
        box(x,13.36f,-0.52f,0.085f,0.22f,0.42f,red);
    }
    obstacle(3.42f,12.35f,-2.12f,2.12f,all);

    // L3 auxiliary machinery: two generators, compressor bank, purifier/ workshop.
    const float ay=floorHeight(AuxiliaryDeck);
    for(int gen=0;gen<2;++gen) {
        // Keep the upper service stair landings clear; gensets use the
        // opposite auxiliary-side bays rather than block the stair throat.
        const float z=gen==0 ? -4.58f : 1.12f;
        box(1.52f,ay+0.20f,z,1.23f,0.20f,0.94f,dark);
        box(0.82f,ay+0.94f,z,0.76f,0.61f,0.67f,engineBlue);
        for(int i=0;i<3;++i)
            box(0.35f+i*0.44f,ay+1.60f,z,0.15f,0.14f,0.58f,machinery);
        cylinderX(1.36f,2.52f,ay+1.06f,z,0.62f,12,steel);
        cylinderX(2.53f,2.68f,ay+1.06f,z,0.32f,10,yellow);
        cylinderX(0.35f,1.10f,ay+2.08f,z+(gen==0?-0.40f:0.40f),0.10f,8,pipe);
        box(0.42f,ay+1.29f,z+(gen==0?1.03f:-1.03f),0.14f,0.31f,0.10f,green);
        obstacle(0.10f,2.90f,z-1.03f,z+1.03f,1u<<AuxiliaryDeck);
    }
    for(int i=0;i<3;++i)
        cylinderY(10.50f+i*0.68f,ay+0.20f,ay+1.95f,-4.86f,0.28f,10,steel);
    box(10.65f,ay+0.48f,-5.75f,1.12f,0.47f,0.39f,dark);
    obstacle(9.60f,11.77f,-5.25f,-4.40f,1u<<AuxiliaryDeck);
    box(13.94f,ay+0.91f,4.70f,0.39f,0.91f,1.25f,machinery);

    // L2 engine side service boards and cylinder maintenance walkways.
    const float my=floorHeight(MainEngineDeck);
    for(int side=-1;side<=1;side+=2) {
        for(int i=0;i<3;++i) {
            const float x=4.73f+i*2.48f,z=side*3.08f;
            box(x,my+1.30f,z,0.35f,0.58f,0.15f,steel);
            box(x,my+1.50f,z-side*0.17f,0.16f,0.08f,0.07f,green);
        }
    }

    // L1 pump flat: maintain the existing LO mission interaction points.
    const float py=floorHeight(PumpFlatDeck);
    for(int i=0;i<2;++i) {
        const float z=i==0?-4.38f:2.75f;
        box(0.55f,py+0.21f,z,0.87f,0.21f,0.68f,dark);
        cylinderX(-0.12f,0.92f,py+0.91f,z,0.45f,12,pipe);
        box(0.90f,py+0.92f,z,0.52f,0.41f,0.38f,machinery);
        box(0.90f,py+1.39f,z,0.43f,0.055f,0.31f,yellow);
        cylinderY(1.25f,py+0.96f,py+1.17f,z,0.22f,10,red);
        cylinderX(-0.40f,1.30f,py+1.20f,z+0.56f,0.09f,10,pipe);
        obstacle(-0.45f,1.58f,z-0.78f,z+0.78f,1u<<PumpFlatDeck);
    }
    box(1.88f,py+1.17f,-2.83f,0.17f,0.67f,0.50f,steel);
    box(1.67f,py+1.32f,-2.83f,0.033f,0.42f,0.40f,dark);
    box(1.62f,py+1.44f,-2.98f,0.024f,0.07f,0.085f,green);
    box(1.62f,py+1.44f,-2.67f,0.024f,0.07f,0.085f,yellow);
    obstacle(1.67f,2.14f,-3.32f,-2.30f,1u<<PumpFlatDeck);
    cylinderY(1.68f,py+0.21f,py+1.61f,-5.87f,0.28f,12,steel);
    box(1.68f,py+1.62f,-5.87f,0.36f,0.11f,0.36f,red);
    cylinderX(0.56f,2.51f,py+0.63f,-5.87f,0.09f,10,pipe);
    obstacle(1.20f,2.11f,-6.24f,-5.49f,1u<<PumpFlatDeck);

    // Local pressure gauges, pump labels and bolted machinery stools.
    for(int i=0;i<2;++i) {
        const float z=i==0?-4.38f:2.75f;
        cylinderY(0.32f,py+1.21f,py+1.57f,z,0.045f,8,steel);
        cylinderX(0.20f,0.30f,py+1.62f,z,0.20f,10,wall);
        box(0.14f,py+1.66f,z,0.024f,0.035f,0.08f,dark);
        for(int side=-1;side<=1;side+=2)
            box(0.10f,py+0.08f,z+side*0.56f,0.08f,0.08f,0.08f,steel);
    }

    // L0 tank top, bilge well indicators and lower shaft direction.
    for(int side=-1;side<=1;side+=2) {
        for(int i=0;i<3;++i) {
            const float x=-1.12f+1.20f*i,z=side*5.64f;
            box(x,0.035f,z,0.43f,0.035f,0.31f,shade(dark,0.70f));
            box(x,0.079f,z,0.31f,0.022f,0.23f,red);
        }
    }
    box(13.35f,1.12f,0.0f,1.18f,0.76f,0.95f,steel);
    obstacle(12.18f,14.56f,-0.95f,0.95f,1u<<TankTopDeck);

    // Long vertical pipe runs connect all floors and emphasize the open height.
    for(int side=-1;side<=1;side+=2) {
        const float z=side*6.15f;
        for(int i=0;i<3;++i) {
            const float x=4.0f+i*4.00f;
            cylinderY(x,0.30f,16.30f,z,0.11f,8,i==0?red:i==1?pipe:steel);
        }
        cylinderX(-2.20f,13.80f,16.10f,side*5.95f,0.13f,8,pipe);
    }
    // Inclined, physically walkable steel stair flights with handrails and
    // anti-slip treads, repeating between all deck levels.

    stairGeometry(stairs_[0]);
    stairGeometry(stairs_[1]);

    // MARINE COMPONENT CATALOG: these are individually identified, reusable
    // world objects, not ad-hoc modelling calls. Each part is a cached local
    // template plus a finish and ship/world transform. See MarineParts.hpp.
    // Placement is illustrative and does not claim an approved vessel GA.
    part(SteelPlate,GreenEpoxy,PartTransform(0.1f,0.035f,-2.9f,1.8f,1.0f,1.1f));
    for(int d=0;d<DeckCount;++d) {
        const float y=floorHeight(d);
        // Removable embossed deck panels and open grating at the engine well.
        for(int i=0;i<4;++i) {
            const float x=4.05f+i*2.3f;
            for(int side=-1;side<=1;side+=2) {
                part(CheckerPlate,GreenEpoxy,
                     PartTransform(x,y+0.060f,side*4.70f,1.75f,0.60f,1.05f));
                part(OpenSteelGrating,GreenGrating,
                     PartTransform(x,y+0.057f,side*2.91f,1.06f,0.72f,0.54f));
            }
        }
        // Structural beams live above the walking envelope, not on the decks.
        for(int side=-1;side<=1;side+=2) {
            part(IBeam,StructuralSteelGrey,
                 PartTransform(6.8f,y+2.85f,side*6.48f,12.0f,0.65f,0.70f));
            part(ChannelSection,StructuralSteelGrey,
                 PartTransform(3.56f,y+2.77f,side*6.46f,1.8f,0.46f,0.7f,1.57f));
            part(GussetBracket,OffWhiteCoatedSteel,
                 PartTransform(3.44f,y+2.42f,side*6.55f,0.50f,0.55f,0.45f));
            part(WorkLightFixture,WorkLightWhite,
                 PartTransform(7.48f,y+2.69f,side*5.72f,1.50f,0.70f,1.0f));
        }
        // The stair treads are additionally stamped from the standardized
        // checker-tread template, including contrasting front/nosing geometry.
        if(d<EcrDeck) {
            for(int j=1;j<15;j+=4) {
                const float t=(j+0.5f)/16.0f;
                const float sx=stairs_[0].bottom_x+
                               (stairs_[0].top_x-stairs_[0].bottom_x)*t;
                const float sz=stairs_[1].bottom_z+
                               (stairs_[1].top_z-stairs_[1].bottom_z)*t;
                part(StairTread,GreenEpoxy,
                     PartTransform(sx,y+3.2f*t+0.025f,5.0f,
                                   0.17f,0.52f,1.04f));
                part(StairTread,GreenEpoxy,
                     PartTransform(13.50f,y+3.2f*t+0.025f,sz,
                                   1.04f,0.52f,0.17f,1.570796327f));
            }
        }
    }

    // Ship-stiffener profiles, angle-bar reinforcement and removable plates.
    part(AngleSection,StructuralSteelGrey,
         PartTransform(6.6f,16.42f,-6.65f,12.5f,0.90f,0.65f));
    part(BulbFlat,StructuralSteelGrey,
         PartTransform(6.7f,16.34f,6.62f,13.0f,0.73f,1.0f));
    part(FlatBar,GalvanizedSteel,
         PartTransform(8.1f,15.98f,5.97f,5.2f,0.50f,0.70f));
    part(HandrailSpan,OffWhiteCoatedSteel,
         PartTransform(2.20f,top+1.05f,-3.42f,1.8f,1.2f,1.0f));
    part(HandrailPost,OffWhiteCoatedSteel,
         PartTransform(2.88f,top+0.54f,3.33f,0.8f,1.04f,0.80f));
    part(AccessCover,GreenEpoxy,
         PartTransform(-1.35f,0.070f,-4.38f,0.85f,0.60f,0.85f));
    part(AccessCover,GreenEpoxy,
         PartTransform(2.20f,0.070f,4.05f,1.2f,0.60f,0.85f));
    part(CableTray,GalvanizedSteel,
         PartTransform(7.5f,13.72f,-6.55f,9.0f,0.55f,0.55f));
    part(CableTray,GalvanizedSteel,
         PartTransform(8.2f,10.60f,6.55f,7.5f,0.55f,0.55f));

    // A fabricated LO skid with matching pipe spools, flange/collar hardware,
    // hand-operated valves and differential pressure instrumentation.
    const float ly=floorHeight(PumpFlatDeck)+1.18f;
    part(PipeSpool,PipeInsulation,
         PartTransform(1.10f,ly,-6.06f,3.60f,1.10f,1.10f));
    part(PipeSpool,PipeInsulation,
         PartTransform(1.60f,ly,5.95f,3.0f,0.88f,0.88f));
    part(PipeElbow90,PipeInsulation,
         PartTransform(3.03f,ly,-6.06f,0.90f,1.10f,1.10f));
    part(PipeTee,PipeInsulation,
         PartTransform(2.94f,ly,5.95f,0.76f,0.90f,0.90f));
    part(ConcentricReducer,PipeInsulation,
         PartTransform(-0.35f,ly,-6.06f,0.66f,1.12f,1.12f));
    for(int i=0;i<3;++i) {
        const float x=0.10f+1.04f*i;
        part(WeldNeckFlange,GalvanizedSteel,
             PartTransform(x,ly,-6.06f,0.35f,0.65f,0.65f));
        part(PipeGasket,RubberGasket,
             PartTransform(x+0.09f,ly,-6.06f,0.30f,0.68f,0.68f));
        for(int angle=0;angle<4;++angle) {
            const float a=1.570796327f*angle;
            part(HexBolt,GalvanizedSteel,
                 PartTransform(x,ly+0.25f*std::cos(a),
                               -6.06f+0.25f*std::sin(a),
                               0.20f,0.25f,0.20f,0,0,1.570796327f));
        }
    }
    part(GateValve,MachineryBlue,PartTransform(1.53f,ly,-6.06f,0.66f,0.66f,0.66f));
    part(GlobeValve,MachineryBlue,PartTransform(1.30f,ly,5.95f,0.63f,0.63f,0.63f));
    part(ButterflyValve,CastIron,PartTransform(2.62f,ly,5.95f,0.72f,0.62f,0.62f));
    part(YStrainer,CastIron,PartTransform(2.35f,ly-0.24f,-5.70f,0.58f,0.58f,0.58f));
    part(BlindFlange,GalvanizedSteel,
         PartTransform(3.50f,ly,-6.06f,0.28f,0.66f,0.66f));
    part(PressureGauge,BronzeInstrument,
         PartTransform(2.30f,ly+0.75f,-5.72f,0.48f,0.44f,0.44f));
    part(PipeSupport,StructuralSteelGrey,
         PartTransform(2.45f,ly-0.70f,-6.00f,0.55f,0.68f,0.78f));
    part(PipeClamp,GalvanizedSteel,
         PartTransform(0.73f,ly,-6.05f,0.44f,0.42f,0.44f));
    part(Ubolt,GalvanizedSteel,
         PartTransform(0.73f,ly+0.37f,-6.05f,0.44f,0.55f,0.44f));
    part(HexNut,GalvanizedSteel,
         PartTransform(0.73f,ly+0.58f,-5.89f,0.22f,0.22f,0.22f));
    part(FlatWasher,GalvanizedSteel,
         PartTransform(0.73f,ly+0.44f,-5.89f,0.22f,0.20f,0.22f));

    // A separate cooling/header manifold with repeated neutral pipe surfaces
    // and narrower blue identification bands as finish-specific objects.
    for(int i=0;i<3;++i) {
        const float x=4.55f+i*2.35f;
        part(PipeSpool,PipeInsulation,
             PartTransform(x,8.15f,1.78f,2.12f,0.78f,0.78f));
        part(WeldNeckFlange,GalvanizedSteel,
             PartTransform(x+1.0f,8.15f,1.78f,0.22f,0.42f,0.42f));
        part(PipeSpool,CoolingWaterBlue,
             PartTransform(x+0.73f,8.15f,1.78f,0.09f,0.82f,0.82f));
        part(PipeSpool,LubricatingOilOrange,
             PartTransform(x+0.73f,2.03f,1.78f,0.09f,0.82f,0.82f));
        part(PipeSpool,FireSystemRed,
             PartTransform(x+0.73f,13.62f,-0.94f,0.09f,0.82f,0.82f));
    }

    // Bolted engine seating: repeating fasteners are individual reusable
    // components, not texturing tricks embedded in the bedplate mesh.
    for(int i=0;i<6;++i) {
        const float x=4.75f+i*1.13f;
        for(int side=-1;side<=1;side+=2) {
            const float z=side*1.79f;
            part(HexBolt,GalvanizedSteel,
                 PartTransform(x,0.20f,z,0.36f,0.50f,0.36f));
            part(FlatWasher,GalvanizedSteel,
                 PartTransform(x,0.12f,z,0.45f,0.46f,0.45f));
        }
    }

}

bool World3D::canStand(float x,float z,int deck_id,float radius) const {
    if(deck_id<0 || deck_id>=DeckCount || radius<0.0f || radius>0.65f) return false;
    if(!std::isfinite(x) || !std::isfinite(z)) return false;
    const float az=std::fabs(z);
    if(deck_id==EcrDeck && x< -2.50f) {
        if(x< -9.75f+radius || az>3.93f-radius) return false;
        // Only the centre ECR doorway connects to the machinery upper platform.
        if(x> -3.54f-radius && az>1.12f-radius) return false;
        if(x> -2.81f-radius && az>1.12f-radius) return false;
    } else {
        if(x< -2.84f+radius || az>6.64f-radius) return false;
    }
    if(x>15.29f-radius) return false;
    // The engine is not a floor: retain port/starboard catwalk openings at L1-L4.
    if(deck_id>TankTopDeck && x>3.10f-radius && x<12.20f+radius &&
       az<2.38f+radius) return false;
    // Floor plates are cut away at each inclined flight. Unlike a simple
    // ladder trigger, a player standing on the slope resolves their height
    // from walkSurface(), not from a fake platform in the stair opening.
    if(x>-1.82f-radius && x<0.86f+radius &&
       z>4.34f-radius && z<5.66f+radius) return false;
    if(x>12.84f-radius && x<14.16f+radius &&
       z>-4.54f-radius && z<-1.88f+radius) return false;
    for(std::size_t i=0;i<obstacles_.size();++i) {
        const Footprint& b=obstacles_[i];
        if(!(b.deck_mask & (1u<<deck_id))) continue;
        if(x>b.x0-radius && x<b.x1+radius &&
           z>b.z0-radius && z<b.z1+radius) return false;
    }
    return true;
}

bool World3D::stairHeight(const Stair3D& st,float x,float z,float feet_y,
                          float& candidate_y,float radius) const {
    const float vx=st.bottom_x-st.top_x;
    const float vz=st.bottom_z-st.top_z;
    const float run2=vx*vx+vz*vz;
    const float run=std::sqrt(run2);
    const float ux=x-st.top_x,uz=z-st.top_z;
    const float t=(ux*vx+uz*vz)/run2; // t=0 at upper landing, 1 lower
    if(t < -0.17f || t > 1.17f) return false;
    const float lateral=std::fabs(ux*vz-uz*vx)/run;
    if(lateral>st.width*0.5f-radius*0.28f) return false;
    // Clamp only beyond the tread run at a safe, level landing approach.
    const float rise=1.0f-std::max(0.0f,std::min(1.0f,t));
    float best=0.48f;
    bool found=false;
    for(int d=TankTopDeck;d<EcrDeck;++d) {
        const float possible=floorHeight(d)+3.2f*rise;
        const float diff=std::fabs(feet_y-possible);
        if(diff<=best) {
            best=diff;
            candidate_y=possible;
            found=true;
        }
    }
    return found;
}

bool World3D::walkSurface(float x,float z,float feet_y,float& next_feet_y,
                          float radius) const {
    if(!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(feet_y))
        return false;
    if(radius<0.0f || radius>0.65f) return false;
    bool found=false;
    float best=0.49f;
    for(int d=TankTopDeck;d<DeckCount;++d) {
        const float y=floorHeight(d);
        const float diff=std::fabs(feet_y-y);
        if(diff<best && canStand(x,z,d,radius)) {
            best=diff;
            next_feet_y=y;
            found=true;
        }
    }
    for(int i=0;i<stairCount();++i) {
        float stair_y=0.0f;
        if(stairHeight(stairs_[i],x,z,feet_y,stair_y,radius)) {
            const float diff=std::fabs(feet_y-stair_y);
            if(diff<=best+0.001f) {
                best=diff;
                next_feet_y=stair_y;
                found=true;
            }
        }
    }
    return found;
}

bool World3D::onStair(float x,float z,float feet_y) const {
    for(int i=0;i<stairCount();++i) {
        float candidate=0.0f;
        if(stairHeight(stairs_[i],x,z,feet_y,candidate,0.25f)
           && std::fabs(feet_y-candidate)<0.30f) {
            const Stair3D& st=stairs_[i];
            const float vx=st.bottom_x-st.top_x,vz=st.bottom_z-st.top_z;
            const float t=((x-st.top_x)*vx+(z-st.top_z)*vz)/(vx*vx+vz*vz);
            if(t>0.02f && t<0.98f) return true;
        }
    }
    return false;
}

StationId World3D::lookAtStation(const Vec3& camera,float yaw) const {
    const int deck_id=cameraDeck(camera.y);
    struct Target { StationId id; int level; float x,z,range; };
    const Target targets[]={
        {EcrConsoleStation,EcrDeck,-9.18f,0.0f,2.70f},
        {LoPumpStation,PumpFlatDeck,1.69f,-2.81f,2.35f},
        {StrainerStation,PumpFlatDeck,1.68f,-5.87f,2.25f}
    };
    StationId best=NoStation;
    float best_dist=1.0e6f;
    for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);++i) {
        if(deck_id!=targets[i].level) continue;
        const float dx=targets[i].x-camera.x,dz=targets[i].z-camera.z;
        const float dist2=dx*dx+dz*dz;
        if(dist2>targets[i].range*targets[i].range || dist2<0.001f) continue;
        const float facing=(dx*std::cos(yaw)+dz*std::sin(yaw))/std::sqrt(dist2);
        if(facing<0.10f) continue;
        if(dist2<best_dist) { best=targets[i].id;best_dist=dist2; }
    }
    return best;
}

} // namespace enginewatch
