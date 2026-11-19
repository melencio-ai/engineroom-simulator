"""G2b: physical mating, assembly BOM and independent 3D export tests."""
from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"asset_pipeline"))
from assembly_builder import AssemblyFitError,bill_of_materials,validate_mated_assembly
from export_assemblies import export_assemblies
from validate_catalog import CatalogError,read_json,validate_catalog


def read_glb(path: Path) -> dict:
    raw=path.read_bytes()
    magic,ver,size=struct.unpack_from("<4sII",raw,0)
    assert (magic,ver,size)==(b"glTF",2,len(raw))
    jslen,kind=struct.unpack_from("<I4s",raw,12)
    assert kind==b"JSON"
    return json.loads(raw[20:20+jslen])


class MarineAssemblyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        registry=read_json(ROOT/"catalog/registry.json")
        cls.components={Path(p).stem:read_json(ROOT/p) for p in registry["components"]}
        cls.joint=read_json(ROOT/"catalog/assemblies/ASM.LO.FLANGE.JOINT.DN50.ILLUSTRATIVE.json")
        cls.kit=read_json(ROOT/"catalog/assemblies/ASM.FASTENER.M16.CLAMPKIT.ILLUSTRATIVE.json")

    def setUp(self):
        self.asm=copy.deepcopy(self.joint)
        self.parts=copy.deepcopy(self.components)

    def child(self,id):
        return next(c for c in self.asm["children"] if c["instance_id"]==id)

    def test_catalog_counts_and_fitted_joint(self):
        counts=validate_catalog(ROOT)
        self.assertEqual(counts,{"components":10,"assemblies":3,"systems":1,"fluids":2})
        details=validate_mated_assembly(self.asm,self.parts)
        self.assertTrue(details["resolved"])
        self.assertEqual(details["fastener_sets"],4)
        self.assertEqual(details["nominal_size"],"DN50")
        self.assertFalse(details["certified"])
        self.assertEqual(len(self.asm["children"]),21)
        self.assertEqual(len(self.asm["exploded_steps"]),19)

    def test_four_part_clampkit_uses_same_components(self):
        result=validate_mated_assembly(self.kit,self.parts)
        self.assertTrue(result["resolved"])
        self.assertEqual(len(self.kit["children"]),4)
        self.assertEqual(len(bill_of_materials(self.kit)),3)

    def test_gasket_gap_cannot_float(self):
        self.child("gasket")["translation_mm"][0]=7
        with self.assertRaisesRegex(AssemblyFitError,"ports do not physically coincide|gasket axial gap|gasket midplane"):
            validate_mated_assembly(self.asm,self.parts)

    def test_bolt_hole_count_must_match_bom(self):
        flange=self.parts["FLANGE.WN.DN50.ILLUSTRATIVE"]
        flange["parameters"]["bolt_hole_count"]=6
        with self.assertRaisesRegex(AssemblyFitError,"fastener number"):
            validate_mated_assembly(self.asm,self.parts)

    def test_bolt_centerline_cannot_miss_drilling(self):
        self.child("bolt_2")["translation_mm"][1]=65
        with self.assertRaisesRegex(AssemblyFitError,"off-axis fastener centerline|bolt circle"):
            validate_mated_assembly(self.asm,self.parts)

    def test_washer_face_cannot_be_rotated_out_of_plane(self):
        self.child("washer_near_0")["rotation_deg"]=[0,0,0]
        with self.assertRaisesRegex(AssemblyFitError,"washer must be perpendicular"):
            validate_mated_assembly(self.asm,self.parts)

    def test_mismatched_thread_pitch_rejected(self):
        self.parts["NUT.HEX.M16.ILLUSTRATIVE"]["ports"][0]["interface"]="METRIC:M16x2.5:internal"
        with self.assertRaisesRegex(AssemblyFitError,"thread nominal diameter/pitch"):
            validate_mated_assembly(self.asm,self.parts)

    def test_incorrect_bolt_length_rejected(self):
        self.parts["BOLT.HEX.ISO4014.M16X60.8_8.ZN"]["parameters"]["length_mm"]=48
        with self.assertRaisesRegex(AssemblyFitError,"outside usable shank|too short"):
            validate_mated_assembly(self.asm,self.parts)

    def test_excess_flange_clearance_rejected(self):
        self.parts["FLANGE.WN.DN50.ILLUSTRATIVE"]["parameters"]["bolt_hole_diameter_mm"]=23
        with self.assertRaisesRegex(AssemblyFitError,"hole clearance unrealistic"):
            validate_mated_assembly(self.asm,self.parts)

    def test_face_gasket_hole_cannot_overlap_drilling(self):
        self.parts["GASKET.RING.DN50.ILLUSTRATIVE"]["parameters"]["outer_diameter_mm"]=120
        with self.assertRaisesRegex(AssemblyFitError,"gasket overlaps bolt drilling"):
            validate_mated_assembly(self.asm,self.parts)

    def test_pipe_flange_bore_alignment_is_required(self):
        self.parts["FLANGE.WN.DN50.ILLUSTRATIVE"]["parameters"]["bore_diameter_mm"]=53
        with self.assertRaisesRegex(AssemblyFitError,"clear bores do not match"):
            validate_mated_assembly(self.asm,self.parts)

    def test_weld_neck_assembly_is_not_removable(self):
        self.child("left_flange")["maintainable"]=True
        with self.assertRaisesRegex(AssemblyFitError,"must not be removable"):
            validate_mated_assembly(self.asm,self.parts)

    def test_moving_welded_pair_separately_is_invalid(self):
        step=next(x for x in self.asm["exploded_steps"] if x["instance_id"]=="right_flange")
        step["translation_mm"]=[200,0,0]
        with self.assertRaisesRegex(AssemblyFitError,"must remain a single moving assembly"):
            validate_mated_assembly(self.asm,self.parts)

    def test_unsupported_pressure_rating_is_not_approved(self):
        self.asm["fit_spec"]["engineering_approved"]=True
        with self.assertRaisesRegex(AssemblyFitError,"claimed approved"):
            validate_mated_assembly(self.asm,self.parts)

    def test_deterministic_export_and_real_named_glb_nodes(self):
        with tempfile.TemporaryDirectory() as tmp:
            a,b=Path(tmp)/"first",Path(tmp)/"again"
            aa=export_assemblies(ROOT,a)
            bb=export_assemblies(ROOT,b)
            self.assertEqual(aa,bb)
            self.assertEqual(len(aa["generated"]),2)
            self.assertEqual(aa["deferred_unresolved"],["ASM.LO.PUMP.SKID.DEMO.001"])
            for rec in aa["generated"]:
                path=a/rec["id"]
                manifest=read_json(path/"manifest.json")
                self.assertFalse(manifest["engineering_approved"])
                self.assertEqual(manifest["instance_count"],rec["instances"])
                self.assertGreaterEqual(len(manifest["psp_chunks"]),1)
                assembled=read_glb(path/"assembled.glb")
                exploded=read_glb(path/"exploded.glb")
                self.assertEqual(len(assembled["nodes"]),rec["instances"])
                self.assertEqual(len(exploded["nodes"]),rec["instances"])
                self.assertEqual(assembled["extras"]["units"],"mm")
                self.assertTrue(assembled["extras"]["not_for_fabrication"])
                if rec["instances"]==21:
                    one={n["name"]:n["matrix"][12] for n in assembled["nodes"]}
                    two={n["name"]:n["matrix"][12] for n in exploded["nodes"]}
                    self.assertEqual(two["right_pipe"]-one["right_pipe"],160)
                    self.assertEqual(two["right_flange"]-one["right_flange"],160)
                for ch in manifest["psp_chunks"]:
                    data=(path/ch["path"]).read_bytes()
                    header=struct.unpack_from("<4sII",data,0)
                    self.assertEqual(header[:2],(b"EW3D",1))
                    self.assertLessEqual(ch["triangles"],1800)
                    self.assertEqual(header[2],ch["triangles"]*3)
                    self.assertEqual(len(data),12+header[2]*16)
                html=(path/"viewer.html").read_text(encoding="utf-8")
                self.assertIn("const SCENE=",html)
                payload=html.split("const SCENE=",1)[1].split(";\\n",1)[0]
                if payload==html.split("const SCENE=",1)[1]:
                    payload=payload.split(";\n",1)[0]
                viewer=json.loads(payload)
                self.assertGreater(viewer["camera"]["distance"],
                                   viewer["camera"]["min_distance"])
                if rec["instances"]==4:
                    self.assertLess(viewer["camera"]["distance"],500)
                else:
                    self.assertGreater(viewer["camera"]["distance"],1500)
                self.assertNotIn("__ENGINEWATCH_ASSEMBLY_DATA__",html)
                self.assertIn('getContext("webgl"',html)
                self.assertIn("ILLUSTRATIVE",html)
                self.assertNotIn("<script src=",html)
                for filename,digest in manifest["outputs_sha256"].items():
                    blob=(path/filename).read_bytes()
                    self.assertEqual(hashlib.sha256(blob).hexdigest(),digest)
                    self.assertEqual(blob,(b/rec["id"]/filename).read_bytes())

    def test_unknown_assembly_cannot_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(ValueError,"Unknown assembly IDs"):
                export_assemblies(ROOT,Path(tmp),{"ASM.FAKE.MISSING"})


if __name__=="__main__":
    unittest.main()
