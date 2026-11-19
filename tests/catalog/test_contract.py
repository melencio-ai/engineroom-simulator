"""Catalog contract regression tests: invalid engineering claims must fail loudly.

These tests validate metadata and parametric consistency, not pressure ratings
or dimensional compliance with licensed ISO/ASME documents.
"""
from __future__ import annotations

import copy
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "asset_pipeline"))
import validate_catalog as contract  # noqa: E402


class CatalogContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.ctx = contract.context_from(ROOT)
        for kind, directory in (
            ("components", "variants"), ("fluids", "fluids"),
            ("assemblies", "assemblies"), ("systems", "systems")
        ):
            for path in (ROOT / "catalog" / directory).glob("*.json"):
                record = contract.read_json(path)
                cls.ctx[kind][record["id"]] = record
        cls.bolt = cls.ctx["components"]["BOLT.HEX.ISO4014.M12X40.8_8.ZN"]
        cls.pipe = cls.ctx["components"]["PIPE.SPOOL.DN50.L800.ILLUSTRATIVE"]

    def test_complete_catalog_contract(self) -> None:
        counts = contract.validate_catalog(ROOT)
        self.assertEqual(counts, {
            "components": 3, "assemblies": 1, "systems": 1, "fluids": 2
        })

    def test_duplicate_registry_path_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            shutil.copytree(ROOT / "catalog", root / "catalog")
            shutil.copytree(ROOT / "schemas", root / "schemas")
            file = root / "catalog/registry.json"
            registry = json.loads(file.read_text(encoding="utf-8"))
            registry["components"].append(registry["components"][0])
            file.write_text(json.dumps(registry), encoding="utf-8")
            with self.assertRaisesRegex(contract.CatalogError, "Duplicate"):
                contract.validate_catalog(root)

    def test_port_requires_unit_axis(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["ports"][0]["axis"] = [0, 0, 0]
        with self.assertRaisesRegex(contract.CatalogError, "unit vector"):
            contract.validate_component(record, self.ctx)

    def test_thread_pitch_must_fit_bolt(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["parameters"]["thread_pitch_mm"] = 50.0
        with self.assertRaisesRegex(contract.CatalogError, "impossible illustrative bolt"):
            contract.validate_component(record, self.ctx)

    def test_thread_port_must_match_dimensions(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["ports"][0]["interface"] = "METRIC:M14x2:external"
        with self.assertRaisesRegex(contract.CatalogError, "thread designation"):
            contract.validate_component(record, self.ctx)

    def test_property_class_is_not_material_grade(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["material"]["grade"] = "8.8"
        with self.assertRaisesRegex(contract.CatalogError, "not be stored as steel grade"):
            contract.validate_component(record, self.ctx)

    def test_unregistered_coating_rejected(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["coating_id"] = "PAINT.MYSTERY.GREEN"
        with self.assertRaisesRegex(contract.CatalogError, "coating not registered"):
            contract.validate_component(record, self.ctx)

    def test_unregistered_standard_edition_rejected(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["standards"][0]["edition"] = "2099"
        with self.assertRaisesRegex(contract.CatalogError, "unregistered standard"):
            contract.validate_component(record, self.ctx)

    def test_metadata_only_cannot_verify_dimensions(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["provenance"]["field_sources"][0]["verification"] = "verified"
        with self.assertRaisesRegex(contract.CatalogError, "metadata-only"):
            contract.validate_component(record, self.ctx)

    def test_unreviewed_approved_status_rejected(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["status"] = "project_approved"
        with self.assertRaisesRegex(contract.CatalogError, "source ID and reviewer"):
            contract.validate_component(record, self.ctx)

    def test_numeric_parameters_must_use_units(self) -> None:
        record = copy.deepcopy(self.bolt)
        record["parameters"]["unlabeled_length"] = 12
        with self.assertRaisesRegex(contract.CatalogError, "unit suffix"):
            contract.validate_component(record, self.ctx)

    def test_pipe_wall_and_bore_must_agree(self) -> None:
        record = copy.deepcopy(self.pipe)
        record["parameters"]["wall_thickness_mm"] = 20
        with self.assertRaisesRegex(contract.CatalogError, "OD/ID/wall inconsistent"):
            contract.validate_component(record, self.ctx)

    def test_pipe_length_matches_port_separation(self) -> None:
        record = copy.deepcopy(self.pipe)
        record["ports"][1]["position_mm"][0] = 600
        with self.assertRaisesRegex(contract.CatalogError, "port separation"):
            contract.validate_component(record, self.ctx)

    def test_unknown_fluid_cannot_claim_lab_measurements(self) -> None:
        fluid = copy.deepcopy(self.ctx["fluids"]["FLUID.MGO.GRADE_UNASSIGNED"])
        fluid["properties"]["density_vs_temperature"]["source_verified"] = True
        with self.assertRaisesRegex(contract.CatalogError, "cannot pretend"):
            contract.validate_fluid(fluid, self.ctx)

    def test_system_rejects_fictitious_fluid_id(self) -> None:
        system = copy.deepcopy(self.ctx["systems"]["SYS.LO.DEMO.001"])
        system["edges"][0]["medium_id"] = "FLUID.NONEXISTENT"
        with self.assertRaisesRegex(contract.CatalogError, "registered fluid"):
            contract.validate_system(system, self.ctx)

    def test_assembly_exploded_prerequisites_must_precede(self) -> None:
        asm = copy.deepcopy(self.ctx["assemblies"]["ASM.LO.PUMP.SKID.DEMO.001"])
        asm["exploded_steps"][0]["prerequisites"] = ["pump"]
        with self.assertRaisesRegex(contract.CatalogError, "must exist and precede"):
            contract.validate_assembly(asm, self.ctx)


if __name__ == "__main__":
    unittest.main()
