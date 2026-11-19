"""G2 parametric mesh, open-hole, LOD and export regression tests."""
import json
import math
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/asset_pipeline"))
from geometry import build_mesh
from export_catalog import export_all
from validate_catalog import read_json, validate_catalog


def variant(ident):
    return read_json(ROOT/"catalog/variants"/(ident+".json"))


def inside_triangle(point, vertices):
    a,b,c=vertices
    def cross(p,q,r):
        return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
    t=[cross(a,b,point),cross(b,c,point),cross(c,a,point)]
    return all(x>=-1e-5 for x in t) or all(x<=1e-5 for x in t)


def face_covers(mesh,x,at_yz):
    for tri in mesh.triangles:
        if all(abs(v[0]-x)<1e-5 for v in tri):
            if inside_triangle(at_yz,[(v[1],v[2]) for v in tri]):
                return True
    return False


class GeometryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        validate_catalog(ROOT)
        reg=read_json(ROOT/"catalog/registry.json")
        cls.parts=[read_json(ROOT/p) for p in reg["components"]]

    def test_every_variant_has_distinct_inspection_and_psp_lod(self):
        self.assertEqual(len(self.parts),7)
        self.assertEqual(len(set(p["generator"]["name"] for p in self.parts)),6)
        for p in self.parts:
            detailed=build_mesh(p,"inspection")
            psp=build_mesh(p,"psp")
            self.assertGreater(detailed.triangle_count,psp.triangle_count,p["id"])
            self.assertGreater(psp.triangle_count,35,p["id"])
            self.assertLessEqual(psp.triangle_count,1800,p["id"])
            self.assertTrue(all(math.isfinite(n)
                  for tri in detailed.triangles for vertex in tri for n in vertex))

    def test_bolt_helical_details_and_length(self):
        for p in self.parts:
            if p["family"]!="fastener.hex_bolt":
                continue
            detailed=build_mesh(p,"inspection")
            psp=build_mesh(p,"psp")
            self.assertGreater(detailed.triangle_count,psp.triangle_count*7)
            a,b=detailed.bounds()
            self.assertAlmostEqual(a[2],-p["parameters"]["length_mm"],places=2)
            self.assertAlmostEqual(b[2],p["parameters"]["head_height_mm"],places=2)

    def test_open_nut_and_washer_bores(self):
        for ident in ("NUT.HEX.M12.ILLUSTRATIVE","WASHER.FLAT.M12.ILLUSTRATIVE"):
            m=build_mesh(variant(ident),"inspection")
            top=m.bounds()[1][2]
            for tri in m.triangles:
                if all(abs(v[2]-top)<1e-5 for v in tri):
                    self.assertFalse(inside_triangle((0,0),[(v[0],v[1]) for v in tri]))

    def test_flange_has_open_face_and_four_real_bolt_holes(self):
        p=variant("FLANGE.WN.DN50.ILLUSTRATIVE")["parameters"]
        mesh=build_mesh(variant("FLANGE.WN.DN50.ILLUSTRATIVE"),"inspection")
        self.assertFalse(face_covers(mesh,0,(0,0)))
        for idx in range(p["bolt_hole_count"]):
            a=2*math.pi*idx/p["bolt_hole_count"]
            yz=(p["bolt_circle_diameter_mm"]/2*math.cos(a),
                p["bolt_circle_diameter_mm"]/2*math.sin(a))
            self.assertFalse(face_covers(mesh,0,yz),str(idx))
            self.assertFalse(face_covers(mesh,p["flange_thickness_mm"],yz),str(idx))
        self.assertAlmostEqual(mesh.bounds()[1][0],
                 p["flange_thickness_mm"]+p["hub_length_mm"])

    def test_gasket_and_flange_interfaces_and_pipe_wall(self):
        bolt=variant("BOLT.HEX.ISO4014.M12X40.8_8.ZN")
        nut=variant("NUT.HEX.M12.ILLUSTRATIVE")
        flange=variant("FLANGE.WN.DN50.ILLUSTRATIVE")
        gasket=variant("GASKET.RING.DN50.ILLUSTRATIVE")
        pipe=variant("PIPE.SPOOL.DN50.L800.ILLUSTRATIVE")
        self.assertEqual(bolt["ports"][0]["interface"].rsplit(":",1)[0],
                         nut["ports"][0]["interface"].rsplit(":",1)[0])
        self.assertEqual(flange["ports"][0]["interface"],gasket["ports"][0]["interface"])
        self.assertEqual(flange["ports"][1]["interface"],pipe["ports"][0]["interface"])
        p=pipe["parameters"]
        self.assertEqual(p["outside_diameter_mm"]-2*p["wall_thickness_mm"],
                         p["inside_diameter_mm"])

    def test_deterministic_obj_glb_and_psp_binary(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)
            a=export_all(ROOT,path/"a")
            b=export_all(ROOT,path/"b")
            self.assertEqual(a,b)
            self.assertEqual(a["component_count"],7)
            for rec in a["components"]:
                self.assertFalse(rec["engineering_verified"])
                ident=rec["id"]
                glb=(path/"a"/"inspection"/(ident+".glb")).read_bytes()
                magic,version,total=struct.unpack_from("<4sII",glb,0)
                self.assertEqual((magic,version,total),(b"glTF",2,len(glb)))
                size,tag=struct.unpack_from("<I4s",glb,12)
                self.assertEqual(tag,b"JSON")
                doc=json.loads(glb[20:20+size].decode("utf-8"))
                self.assertFalse(doc["extras"]["certified"])
                self.assertEqual(doc["accessors"][0]["count"],
                                 rec["inspection_triangles"]*3)
                ew=(path/"a"/"psp"/(ident+".ewmesh")).read_bytes()
                magic,version,count=struct.unpack_from("<4sII",ew,0)
                self.assertEqual((magic,version),(b"EW3D",1))
                self.assertEqual(count,rec["psp_triangles"]*3)
                self.assertEqual(len(ew),12+count*16)
                colour,x,y,z=struct.unpack_from("<Ifff",ew,12)
                self.assertEqual(colour>>24,255)
                src=build_mesh(variant(ident),"psp").triangles[0][0]
                for measured,mm in zip((x,y,z),src):
                    self.assertAlmostEqual(measured,mm/1000,places=5)

    def test_unknown_id_cannot_be_generated(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,"Unknown catalog IDs"):
                export_all(ROOT,Path(temp),{"FAKE.PART"})


if __name__=="__main__":
    unittest.main()
