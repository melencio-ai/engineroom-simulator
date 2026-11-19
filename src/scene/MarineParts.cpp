#include "scene/MarineParts.hpp"

#include <algorithm>
#include <cmath>

namespace enginewatch {
namespace {
const float PI=3.14159265358979323846f;
uint32_t rgb(unsigned r,unsigned g,unsigned b) {
    return 0xff000000u|(b<<16)|(g<<8)|r;
}
unsigned byte(float v) {
    return static_cast<unsigned>(std::max(0.0f,std::min(255.0f,v)));
}
Vec3 add(const Vec3& a,const Vec3& b) { return Vec3{a.x+b.x,a.y+b.y,a.z+b.z}; }
Vec3 sub(const Vec3& a,const Vec3& b) { return Vec3{a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 mul(const Vec3& a,float b) { return Vec3{a.x*b,a.y*b,a.z*b}; }
Vec3 cross(const Vec3& a,const Vec3& b) {
    return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
float dot(const Vec3& a,const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 normalized(const Vec3& a) {
    const float len=std::sqrt(dot(a,a));
    return len>0.00001f?mul(a,1.0f/len):Vec3{1,0,0};
}

const MarineFinish finishes[MarineMaterialCount]={
    {"Marine green epoxy coating",rgb(40,93,67),0.68f,0.12f},
    {"Green coated open grating",rgb(65,113,82),0.79f,0.20f},
    {"Off-white coated structural steel",rgb(213,220,211),0.63f,0.10f},
    {"Hot-dip galvanized steel",rgb(158,172,167),0.54f,0.58f},
    {"Grey-green structural steel",rgb(135,151,144),0.66f,0.36f},
    {"Marine machinery blue",rgb(69,105,116),0.61f,0.30f},
    {"Insulated/clad pipe",rgb(171,177,170),0.72f,0.24f},
    {"Cast iron or dark steel",rgb(77,84,83),0.76f,0.30f},
    {"Stainless steel",rgb(190,202,199),0.38f,0.84f},
    {"Brass or bronze instrumentation",rgb(194,164,92),0.40f,0.76f},
    {"Safety yellow coating",rgb(229,180,63),0.73f,0.08f},
    {"LO identification orange",rgb(209,128,62),0.75f,0.08f},
    {"Water identification blue",rgb(69,134,177),0.75f,0.08f},
    {"Fire system identification red",rgb(169,66,54),0.75f,0.08f},
    {"Elastomer gasket black",rgb(46,49,47),0.94f,0.0f},
    {"Diffuser/lighting white",rgb(238,239,214),0.58f,0.0f}
};
const char* names[MarinePartCount]={
    "Steel plate","Checker plate","Open grating","I-beam","Channel section",
    "Angle section","Bulb flat","Flat bar","Gusset bracket","Stair tread",
    "Handrail post","Handrail span","Pipe spool","90-degree pipe elbow",
    "Pipe tee","Concentric reducer","Weld-neck flange","Blind flange",
    "Flange gasket","Hex bolt","Hex nut","Flat washer","Gate valve",
    "Globe valve","Butterfly valve","Y-strainer","Pressure gauge",
    "Pipe clamp","Pipe support","Cable tray","Access cover","Work light","U-bolt"
};

struct Builder {
    std::vector<Vertex3D>& out;
    explicit Builder(std::vector<Vertex3D>& v):out(v) {}
    uint32_t grey(float light) const {
        unsigned c=byte(180.0f*light);
        return rgb(c,c,c);
    }
    void tri(const Vec3& a,const Vec3& b,const Vec3& c,float light=1.0f) {
        uint32_t col=grey(light);
        out.push_back(Vertex3D{col,a.x,a.y,a.z});
        out.push_back(Vertex3D{col,b.x,b.y,b.z});
        out.push_back(Vertex3D{col,c.x,c.y,c.z});
    }
    void quad(const Vec3& a,const Vec3& b,const Vec3& c,const Vec3& d,float light=1.0f) {
        tri(a,b,c,light);tri(a,c,d,light);
    }
    void box(float x,float y,float z,float hx,float hy,float hz,float light=1.0f) {
        const float x0=x-hx,x1=x+hx,y0=y-hy,y1=y+hy,z0=z-hz,z1=z+hz;
        quad(Vec3{x0,y1,z0},Vec3{x1,y1,z0},Vec3{x1,y1,z1},Vec3{x0,y1,z1},light*1.18f);
        quad(Vec3{x0,y0,z1},Vec3{x1,y0,z1},Vec3{x1,y0,z0},Vec3{x0,y0,z0},light*0.58f);
        quad(Vec3{x0,y0,z0},Vec3{x1,y0,z0},Vec3{x1,y1,z0},Vec3{x0,y1,z0},light*0.85f);
        quad(Vec3{x1,y0,z1},Vec3{x0,y0,z1},Vec3{x0,y1,z1},Vec3{x1,y1,z1},light*0.90f);
        quad(Vec3{x0,y0,z1},Vec3{x0,y0,z0},Vec3{x0,y1,z0},Vec3{x0,y1,z1},light*0.78f);
        quad(Vec3{x1,y0,z0},Vec3{x1,y0,z1},Vec3{x1,y1,z1},Vec3{x1,y1,z0},light*1.05f);
    }
    void tube(const Vec3& a,const Vec3& b,float radius,float inner,int sides=10,
              float light=1.0f) {
        const Vec3 d=normalized(sub(b,a));
        Vec3 reference=std::fabs(d.y)>0.90f?Vec3{0,0,1}:Vec3{0,1,0};
        const Vec3 u=normalized(cross(d,reference)),v=cross(d,u);
        for(int i=0;i<sides;++i) {
            const float angle=2.0f*PI*i/sides,angle2=2.0f*PI*(i+1)/sides;
            const Vec3 u0=add(mul(u,std::cos(angle)),mul(v,std::sin(angle)));
            const Vec3 u1=add(mul(u,std::cos(angle2)),mul(v,std::sin(angle2)));
            const Vec3 oa=add(a,mul(u0,radius)),ob=add(a,mul(u1,radius));
            const Vec3 ea=add(b,mul(u0,radius)),eb=add(b,mul(u1,radius));
            quad(oa,ea,eb,ob,light*(0.83f+0.25f*std::fabs(u0.y)));
            if(inner>0.0001f) {
                const Vec3 ia=add(a,mul(u0,inner)),ib=add(a,mul(u1,inner));
                const Vec3 ja=add(b,mul(u0,inner)),jb=add(b,mul(u1,inner));
                quad(ia,ib,jb,ja,light*0.56f);
                quad(oa,ob,ib,ia,light*0.74f);
                quad(ea,ja,jb,eb,light*0.96f);
            } else {
                tri(a,ob,oa,light*0.73f);
                tri(b,ea,eb,light*1.10f);
            }
        }
    }
    void rod(const Vec3& a,const Vec3& b,float radius,int sides=7,float light=1.0f) {
        tube(a,b,radius,0.0f,sides,light);
    }
    void tubeX(float a,float b,float ro,float ri=0,int sides=10,float light=1.0f) {
        tube(Vec3{a,0,0},Vec3{b,0,0},ro,ri,sides,light);
    }
    void tubeY(float a,float b,float ro,float ri=0,int sides=10,float light=1.0f) {
        tube(Vec3{0,a,0},Vec3{0,b,0},ro,ri,sides,light);
    }
    void tubeZ(float a,float b,float ro,float ri=0,int sides=10,float light=1.0f) {
        tube(Vec3{0,0,a},Vec3{0,0,b},ro,ri,sides,light);
    }
    void checker(float y=0.0f) {
        box(0,y,0,0.50f,0.035f,0.50f);
        // Raised diagonal tread bars are low-poly but real 3D, not a flat grid.
        for(int i=-2;i<=2;++i) {
            float x=i*0.16f;
            quad(Vec3{x-0.10f,y+0.040f,-0.10f},Vec3{x-0.073f,y+0.040f,-0.13f},
                 Vec3{x+0.10f,y+0.040f,0.10f},Vec3{x+0.073f,y+0.040f,0.13f},1.24f);
            quad(Vec3{x-0.10f,y+0.041f,0.10f},Vec3{x-0.073f,y+0.041f,0.13f},
                 Vec3{x+0.10f,y+0.041f,-0.10f},Vec3{x+0.073f,y+0.041f,-0.13f},1.24f);
        }
    }
    void flangeBolts(float at_x,float circle,float radius,int count=8) {
        for(int i=0;i<count;++i) {
            const float angle=2.0f*PI*i/count;
            const float y=circle*std::cos(angle),z=circle*std::sin(angle);
            tube(Vec3{at_x-0.025f,y,z},Vec3{at_x+0.07f,y,z},
                 radius,0.0f,6,0.96f);
        }
    }
    void wheel(float y,float radius,float light=1.0f) {
        for(int i=0;i<12;++i) {
            const float a=2.0f*PI*i/12,b=2.0f*PI*(i+1)/12;
            rod(Vec3{radius*std::cos(a),y,radius*std::sin(a)},
                Vec3{radius*std::cos(b),y,radius*std::sin(b)},0.028f,5,light);
        }
        for(int i=0;i<4;++i) {
            const float a=PI*0.5f*i;
            rod(Vec3{0,y,0},Vec3{radius*std::cos(a),y,radius*std::sin(a)},
                0.021f,5,light*0.85f);
        }
        tubeY(y-0.04f,y+0.04f,0.065f,0,8);
    }
    void valveBody(float body_radius=0.23f) {
        tubeX(-0.46f,0.46f,0.145f,0.10f,10);
        tubeX(-0.44f,-0.31f,0.275f,0.12f,10,0.95f);
        tubeX(0.31f,0.44f,0.275f,0.12f,10,0.95f);
        tubeX(-0.21f,0.21f,body_radius,0,12,0.98f);
        tubeY(0.18f,0.47f,0.10f,0,8,0.82f);
    }
};

void buildOne(MarinePartId id,std::vector<Vertex3D>& vertices) {
    Builder b(vertices);
    switch(id) {
    case SteelPlate:
        b.box(0,0,0,0.5f,0.038f,0.5f);
        break;
    case CheckerPlate:
        b.checker();
        break;
    case OpenSteelGrating:
        for(int i=0;i<11;++i)
            b.box(-0.47f+i*0.094f,0,0,0.018f,0.03f,0.49f,0.93f);
        for(int j=0;j<5;++j)
            b.box(0,-0.01f,-0.44f+j*0.22f,0.49f,0.022f,0.015f,1.14f);
        break;
    case IBeam:
        b.box(0,-0.27f,0,0.50f,0.045f,0.24f);
        b.box(0,0.27f,0,0.50f,0.045f,0.24f);
        b.box(0,0,0,0.50f,0.27f,0.050f,0.93f);
        break;
    case ChannelSection:
        b.box(0,0,-0.21f,0.5f,0.30f,0.040f,0.91f);
        b.box(0,-0.27f,0,0.5f,0.04f,0.23f);
        b.box(0,0.27f,0,0.5f,0.04f,0.23f);
        break;
    case AngleSection:
        b.box(0,-0.27f,0,0.5f,0.04f,0.26f);
        b.box(0,0,0.22f,0.5f,0.27f,0.04f,0.89f);
        break;
    case BulbFlat:
        b.box(0,0,0,0.50f,0.27f,0.045f,0.94f);
        b.rod(Vec3{-0.5f,0.26f,0.02f},Vec3{0.5f,0.26f,0.02f},0.075f,7);
        break;
    case FlatBar:
        b.box(0,0,0,0.5f,0.15f,0.032f);
        break;
    case GussetBracket:
        b.tri(Vec3{-0.47f,-0.45f,-0.035f},
              Vec3{0.47f,-0.45f,-0.035f},
              Vec3{0.47f,0.47f,-0.035f},0.90f);
        b.tri(Vec3{-0.47f,-0.45f,0.035f},
              Vec3{0.47f,0.47f,0.035f},
              Vec3{0.47f,-0.45f,0.035f},1.12f);
        b.box(0,-0.45f,0,0.48f,0.04f,0.06f);
        b.box(0.45f,0,0,0.04f,0.45f,0.06f);
        break;
    case StairTread:
        b.checker(-0.01f);
        b.box(0.47f,0.031f,0,0.027f,0.025f,0.50f,1.24f);
        break;
    case HandrailPost:
        b.box(0,-0.48f,0,0.15f,0.035f,0.15f,0.82f);
        b.tubeY(-0.48f,0.49f,0.040f,0.0f,8);
        break;
    case HandrailSpan:
        b.tubeX(-0.50f,0.50f,0.039f,0,8);
        b.tube(Vec3{-0.5f,-0.45f,0},Vec3{-0.5f,0,0},0.032f,0,8);
        b.tube(Vec3{0.5f,-0.45f,0},Vec3{0.5f,0,0},0.032f,0,8);
        break;
    case PipeSpool:
        b.tubeX(-0.50f,0.50f,0.13f,0.098f,12);
        break;
    case PipeElbow90: {
        // A pipe centerline bent through a quarter-circle in the XZ plane.
        const int arcs=8,sides=10;
        for(int a=0;a<arcs;++a) {
            float t=PI*0.5f*a/arcs,u=PI*0.5f*(a+1)/arcs;
            Vec3 ca{0.45f*std::sin(t)-0.23f,0,0.45f*(1.0f-std::cos(t))-0.23f};
            Vec3 cb{0.45f*std::sin(u)-0.23f,0,0.45f*(1.0f-std::cos(u))-0.23f};
            for(int j=0;j<sides;++j) {
                float p=2.0f*PI*j/sides,q=2.0f*PI*(j+1)/sides;
                const Vec3 an{-std::sin(t),0,std::cos(t)};
                const Vec3 bn{-std::sin(u),0,std::cos(u)};
                Vec3 a0=add(ca,add(mul(an,0.13f*std::cos(p)),Vec3{0,0.13f*std::sin(p),0}));
                Vec3 a1=add(ca,add(mul(an,0.13f*std::cos(q)),Vec3{0,0.13f*std::sin(q),0}));
                Vec3 b0=add(cb,add(mul(bn,0.13f*std::cos(p)),Vec3{0,0.13f*std::sin(p),0}));
                Vec3 b1=add(cb,add(mul(bn,0.13f*std::cos(q)),Vec3{0,0.13f*std::sin(q),0}));
                b.quad(a0,b0,b1,a1,0.81f+0.28f*std::fabs(std::cos(p)));
            }
        }
        break;
    }
    case PipeTee:
        b.tubeX(-0.5f,0.5f,0.13f,0.087f,10);
        b.tubeY(0.0f,0.48f,0.13f,0.087f,10);
        break;
    case ConcentricReducer: {
        for(int j=0;j<12;++j) {
            const float a=2*PI*j/12,c=2*PI*(j+1)/12;
            b.quad(Vec3{-0.48f,0.22f*std::cos(a),0.22f*std::sin(a)},
                   Vec3{0.48f,0.11f*std::cos(a),0.11f*std::sin(a)},
                   Vec3{0.48f,0.11f*std::cos(c),0.11f*std::sin(c)},
                   Vec3{-0.48f,0.22f*std::cos(c),0.22f*std::sin(c)});
        }
        break;
    }
    case WeldNeckFlange:
        b.tubeX(-0.48f,0.42f,0.22f,0.14f,12);
        b.tubeX(-0.07f,0.09f,0.47f,0.14f,12);
        b.flangeBolts(0.10f,0.36f,0.052f);
        break;
    case BlindFlange:
        b.tubeX(-0.10f,0.10f,0.48f,0.0f,12);
        b.flangeBolts(0.12f,0.36f,0.052f);
        break;
    case PipeGasket:
        b.tubeX(-0.035f,0.035f,0.37f,0.21f,12);
        break;
    case HexBolt:
        b.tubeY(-0.50f,0.32f,0.065f,0.0f,8);
        b.tubeY(0.32f,0.50f,0.145f,0.0f,6);
        b.tubeY(0.24f,0.32f,0.105f,0.0f,8);
        break;
    case HexNut:
        b.tubeY(-0.13f,0.13f,0.24f,0.10f,6);
        break;
    case FlatWasher:
        b.tubeY(-0.025f,0.025f,0.25f,0.105f,12);
        break;
    case GateValve:
        b.valveBody(0.26f);
        b.box(0,0.35f,0,0.18f,0.17f,0.15f);
        b.rod(Vec3{0,0.42f,0},Vec3{0,0.79f,0},0.055f);
        b.wheel(0.81f,0.34f);
        break;
    case GlobeValve:
        b.valveBody(0.35f);
        b.tubeY(0.32f,0.57f,0.15f,0,8);
        b.rod(Vec3{0,0.49f,0},Vec3{0,0.82f,0},0.05f);
        b.wheel(0.84f,0.32f);
        break;
    case ButterflyValve:
        b.tubeX(-0.15f,0.15f,0.38f,0.24f,12);
        b.tubeX(-0.04f,0.04f,0.24f,0,10,0.75f);
        b.tubeY(0.30f,0.72f,0.045f,0,8);
        b.rod(Vec3{0,0.72f,0},Vec3{0.46f,0.72f,0},0.05f,8);
        break;
    case YStrainer:
        b.tubeX(-0.5f,0.5f,0.14f,0.09f,10);
        b.tube(Vec3{-0.06f,0.0f,0},Vec3{0.40f,-0.47f,0},
               0.18f,0.11f,10);
        b.tube(Vec3{0.36f,-0.45f,0},Vec3{0.44f,-0.52f,0},
               0.24f,0.10f,10,0.85f);
        break;
    case PressureGauge:
        b.tubeX(-0.18f,0.18f,0.37f,0,16,0.89f);
        b.tubeX(0.18f,0.20f,0.32f,0,16,1.21f);
        b.rod(Vec3{0.218f,0,0},Vec3{0.224f,0.22f,0.12f},0.015f,5,0.59f);
        b.tubeY(-0.50f,-0.34f,0.07f,0,8);
        break;
    case PipeClamp:
        for(int i=0;i<12;++i) {
            float a=PI*i/12.0f,c=PI*(i+1)/12.0f;
            b.rod(Vec3{0.37f*std::cos(a),0.37f*std::sin(a),0},
                  Vec3{0.37f*std::cos(c),0.37f*std::sin(c),0},0.035f,6);
        }
        b.box(-0.39f,-0.02f,0,0.055f,0.14f,0.08f);
        b.box(0.39f,-0.02f,0,0.055f,0.14f,0.08f);
        break;
    case PipeSupport:
        b.box(0,-0.47f,0,0.45f,0.04f,0.36f);
        b.box(0,-0.12f,0,0.075f,0.36f,0.075f,0.94f);
        b.box(0,0.24f,0,0.41f,0.06f,0.20f,0.90f);
        b.tubeX(-0.38f,0.38f,0.20f,0.15f,10);
        break;
    case CableTray:
        b.box(0,-0.02f,-0.40f,0.50f,0.18f,0.035f);
        b.box(0,-0.02f,0.40f,0.50f,0.18f,0.035f);
        for(int i=-4;i<=4;i+=2)
            b.box(i*0.10f,-0.18f,0,0.045f,0.028f,0.42f,0.93f);
        break;
    case AccessCover:
        b.checker(-0.02f);
        for(int side=-1;side<=1;side+=2) {
            b.rod(Vec3{side*0.26f,0.035f,-0.15f},
                  Vec3{side*0.26f,0.11f,-0.15f},0.028f,6);
            b.rod(Vec3{side*0.26f,0.11f,-0.15f},
                  Vec3{side*0.26f,0.11f,0.15f},0.028f,6);
            b.rod(Vec3{side*0.26f,0.11f,0.15f},
                  Vec3{side*0.26f,0.035f,0.15f},0.028f,6);
        }
        break;
    case WorkLightFixture:
        b.box(0,0,0,0.50f,0.06f,0.16f,0.98f);
        b.box(0,-0.052f,0,0.45f,0.016f,0.12f,1.32f);
        break;
    case Ubolt:
        b.rod(Vec3{-0.28f,-0.45f,0},Vec3{-0.28f,0,0},0.044f,6);
        b.rod(Vec3{0.28f,-0.45f,0},Vec3{0.28f,0,0},0.044f,6);
        for(int i=0;i<10;++i) {
            float a=PI*i/10,beta=PI*(i+1)/10;
            b.rod(Vec3{0.28f*std::cos(a),0.28f*std::sin(a),0},
                  Vec3{0.28f*std::cos(beta),0.28f*std::sin(beta),0},
                  0.044f,6);
        }
        break;
    default: break;
    }
}
} // anonymous namespace

MarinePartLibrary::MarinePartLibrary() { build(); }
void MarinePartLibrary::build() {
    for(int i=0;i<MarinePartCount;++i) {
        meshes_[i].reserve(384);
        buildOne(static_cast<MarinePartId>(i),meshes_[i]);
    }
}
const MarineFinish& MarinePartLibrary::finish(MarineMaterialId id) const {
    if(id<0 || id>=MarineMaterialCount) id=StructuralSteelGrey;
    return finishes[static_cast<int>(id)];
}
const char* MarinePartLibrary::partName(MarinePartId id) const {
    if(id<0 || id>=MarinePartCount) return "Invalid component";
    return names[static_cast<int>(id)];
}
std::size_t MarinePartLibrary::templateVertexCount(MarinePartId id) const {
    if(id<0 || id>=MarinePartCount) return 0;
    return meshes_[static_cast<int>(id)].size();
}

bool MarinePartLibrary::append(std::vector<Vertex3D>& destination,MarinePartId id,
                              MarineMaterialId material,const PartTransform& t) const {
    if(id<0 || id>=MarinePartCount || material<0 || material>=MarineMaterialCount)
        return false;
    const float args[]={t.position.x,t.position.y,t.position.z,
                        t.scale.x,t.scale.y,t.scale.z,
                        t.yaw_rad,t.pitch_rad,t.roll_rad};
    for(unsigned i=0;i<sizeof(args)/sizeof(args[0]);++i)
        if(!std::isfinite(args[i])) return false;
    if(t.scale.x<=0.0f || t.scale.y<=0.0f || t.scale.z<=0.0f)
        return false;

    const uint32_t base=finishes[static_cast<int>(material)].abgr;
    const float r=static_cast<float>(base&255u),g=static_cast<float>((base>>8)&255u);
    const float b=static_cast<float>((base>>16)&255u);
    const float cr=std::cos(t.roll_rad),sr=std::sin(t.roll_rad);
    const float cp=std::cos(t.pitch_rad),sp=std::sin(t.pitch_rad);
    const float cy=std::cos(t.yaw_rad),sy=std::sin(t.yaw_rad);
    const std::vector<Vertex3D>& mesh=meshes_[static_cast<int>(id)];
    destination.reserve(destination.size()+mesh.size());
    for(std::size_t i=0;i<mesh.size();++i) {
        const Vertex3D& p=mesh[i];
        // Scale, local Z-roll, X-pitch, then global Y-yaw.
        const float xx=p.x*t.scale.x,yy=p.y*t.scale.y,zz=p.z*t.scale.z;
        const float a=cr*xx-sr*yy,d=sr*xx+cr*yy;
        const float e=cp*d-sp*zz,f=sp*d+cp*zz;
        const float x=cy*a-sy*f,z=sy*a+cy*f;
        const float light=(p.color&255u)/180.0f;
        destination.push_back(Vertex3D{
            rgb(byte(r*light),byte(g*light),byte(b*light)),
            x+t.position.x,e+t.position.y,z+t.position.z
        });
    }
    return true;
}
} // namespace enginewatch
