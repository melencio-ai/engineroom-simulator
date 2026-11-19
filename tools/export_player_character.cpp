// G3.5: export the exact low-poly in-game marine engineer rig as portable OBJ.
// Coordinates are metres; the rig is procedural and does not require Blender.
#include "scene/PlayerCharacter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace enginewatch;

namespace {
std::string materialId(uint32_t c) {
    std::ostringstream ss;
    ss<<"color_"<<std::hex<<std::setfill('0')<<std::setw(8)<<c;
    return ss.str();
}
void file(const std::string& path,const std::string& content) {
    std::ofstream stream(path.c_str(),std::ios::binary);
    if(!stream) throw std::runtime_error("Unable to write "+path);
    stream<<content;
    if(!stream) throw std::runtime_error("Output error "+path);
}
std::size_t exportPose(const std::string& dir,const std::string& name,
                       const std::vector<Vertex3D>& verts,
                       std::map<uint32_t,std::string>& materials) {
    if(verts.empty() || verts.size()%3!=0 ||
       verts.size()>PlayerCharacter::MaxRuntimeVertices)
        throw std::runtime_error("Invalid character mesh vertex budget");
    std::ostringstream out;
    out<<std::fixed<<std::setprecision(6);
    out<<"# Engine Watch Marine Engineer G3.5: "<<name<<"\n"
          "# Units: metres. Prototype animated rig / illustrative PPE only.\n"
          "mtllib marine_engineer.mtl\n"
          "o marine_engineer_"<<name<<"\n";
    for(std::size_t i=0;i<verts.size();++i) {
        const Vertex3D& v=verts[i];
        if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z) ||
           ((v.color>>24)&255u)!=255u)
            throw std::runtime_error("Nonfinite/transparent character vertex");
        out<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<"\n";
        materials[v.color]=materialId(v.color);
    }
    for(std::size_t i=0;i<verts.size();i+=3) {
        if(verts[i].color!=verts[i+1].color ||
           verts[i].color!=verts[i+2].color)
            throw std::runtime_error("Face contains mixed colours");
        out<<"usemtl "<<materials[verts[i].color]<<"\n";
        out<<"f "<<i+1<<" "<<i+2<<" "<<i+3<<"\n";
    }
    file(dir+"/"+name+".obj",out.str());
    std::cout<<name<<": "<<verts.size()/3<<" triangles, "
             <<verts.size()<<" vertices\n";
    return verts.size();
}
std::string profileJson(std::size_t body_triangles,
                        std::size_t hands_triangles) {
    std::ostringstream o;
    o<<"{\n"
      "  \"schema_version\": \"0.1.0\",\n"
      "  \"id\": \"CHAR.MARINE_ENGINEER.WATCHKEEPER.ILLUSTRATIVE\",\n"
      "  \"title\": \"Marine Engineering Watchkeeper (low-poly prototype)\",\n"
      "  \"status\": \"illustrative\",\n"
      "  \"coordinate_units\": \"metres\",\n"
      "  \"body_height_m\": "<<PlayerCharacter::BodyHeight<<",\n"
      "  \"eye_height_m\": "<<PlayerCharacter::EyeHeight<<",\n"
      "  \"max_runtime_vertices\": "<<PlayerCharacter::MaxRuntimeVertices<<",\n"
      "  \"idle_body_triangles\": "<<body_triangles<<",\n"
      "  \"first_person_hand_triangles\": "<<hands_triangles<<",\n"
      "  \"uniform\": \"Navy marine boiler suit with reflective bands\",\n"
      "  \"equipment\": [\"Hard hat\", \"Working gloves\", \"Safety footwear\", \"Chest ID\"],\n"
      "  \"ppe_approved_for_tasks\": false,\n"
      "  \"character_system\": \"C++11 procedural rigid joint pivots, not skinned CAD\",\n"
      "  \"supported_poses\": [\"idle\", \"walk\", \"ladder\", \"pushbutton reach\", \"instrument inspection\"],\n"
      "  \"camera_modes\": [\"first_person_hands\", \"third_person_body\"],\n"
      "  \"limitations\": [\"PPE is visual only\", \"No finger IK\", \"No contact-time interaction verification\", \"No external OEM character asset\"]\n"
      "}\n";
    return o.str();
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2) {
        std::cerr<<"Usage: enginewatch_export_character OUTPUT_DIRECTORY\n";
        return 2;
    }
    try {
        const std::string dir(argv[1]);
        std::map<uint32_t,std::string> materials;
        std::vector<Vertex3D> vertices;
        const Vec3 feet{0.0f,0.0f,0.0f};
        CharacterPose idle;
        PlayerCharacter::buildBody(feet,0,idle,vertices);
        const std::size_t idle_count=exportPose(dir,"idle_body",vertices,materials);
        CharacterPose walk=idle;
        walk.walk_phase=1.5707963268f;walk.motion=1.0f;
        PlayerCharacter::buildBody(feet,0,walk,vertices);
        exportPose(dir,"walking_body",vertices,materials);
        walk.climbing=true;
        PlayerCharacter::buildBody(feet,0,walk,vertices);
        exportPose(dir,"ladder_body",vertices,materials);
        CharacterPose reach=idle;
        reach.action=CharacterPressControl;reach.action_progress=0.5f;
        PlayerCharacter::buildBody(feet,0,reach,vertices);
        exportPose(dir,"pushbutton_reach",vertices,materials);
        CharacterPose inspect=idle;
        inspect.action=CharacterInspectInstrument;
        inspect.action_progress=0.5f;
        PlayerCharacter::buildBody(feet,0,inspect,vertices);
        exportPose(dir,"instrument_inspect",vertices,materials);
        PlayerCharacter::buildFirstPersonHands(Vec3{0,PlayerCharacter::EyeHeight,0},
                                               0,-0.16f,idle,vertices);
        const std::size_t hands_count=exportPose(dir,"first_person_hands",vertices,materials);
        std::ostringstream mtl;
        mtl<<std::fixed<<std::setprecision(6);
        for(std::map<uint32_t,std::string>::const_iterator it=materials.begin();
            it!=materials.end();++it) {
            const uint32_t c=it->first;
            const float r=(c&255u)/255.0f;
            const float g=((c>>8)&255u)/255.0f;
            const float b=((c>>16)&255u)/255.0f;
            mtl<<"newmtl "<<it->second<<"\n"
                  "Kd "<<r<<" "<<g<<" "<<b<<"\n"
                  "Ka 0.120000 0.120000 0.120000\n"
                  "d 1.0\nillum 1\n\n";
        }
        file(dir+"/marine_engineer.mtl",mtl.str());
        file(dir+"/profile.json",profileJson(idle_count/3,hands_count/3));
        file(dir+"/README.txt",
             "Engine Watch G3.5 low-poly character proof.\n"
             "OBJ units are metres. These are static snapshots of a runtime\n"
             "procedural rigid-joint rig, not a skinned/animated GLB file.\n"
             "First-person hands, walking, ladder and button-reach poses included.\n"
             "PPE visual illustration only, not vessel/task safety certification.\n");
        std::cout<<"Character export PASS: 6 OBJ poses + MTL + profile; no PSP scene change\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"Character exporter failure: "<<e.what()<<"\n";
        return 1;
    }
}
