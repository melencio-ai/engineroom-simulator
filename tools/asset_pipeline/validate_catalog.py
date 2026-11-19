#!/usr/bin/env python3
"""Validate versioned Engine Watch catalog contracts and their semantics.

No standards dimensions are certified by these checks. Metadata-only standards
references cannot upgrade a part to verified engineering status.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[2]
KINDS = {
    "components": ("component.schema.json", "variants"),
    "assemblies": ("assembly.schema.json", "assemblies"),
    "systems": ("system.schema.json", "systems"),
    "fluids": ("fluid.schema.json", "fluids"),
}
SUFFIXES = (
    "_mm", "_mm2", "_kg", "_bar", "_rpm", "_c", "_a",
    "_v", "_kw", "_percent", "_count", "_micrometre",
)
APPROVED = {"catalog_documented", "project_approved", "simulation_calibrated"}


class CatalogError(ValueError):
    """A structural or semantic catalog inconsistency."""


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise CatalogError(reason)


def read_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise CatalogError(f"{path}: {exc}") from exc


def check_unique(items: list[str], description: str) -> None:
    require(len(set(items)) == len(items), f"Duplicate {description}")


def context_from(root: Path) -> dict:
    sources = read_json(root / "catalog/standards/registry.json")["records"]
    evidence = read_json(root / "catalog/standards/evidence_registry.json")["records"]
    materials = read_json(root / "catalog/materials/registry.json")
    families = read_json(root / "catalog/families/registry.json")["families"]
    policy = read_json(root / "catalog/standards/confidence_policy.json")
    for kind, items in (
        ("standards source ID", [s["id"] for s in sources]),
        ("approved evidence ID", [e["source_id"] for e in evidence]),
        ("physical material ID", [m["id"] for m in materials["materials"]]),
        ("coating ID", [m["id"] for m in materials["coatings"]]),
        ("visual finish ID", [m["id"] for m in materials["visual_finishes"]]),
        ("part family", [f["id"] for f in families]),
    ):
        check_unique(items, kind)
    require(policy["schema_version"] == "1.0.0", "Confidence policy version mismatch")
    require(all(s["license_status"] == "metadata_only"
                and not s["dimensional_data_verified"] for s in sources),
            "Starter source registry may only claim metadata-level provenance")
    return {
        "standards": {s["id"]: s for s in sources},
        "evidence": {e["source_id"]: e for e in evidence},
        "materials": {m["base_material"]: m for m in materials["materials"]},
        "coatings": {m["id"]: m for m in materials["coatings"]},
        "families": {f["id"]: f for f in families},
        "components": {},
        "assemblies": {},
        "systems": {},
        "fluids": {},
    }


def validate_component(part: dict, ctx: dict) -> None:
    ident = part["id"]
    family_id = part["family"]
    require(family_id in ctx["families"], f"{ident}: unregistered family {family_id}")
    family = ctx["families"][family_id]
    params = part["parameters"]
    require(part["generator"]["name"] == family["generator"],
            f"{ident}: generator does not match family")
    require(part["generator"]["implementation_status"] == family["implementation_status"],
            f"{ident}: do not mark an unimplemented generator as available")
    for key in family["required_numeric_parameters"]:
        value = params.get(key)
        require(type(value) in (float, int) and math.isfinite(value) and value > 0,
                f"{ident}: {key} must be positive finite numeric data")
    for key, value in params.items():
        if isinstance(value, bool):
            continue
        if type(value) in (float, int):
            require(math.isfinite(value) and key.endswith(SUFFIXES),
                    f"{ident}: numeric parameter {key} lacks an approved unit suffix")
    mat = part["material"]
    require(mat["base_material"] in ctx["materials"],
            f"{ident}: physical material not registered")
    if part["coating_id"] is not None:
        require(part["coating_id"] in ctx["coatings"],
                f"{ident}: coating not registered")
    # Fastener property class is not a grade of steel and not a coating.
    if "property_class" in params:
        require(mat["grade"] != params["property_class"],
                f"{ident}: fastener property class must not be stored as steel grade")
    check_unique([p["id"] for p in part["ports"]], f"{ident} ports")
    kinds = {p["kind"] for p in part["ports"]}
    for kind in family["required_ports"]:
        require(kind in kinds, f"{ident}: missing {kind} connection port")
    for port in part["ports"]:
        axis = port["axis"]
        require(all(math.isfinite(x) for x in axis) and
                abs(math.sqrt(sum(x*x for x in axis)) - 1.0) < 0.001,
                f"{ident}.{port['id']}: port axis must be a unit vector")
        require(all(math.isfinite(x) for x in port["position_mm"]),
                f"{ident}.{port['id']}: port position invalid")
    source_index = {(s["code"], s["edition"]): s for s in ctx["standards"].values()}
    for ref in part["standards"]:
        index_key = (ref["code"], ref["edition"])
        require(index_key in source_index,
                f"{ident}: unregistered standard {ref['code']}:{ref['edition']}")
        registered = source_index[index_key]
        require(ref["url"] == registered["url"],
                f"{ident}: standard URL differs from registered official metadata")
    prov = part["provenance"]
    for field in prov["field_sources"]:
        require(field["parameter"] in params, f"{ident}: unknown sourced parameter")
        require(field["source_id"] in ctx["standards"],
                f"{ident}: unknown source reference {field['source_id']}")
        require(field["verification"] != "verified",
                f"{ident}: metadata-only source cannot verify a dimension")
    if part["status"] in APPROVED:
        require(prov["verified"] and prov["source_id"] and prov["reviewer"],
                f"{ident}: approved status requires source ID and reviewer")
        require(prov["source_id"] in ctx["evidence"],
                f"{ident}: unregistered verified evidence ID; a public metadata page is not enough")
        evidence_record = ctx["evidence"][prov["source_id"]]
        require(evidence_record.get("permitted_use") == "approved_catalog_values"
                and evidence_record.get("reviewer") == prov["reviewer"],
                f"{ident}: engineering claims require approved use rights and matching reviewer")
    else:
        require(not prov["verified"], f"{ident}: illustrative part cannot claim verification")
    if family_id == "fastener.hex_bolt":
        d, pitch, length = (params[k] for k in
                            ("nominal_diameter_mm", "thread_pitch_mm", "length_mm"))
        require(pitch < d and length >= d, f"{ident}: impossible illustrative bolt dimensions")
        thread_ports = [p for p in part["ports"] if p["kind"] == "thread"]
        require(len(thread_ports) == 1, f"{ident}: expected one threaded interface")
        require(thread_ports[0]["interface"] == f"METRIC:M{d:g}x{pitch:g}:external",
                f"{ident}: bolt port thread designation mismatches dimensions")
    elif family_id == "piping.pipe_spool":
        outside = params["outside_diameter_mm"]
        inside = params["inside_diameter_mm"]
        wall = params["wall_thickness_mm"]
        length = params["length_mm"]
        require(outside > 2*wall and abs(inside-(outside-2*wall)) < 0.001,
                f"{ident}: OD/ID/wall inconsistent")
        endpoints = [p for p in part["ports"] if p["kind"] == "pipe_end"]
        require(len(endpoints) == 2, f"{ident}: pipe requires two end ports")
        require(endpoints[0]["interface"] == endpoints[1]["interface"],
                f"{ident}: both spool ends must share an interface")
        require(abs(abs(endpoints[0]["position_mm"][0]-endpoints[1]["position_mm"][0])-length) < 0.001,
                f"{ident}: spool port separation must match its length")
        require(all(abs(a+b)<0.001 for a,b in zip(endpoints[0]["axis"],endpoints[1]["axis"])),
                f"{ident}: pipe end port axes must oppose each other")


def validate_fluid(fluid: dict, ctx: dict) -> None:
    props = fluid["properties"]
    for name in ("density_vs_temperature", "viscosity_vs_temperature"):
        curve = props[name]
        if curve["model"] == "unknown":
            require(not curve["samples"] and not curve["source_verified"],
                    f"{fluid['id']}: unknown {name} curve cannot pretend to be sourced")
        else:
            samples = curve["samples"]
            require(len(samples) >= 2,
                    f"{fluid['id']}: tabulated {name} needs at least two samples")
            temps = [p["temperature_c"] for p in samples]
            require(temps == sorted(set(temps)),
                    f"{fluid['id']}: curve temperature entries must be ordered/unique")
    if fluid["status"] == "illustrative" and fluid["standard_grade"] is None:
        require(props["sulfur_mass_percent"] is None,
                f"{fluid['id']}: do not invent sulfur value for unassigned grade")


def validate_assembly(asm: dict, ctx: dict) -> None:
    ident = asm["id"]
    children = {p["instance_id"]: p for p in asm["children"]}
    check_unique([p["instance_id"] for p in asm["children"]], f"{ident} child IDs")
    check_unique([p["id"] for p in asm["connections"]], f"{ident} connections")
    check_unique([p["step"] for p in asm["exploded_steps"]], f"{ident} exploded step numbers")
    steps = {p["instance_id"]: p["step"] for p in asm["exploded_steps"]}
    for child in children.values():
        ref = child["component_id"]
        if ref not in ctx["components"]:
            require(asm["status"] == "illustrative" and ref.endswith(".TODO"),
                    f"{ident}: unregistered component {ref}; only explicit draft TODOs allowed")
    for edge in asm["connections"]:
        for endpoint in ("from", "to"):
            instance, port_id = edge[endpoint].split(".", 1)
            require(instance in children, f"{ident}: unknown assembly instance {instance}")
            comp_id = children[instance]["component_id"]
            if comp_id in ctx["components"]:
                ports = {p["id"]: p for p in ctx["components"][comp_id]["ports"]}
                require(port_id in ports and ports[port_id]["kind"] == edge["kind"],
                        f"{ident}: incompatible or absent port {edge[endpoint]}")
    for step in asm["exploded_steps"]:
        require(step["instance_id"] in children, f"{ident}: exploded instance missing")
        for prereq in step["prerequisites"]:
            require(prereq in steps and steps[prereq] < step["step"],
                    f"{ident}: exploded prerequisites must exist and precede the step")
    require(asm["status"] == "illustrative" or asm["provenance"]["verified"],
            f"{ident}: non-illustrative assembly must have verified reference")
    # G2b checks spatial mating positions, shaft engagement and joint fit.
    # The G1 placeholder assembly intentionally remains unresolved/draft.
    from assembly_builder import validate_mated_assembly
    validate_mated_assembly(asm, ctx["components"])


def validate_system(sys: dict, ctx: dict) -> None:
    ident = sys["id"]
    nodes = [n["id"] for n in sys["nodes"]]
    check_unique(nodes, f"{ident} node IDs")
    check_unique([e["id"] for e in sys["edges"]], f"{ident} edge IDs")
    for edge in sys["edges"]:
        require(edge["from"] in nodes and edge["to"] in nodes and edge["from"] != edge["to"],
                f"{ident}: edge {edge['id']} connects invalid or identical nodes")
        if edge["kind"] == "fluid":
            require(sys["domain"] in ("fluid","mixed") and
                    edge["medium_id"] in ctx["fluids"],
                    f"{ident}: fluid edge must reference a registered fluid")
        else:
            require(edge["medium_id"] is None, f"{ident}: nonfluid edge has fluid medium")
    require(sys["status"] == "illustrative" or sys["provenance"]["verified"],
            f"{ident}: verified network requires a verified source")


def validate_catalog(root: Path = ROOT) -> dict:
    registry = read_json(root / "catalog/registry.json")
    require(registry["schema_version"] == "1.0.0", "Catalog registry version mismatch")
    ctx = context_from(root)
    ids = []
    for kind, (schema_name, folder) in KINDS.items():
        paths = registry[kind]
        check_unique(paths, f"{kind} registry paths")
        expected = {
            p.relative_to(root).as_posix() for p in
            (root / "catalog" / folder).glob("*.json")
        }
        require(set(paths) == expected,
                f"{kind}: missing/unregistered files: {sorted(set(paths)^expected)}")
        schema = read_json(root / "schemas" / schema_name)
        Draft202012Validator.check_schema(schema)
        validator = Draft202012Validator(schema)
        for rel in paths:
            relative = Path(rel)
            require(not relative.is_absolute() and ".." not in relative.parts and
                    relative.parts[:2] == ("catalog", folder),
                    f"Unsafe catalog registry path: {rel}")
            data = read_json(root / relative)
            errs = list(validator.iter_errors(data))
            require(not errs,
                    f"{rel}: schema errors: {'; '.join(e.message for e in errs)}")
            require(data["id"] == relative.stem,
                    f"{rel}: filename and stable ID must match")
            require(data["id"] not in ids, f"Duplicate catalog identifier {data['id']}")
            ids.append(data["id"])
            ctx[kind][data["id"]] = data
    for comp in ctx["components"].values():
        validate_component(comp, ctx)
    for fluid in ctx["fluids"].values():
        validate_fluid(fluid, ctx)
    for asm in ctx["assemblies"].values():
        validate_assembly(asm, ctx)
    for system in ctx["systems"].values():
        validate_system(system, ctx)
    return {kind: len(ctx[kind]) for kind in KINDS}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    counts = validate_catalog(args.root)
    print("Catalog contract PASS: " +
          ", ".join(f"{count} {kind}" for kind, count in counts.items()))
    print("Dimensional claims remain illustrative and unverified; no CAD meshes were generated.")


if __name__ == "__main__":
    main()
