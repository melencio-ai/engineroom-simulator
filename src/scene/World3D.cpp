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
const uint32_t steel = rgb(64, 92, 108);
const uint32_t deck = rgb(31, 53, 63);
const uint32_t wall = rgb(44, 65, 77);
const uint32_t machinery = rgb(63, 91, 109);
const uint32_t engineBlue = rgb(47, 86, 119);
const uint32_t yellow = rgb(229, 179, 72);
const uint32_t red = rgb(160, 67, 54);
const uint32_t pipe = rgb(78, 143, 155);
const uint32_t bright = rgb(135, 158, 163);
const uint32_t green = rgb(67, 166, 126);
const uint32_t dark = rgb(26, 43, 51);
}

World3D::World3D() { build(); }

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
void World3D::obstacle(float x0,float x1,float z0,float z1) {
    obstacles_.push_back(Footprint{x0,x1,z0,z1});
}

void World3D::build() {
    vertices_.reserve(18000);
    // ECR and engine room are connected by a real doorway, not a scene switch.
    box(-6.5f,-0.15f,0.0f,3.5f,0.15f,4.2f,deck);
    box( 6.2f,-0.15f,0.0f,9.3f,0.15f,7.0f,deck);
    box(-6.5f,4.60f,0.0f,3.5f,0.15f,4.2f,dark);
    box( 6.2f,4.80f,0.0f,9.3f,0.20f,7.0f,dark);
    box(-10.0f,2.3f,0.0f,0.16f,2.3f,4.2f,wall);
    box(-6.5f,2.3f,-4.2f,3.5f,2.3f,0.16f,wall);
    box(-6.5f,2.3f, 4.2f,3.5f,2.3f,0.16f,wall);
    box(-3.0f,2.3f,-2.75f,0.20f,2.3f,1.45f,wall);
    box(-3.0f,2.3f, 2.75f,0.20f,2.3f,1.45f,wall);
    box(-3.0f,4.10f,0.0f,0.20f,0.50f,1.28f,yellow);
    box(15.5f,2.4f,0.0f,0.20f,2.4f,7.0f,wall);
    box( 6.2f,2.4f,-7.0f,9.3f,2.4f,0.18f,wall);
    box( 6.2f,2.4f, 7.0f,9.3f,2.4f,0.18f,wall);
    // A small amount of repeated grating and hazard marking gives depth cues.
    for (int i=-2;i<=15;++i) {
        box(static_cast<float>(i)+0.10f,0.018f,0.0f,0.026f,0.018f,6.80f,shade(bright,0.39f));
    }
    for (int i=-6;i<=6;++i) {
        box(6.1f,0.016f,static_cast<float>(i),9.10f,0.016f,0.020f,shade(bright,0.43f));
    }
    for (int i=0;i<5;++i) {
        box(-9.4f+i*1.25f,0.02f,3.4f,0.32f,0.022f,0.09f,yellow);
    }
    // ECR: wall-mounted LO monitoring desk, gauges and illuminated mimic boards.
    box(-9.34f,1.12f,0.0f,0.44f,0.66f,2.22f,steel);
    box(-9.03f,1.78f,0.0f,0.17f,0.76f,2.12f,dark);
    for(int i=0;i<5;++i) {
        box(-8.83f,1.94f,-1.57f+i*0.78f,0.025f,0.26f,0.28f,engineBlue);
        box(-8.79f,2.01f,-1.57f+i*0.78f,0.015f,0.045f,0.16f,
            (i<2 ? green : yellow));
    }
    box(-8.83f,1.20f,0.0f,0.05f,0.10f,1.72f,bright);
    box(-9.10f,0.60f,0.0f,0.35f,0.56f,1.95f,dark);
    // ECR ceiling light panels.
    for(int i=0;i<3;++i) box(-8.5f+i*1.6f,4.39f,0.0f,0.57f,0.027f,0.36f,rgb(172,198,189));

    // Main engine: heavy bedplate, crankcase, six distinct cylinder modules.
    box(7.55f,0.46f,0.0f,4.04f,0.46f,1.76f,dark);
    box(7.40f,1.48f,0.0f,3.35f,0.62f,1.42f,engineBlue);
    box(7.40f,2.24f,0.0f,3.24f,0.19f,1.12f,steel);
    for(int i=0;i<6;++i) {
        const float x=4.62f+i*1.12f;
        box(x,2.65f,0.0f,0.46f,0.48f,1.04f,machinery);
        box(x,3.25f,0.0f,0.49f,0.16f,0.94f,bright);
        box(x,1.35f,-1.51f,0.30f,0.24f,0.08f,steel);
        cylinderY(x,3.41f,3.81f,0.0f,0.21f,10,red);
    }
    cylinderX(11.10f,11.65f,1.48f,0.0f,1.12f,16,steel);
    cylinderX(11.66f,11.83f,1.48f,0.0f,0.30f,12,yellow);
    // Exhaust manifold, scavenge receiver and LO piping.
    cylinderX(4.25f,10.70f,3.92f,-0.76f,0.17f,12,red);
    cylinderX(4.10f,10.80f,1.04f,1.65f,0.10f,10,pipe);
    for(int i=0;i<6;++i) {
        const float x=4.62f+i*1.12f;
        box(x,3.71f,-0.43f,0.12f,0.16f,0.41f,red);
    }
    obstacle(3.20f,11.88f,-1.90f,1.90f);

    // Two separate diesel-generator sets along the fore-and-aft walkway.
    for(int side=-1;side<=1;side+=2) {
        const float z=side*4.82f;
        box(6.00f,0.31f,z,3.12f,0.31f,1.10f,dark);
        box(4.50f,1.18f,z,1.26f,0.65f,0.83f,engineBlue);
        for(int i=0;i<4;++i) box(3.75f+i*0.48f,1.88f,z,0.18f,0.18f,0.70f,machinery);
        cylinderX(5.89f,8.18f,1.23f,z,0.84f,14,steel);
        cylinderX(8.19f,8.42f,1.23f,z,0.43f,12,yellow);
        cylinderX(3.60f,5.90f,2.48f,z+side*0.52f,0.11f,10,red);
        box(4.52f,1.27f,z-side*0.98f,0.21f,0.38f,0.09f,green);
        obstacle(2.90f,8.85f,z-1.20f,z+1.20f);
    }

    // Main and standby LO pumps with nearby valve wheels and manifold.
    for(int i=0;i<2;++i) {
        const float z=i==0 ? -3.70f : 3.65f;
        box(0.60f,0.25f,z,0.87f,0.25f,0.70f,dark);
        cylinderX(-0.02f,0.88f,0.93f,z,0.45f,12,pipe);
        box(0.82f,0.93f,z,0.56f,0.49f,0.42f,machinery);
        box(0.83f,1.47f,z,0.45f,0.055f,0.33f,yellow);
        cylinderY(1.25f,1.03f,1.20f,z,0.23f,10,red);
        cylinderX(-0.45f,1.27f,1.20f,z+0.55f,0.10f,10,pipe);
        obstacle(-0.40f,1.58f,z-0.80f,z+0.80f);
    }
    // Local controls and strainer are separately inspectable.
    box(1.86f,1.20f,-2.79f,0.18f,0.65f,0.56f,steel);
    box(1.64f,1.32f,-2.78f,0.036f,0.44f,0.44f,dark);
    box(1.58f,1.49f,-2.92f,0.025f,0.080f,0.080f,green);
    box(1.58f,1.49f,-2.67f,0.025f,0.080f,0.080f,yellow);
    cylinderY(1.47f,0.30f,1.54f,-5.90f,0.30f,14,steel);
    box(1.47f,1.56f,-5.90f,0.40f,0.13f,0.40f,red);
    cylinderX(0.43f,2.40f,0.65f,-5.90f,0.105f,10,pipe);
    obstacle(1.05f,1.90f,-6.34f,-5.46f);

    // Long overhead systems: cooling pipes, ventilation and structural rails.
    cylinderX(-2.00f,14.00f,4.12f,-5.92f,0.16f,12,pipe);
    cylinderX(-2.00f,14.00f,4.12f, 5.92f,0.16f,12,pipe);
    cylinderX(-2.00f,13.70f,4.18f,-2.84f,0.11f,12,red);
    for(int i=0;i<6;++i) {
        const float x=-1.0f+i*2.85f;
        pipeZ(-5.9f,-3.1f,x,4.12f,0.09f,pipe);
        box(x,4.39f,0.0f,0.10f,0.15f,6.65f,steel);
        box(x,3.92f,-6.20f,0.13f,0.44f,0.12f,yellow);
        box(x,3.92f, 6.20f,0.13f,0.44f,0.12f,yellow);
    }
    // Protective handrails beside the main engine, with a gap at the inspection point.
    for(int side=-1;side<=1;side+=2) {
        const float z=side*2.40f;
        cylinderX(4.20f,11.60f,1.15f,z,0.035f,8,yellow);
        cylinderX(4.20f,11.60f,0.60f,z,0.030f,8,yellow);
        for(int i=0;i<5;++i) cylinderY(4.3f+i*1.75f,0.04f,1.15f,z,0.045f,8,yellow);
    }
}

