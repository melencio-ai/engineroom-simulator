#include "scene/MarineParts.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

using namespace enginewatch;

namespace {
std::string stem(const char* name) {
    std::string s;
    for(const char* c=name;*c;++c) {
        const unsigned char ch=static_cast<unsigned char>(*c);
        if(std::isalnum(ch)) s.push_back(static_cast<char>(std::tolower(ch)));
        else if(s.empty() || s[s.size()-1]!='_') s.push_back('_');
    }
    if(!s.empty() && s[s.size()-1]=='_') s.resize(s.size()-1);
    return s;
}
std::string join(const std::string& dir,const std::string& file) {
    if(dir.empty() || dir==".") return file;
    const char tail=dir[dir.size()-1];
    return dir+(tail=='/'||tail=='\\'?"":"/")+file;
}
MarineMaterialId defaultFinish(MarinePartId p) {
    switch(p) {
    case CheckerPlate:case StairTread:case AccessCover:return GreenEpoxy;
    case OpenSteelGrating:return GreenGrating;
    case IBeam:case ChannelSection:case AngleSection:case BulbFlat:
    case FlatBar:case GussetBracket:case SteelPlate:return StructuralSteelGrey;
    case HandrailPost:case HandrailSpan:return OffWhiteCoatedSteel;
    case PipeSpool:case PipeElbow90:case PipeTee:case ConcentricReducer:
        return PipeInsulation;
    case WeldNeckFlange:case BlindFlange:case HexBolt:case HexNut:
    case FlatWasher:case PipeClamp:case PipeSupport:case Ubolt:
        return GalvanizedSteel;
    case PipeGasket:return RubberGasket;
    case GateValve:case GlobeValve:return MachineryBlue;
    case ButterflyValve:case YStrainer:return CastIron;
    case PressureGauge:return BronzeInstrument;
    case CableTray:return StainlessSteel;
    case WorkLightFixture:return WorkLightWhite;
    default:return StructuralSteelGrey;
    }
}
bool exportOne(const MarinePartLibrary& lib,MarinePartId id,const std::string& dir) {
    const std::string name=stem(lib.partName(id));
    const std::string objName=name+".obj",mtlName=name+".mtl";
    const MarineMaterialId finish=defaultFinish(id);
    const MarineFinish& mat=lib.finish(finish);
    std::vector<Vertex3D> mesh;
    if(!lib.append(mesh,id,finish,PartTransform(0,0,0))) return false;
    std::ofstream obj(join(dir,objName).c_str());
    std::ofstream mtl(join(dir,mtlName).c_str());
    if(!obj || !mtl) return false;
    const float r=(mat.abgr&255u)/255.0f,g=((mat.abgr>>8)&255u)/255.0f;
    const float b=((mat.abgr>>16)&255u)/255.0f;
    mtl << "# ENGINE WATCH 3D reusable marine material\n"
        << "# Palette approximates coatings, not steel certification/physics.\n"
        << "newmtl marine_finish\n"
        << "Ka 0.12 0.12 0.12\n"
        << "Kd "<<r<<" "<<g<<" "<<b<<"\n"
        << "Ks "<<0.18f+0.40f*mat.metallic<<" "
                 <<0.18f+0.40f*mat.metallic<<" "
                 <<0.18f+0.40f*mat.metallic<<"\n"
        << "Ns "<<8.0f+(1.0f-mat.roughness)*46.0f<<"\n"
        << "d 1.0\n"
        << "illum 2\n";
    obj << "# ENGINE WATCH 3D - component prototype\n"
        << "# Units: conceptual game metres, normalized base dimensions.\n"
        << "# Geometry is for visualization, NOT fabrication or pressure rating.\n"
        << "mtllib "<<mtlName<<"\n"
        << "o "<<name<<"\n"
        << "usemtl marine_finish\n";
    obj.setf(std::ios::fixed);
    obj << std::setprecision(6);
    // Baked colours are also supplied as the common OBJ vertex-color
    // extension; viewers lacking it can fall back to the companion MTL.
    for(std::size_t i=0;i<mesh.size();++i) {
        const Vertex3D& v=mesh[i];
        obj << "v "<<v.x<<" "<<v.y<<" "<<v.z<<" "
            << ((v.color&255u)/255.0f)<<" "
            << (((v.color>>8)&255u)/255.0f)<<" "
            << (((v.color>>16)&255u)/255.0f)<<"\n";
    }
    for(std::size_t i=0;i+2<mesh.size();i+=3)
        obj << "f "<<i+1<<" "<<i+2<<" "<<i+3<<"\n";
    return obj.good() && mtl.good();
}
}

int main(int argc,char** argv) {
    if(argc!=2) {
        std::fprintf(stderr,"Usage: enginewatch_export_parts EXISTING_OUTPUT_DIRECTORY\n");
        return 2;
    }
    MarinePartLibrary library;
    unsigned success=0;
    for(int i=0;i<MarinePartCount;++i) {
        MarinePartId part=static_cast<MarinePartId>(i);
        if(!exportOne(library,part,argv[1])) {
            std::fprintf(stderr,"Failed exporting %s (does directory exist?)\n",
                         library.partName(part));
            return 1;
        }
        ++success;
    }
    std::printf("Exported %u separate 3D OBJ objects with companion MTL finishes into %s\n",
                success,argv[1]);
    return success==33?0:1;
}
