// Engine Watch G3 native shipyard deck-layout exporter.
// Produces floor-plan JSON and one inspectable vector blueprint per deck.
// No external SVG/JSON libraries or C++17 filesystem support required.
#include "scene/DeckLayout.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace enginewatch;

namespace {

const char* kindName(DeckSurface kind) {
    switch(kind) {
    case SolidSteelPlate: return "solid_steel_plate";
    case OpenSteelGrate: return "open_steel_grating";
    case InsetServiceCover: return "inset_service_cover";
    default: return "no_floor";
    }
}

const char* openingName(OpeningPurpose kind) {
    switch(kind) {
    case EngineMachineryWell: return "main_engine_well";
    case ForwardStairwell: return "forward_inclined_stair";
    case AftEscapeStairwell: return "aft_escape_stair";
    }
    return "unmapped_opening";
}

std::string filename(const std::string& dir,const std::string& name) {
    return dir+"/"+name;
}

double xx(float x) { return 46.0+(static_cast<double>(x)+10.5)*38.2; }
double yy(float z) { return 44.0+(7.35-static_cast<double>(z))*38.2; }

void rect(std::ostream& o,float x0,float x1,float z0,float z1,
          const std::string& css) {
    o<<"<rect class=\""<<css<<"\" x=\""<<xx(x0)
     <<"\" y=\""<<yy(z1)<<"\" width=\""<<(x1-x0)*38.2
     <<"\" height=\""<<(z1-z0)*38.2<<"\" />\n";
}

void makeSvg(const DeckPlan& plan,int deck,const std::string& path) {
    std::ofstream out(path.c_str());
    if(!out) throw std::runtime_error("Unable to write deck schematic: "+path);
    out<<std::fixed<<std::setprecision(2);
    out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1100\" "
          "height=\"700\" viewBox=\"0 0 1100 700\" role=\"img\" "
          "aria-label=\"Engine Watch G3 Deck "<<deck<<" construction plan\">\n"
          "<defs><pattern id=\"grate\" width=\"7\" height=\"7\" "
          "patternUnits=\"userSpaceOnUse\"><path d=\"M0 7 L7 0\" "
          "stroke=\"#b0d5ad\" stroke-width=\"1\" /></pattern></defs>\n"
          "<style>"
          "text{font-family:Arial,sans-serif;fill:#e6efea}"
          ".plate{fill:#286b4d;stroke:#afd1b5;stroke-width:0.75}"
          ".grate{fill:#426d67;stroke:#b5d9b7;stroke-width:0.8}"
          ".cover{fill:#bca259;stroke:#f8e7a6;stroke-width:1.2}"
          ".shaft{fill:#0c151e;stroke:#eac574;stroke-width:1.2;stroke-dasharray:6 3}"
          ".lane{fill:#d39152;fill-opacity:.14;stroke:#d7a863;stroke-dasharray:5 3}"
          ".label{font-size:13px;letter-spacing:1.2px;fill:#d3e1da}"
          ".legend{font-size:14px;fill:#c1d4cc}"
          "</style>\n";
    out<<"<rect width=\"1100\" height=\"700\" fill=\"#101e27\" />\n";
    out<<"<text x=\"46\" y=\"26\" font-size=\"18\" font-weight=\"bold\">"
          "ENGINE WATCH / G3 CONSTRUCTION PLAN / DECK 0"<<deck
       <<"</text>\n";
    out<<"<text x=\"46\" y=\"663\" class=\"legend\">"
          "Green: solid chequer plate  |  Hatched: open grating  |  "
          "Ochre: removable access cover  |  Dark: real deck opening</text>\n";
    out<<"<text x=\"46\" y=\"686\" class=\"label\">"
          "ILLUSTRATIVE SHIP ARRANGEMENT / PLAN UNITS METRES / "
          "NOT APPROVED GA DRAWING</text>\n";
    // Steel top-floor cells come directly from the playable physical plan.
    for(const DeckCell& c : plan.cells()) {
        if(c.level!=deck) continue;
        const char* css=(c.surface==OpenSteelGrate)?"grate":
                        (c.surface==InsetServiceCover)?"cover":"plate";
        rect(out,c.x0,c.x1,c.z0,c.z1,css);
        if(c.surface==OpenSteelGrate)
            out<<"<rect fill=\"url(#grate)\" x=\""<<xx(c.x0)
               <<"\" y=\""<<yy(c.z1)<<"\" width=\""<<(c.x1-c.x0)*38.2
               <<"\" height=\""<<(c.z1-c.z0)*38.2<<"\" />\n";
    }
    for(const DeckOpening& h : plan.openings()) {
        if(h.level!=deck) continue;
        rect(out,h.x0,h.x1,h.z0,h.z1,"shaft");
        out<<"<text x=\""<<(xx(h.x0)+xx(h.x1))*0.5
           <<"\" y=\""<<(yy(h.z0)+yy(h.z1))*0.5
           <<"\" font-size=\"12\" text-anchor=\"middle\" fill=\"#f0cf91\">"
           <<openingName(h.purpose)<<"</text>\n";
    }
    if(deck==TankTopDeck) {
        // L0 still has a solid floor below the stairs: show their lanes as
        // physical obstructions, NOT holes in the tank top plate.
        const Stair3D& f=plan.stair(0);
        const Stair3D& a=plan.stair(1);
        rect(out,-1.82f,0.86f,f.top_z-0.66f,f.top_z+0.66f,"lane");
        rect(out,a.top_x-0.66f,a.top_x+0.66f,-4.54f,-1.88f,"lane");
    }
    out<<"<text x=\"805\" y=\"50\" class=\"label\">FORWARD +X / ATHWARTSHIPS Z</text>\n"
          "<text x=\"805\" y=\"69\" class=\"label\">Floor elevation "
       <<3.2*deck<<" m</text>\n";
    out<<"</svg>\n";
    if(!out) throw std::runtime_error("Unable to finish deck schematic");
}

void makeJson(const DeckPlan& plan,const std::string& path) {
    std::ofstream o(path.c_str());
    if(!o) throw std::runtime_error("Unable to write floor-plan JSON");
    o<<std::fixed<<std::setprecision(5);
    o<<"{\n"
       "  \"format\": \"engine-watch-deckplan/1\",\n"
       "  \"coordinate_system\": {\"x\": \"longitudinal metres\", "
       "\"y\": \"vertical metres\", \"z\": \"athwartships metres\"},\n"
       "  \"warning\": \"Educational composite, not approved vessel design\",\n"
       "  \"decks\": [";
    for(int d=0;d<DeckCount;++d) {
        if(d) o<<",";
        o<<"{\"index\":"<<d<<",\"elevation_m\":"<<3.2*d<<"}";
    }
    o<<"],\n  \"cells\": [\n";
    const std::vector<DeckCell>& cells=plan.cells();
    for(std::size_t i=0;i<cells.size();++i) {
        const DeckCell& c=cells[i];
        if(i) o<<",\n";
        o<<"    {\"id\": \"D"<<c.level<<"-C"<<i<<"\","
         <<"\"deck\":"<<c.level<<",\"surface\":\""<<kindName(c.surface)<<"\","
         <<"\"x0_m\":"<<c.x0<<",\"x1_m\":"<<c.x1
         <<",\"z0_m\":"<<c.z0<<",\"z1_m\":"<<c.z1<<"}";
    }
    o<<"\n  ],\n  \"openings\": [\n";
    const std::vector<DeckOpening>& ops=plan.openings();
    for(std::size_t i=0;i<ops.size();++i) {
        const DeckOpening& c=ops[i];
        if(i) o<<",\n";
        o<<"    {\"deck\":"<<c.level<<",\"purpose\":\""<<openingName(c.purpose)<<"\","
         <<"\"x0_m\":"<<c.x0<<",\"x1_m\":"<<c.x1
         <<",\"z0_m\":"<<c.z0<<",\"z1_m\":"<<c.z1<<"}";
    }
    o<<"\n  ],\n  \"grating_bands\": [\n";
    const std::vector<DeckGratingBand>& bands=plan.gratingBands();
    for(std::size_t i=0;i<bands.size();++i) {
        const DeckGratingBand& c=bands[i];
        if(i) o<<",\n";
        o<<"    {\"deck\":"<<c.level<<",\"bays\":"<<c.bay_count<<","
         <<"\"x0_m\":"<<c.x0<<",\"x1_m\":"<<c.x1
         <<",\"z0_m\":"<<c.z0<<",\"z1_m\":"<<c.z1<<"}";
    }
    o<<"\n  ],\n  \"stairs\": [\n";
    for(int i=0;i<2;++i) {
        if(i) o<<",\n";
        const Stair3D& s=plan.stair(i);
        o<<"    {\"id\":"<<i<<",\"top_x_m\":"<<s.top_x
         <<",\"top_z_m\":"<<s.top_z
         <<",\"bottom_x_m\":"<<s.bottom_x
         <<",\"bottom_z_m\":"<<s.bottom_z
         <<",\"width_m\":"<<s.width
         <<",\"alternate_escape\":"<<(s.alternate_escape?"true":"false")<<"}";
    }
    o<<"\n  ]\n}\n";
    if(!o) throw std::runtime_error("Unable to finish deck plan JSON");
}

} // namespace