bool World3D::canStand(float x, float z, float r) const {
    if(r < 0.0f || r > 1.0f) return false;
    if(x < -9.80f+r || x > 15.30f-r) return false;
    const float az = std::fabs(z);
    if(x < -3.35f) {
        if(az > 4.00f-r) return false;
    } else if(x <= -2.65f) {
        if(az > 1.24f-r) return false;
    } else if(az > 6.79f-r) return false;
    for(std::size_t i=0;i<obstacles_.size();++i) {
        const Footprint& b=obstacles_[i];
        if(x > b.x0-r && x < b.x1+r && z > b.z0-r && z < b.z1+r) return false;
    }
    return true;
}

StationId World3D::lookAtStation(const Vec3& camera, float yaw) const {
    struct Target { StationId id; float x,z,range; };
    const Target targets[] = {
        {EcrConsoleStation,-9.16f,0.0f,2.70f},
        {LoPumpStation,1.61f,-2.79f,2.30f},
        {StrainerStation,1.47f,-5.90f,2.25f}
    };
    StationId best=NoStation;
    float bestDist=1.0e6f;
    for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);++i) {
        const float dx=targets[i].x-camera.x, dz=targets[i].z-camera.z;
        const float dsq=dx*dx+dz*dz;
        if(dsq > targets[i].range*targets[i].range || dsq < 0.001f) continue;
        const float facing=(dx*std::cos(yaw)+dz*std::sin(yaw))/std::sqrt(dsq);
        if(facing < 0.10f) continue;
        if(dsq < bestDist) { best=targets[i].id; bestDist=dsq; }
    }
    return best;
}

} // namespace enginewatch