int main(int argc,char** argv) {
    if(argc!=2) {
        std::cerr<<"Usage: enginewatch_export_deckplan OUTPUT_DIRECTORY\n";
        return 2;
    }
    try {
        DeckPlan p;
        if(!p.validate()) {
            std::cerr<<"Deck plan has overlapping decks or invalid openings\n";
            return 1;
        }
        const std::string root=argv[1];
        makeJson(p,filename(root,"deck-constructor.json"));
        for(int d=0;d<DeckCount;++d) {
            std::ostringstream name;name<<"deck-0"<<d<<".svg";
            makeSvg(p,d,filename(root,name.str()));
        }
        std::ofstream note(filename(root,"README.txt").c_str());
        note<<"Engine Watch G3 shared physical deck constructor\n"
               "Five vector floor plans and the exact C++ collision/mesh topology\n"
               "come from the same DeckPlan instance.\n"
               "Floor units: metres. SVG files are not certified naval drawings.\n"
               "The PSP scene remains a representative training composite.\n";
        std::cout<<"G3 deck export PASS: "<<p.cells().size()
                 <<" floor cells, "<<p.openings().size()<<" cutouts, "
                 <<p.gratingBands().size()<<" supported grating bands\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"G3 exporter error: "<<e.what()<<"\n";
        return 1;
    }
}
