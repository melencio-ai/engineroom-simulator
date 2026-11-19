# Marine Component Master Library — G1 catalog foundation

**Branch:** feature/catalog-v1-schema-provenance  
**Baseline:** Engine Watch 3D v0.7.1 (main)  
**Status:** Schema and data contracts only. No PSP 3D geometry or pump simulation changes.

The implementation extends the earlier blueprint into versioned catalog records and deterministic validation. It does not certify engineering dimensions, real-world installation safety, or OEM-specific systems.

## Delivered

- Four JSON Schema Draft 2020-12 definitions: **component**, **assembly**, **system**, and **fluid**.
- A manifest of stable component IDs, assembly IDs, system IDs and fluid IDs.
- Separate **physical materials**, **coatings** and **visual finishes**.
- An ISO/ASME standards-reference registry containing public metadata ONLY, and a confidence/provenance policy.
- Two distinct illustrative metric hex-head bolts (M12×40 / M10×30, each with property class 8.8 separated from steel grade).
- One illustrative DN50 steel pipe spool with explicit OD, wall, bore, length, nominal size and mating interfaces.
- Two unspecified fluid profiles, one draft LO motor/pump assembly and one conceptual lubricating-oil flow graph.
- A semantic validator and negative regression tests that reject inconsistent or falsely verified records.
- A dedicated GitHub Actions **catalog-contract** job, alongside existing PSP / PC / PPSSPP checks.

## Validate

From repository root:

    python -m pip install "jsonschema>=4.20,<5"
    python tools/asset_pipeline/validate_catalog.py
    python -m unittest discover -s tests/catalog -p 'test_*.py' -v

The validator returns nonzero on errors. It validates source and model contracts, **not** pressure containment, statutory compliance, dimensional certification, or system calibration.

## Repository architecture

    schemas/
      component.schema.json
      assembly.schema.json
      system.schema.json
      fluid.schema.json
    catalog/
      registry.json
      standards/registry.json
      standards/confidence_policy.json
      materials/registry.json
      families/registry.json
      variants/             # two bolts and one illustrative pipe spool
      assemblies/           # explicitly unresolved conceptual skid
      systems/              # non-OEM conceptual LO network
      fluids/               # unspecified LO and MGO
    tools/asset_pipeline/validate_catalog.py
    tests/catalog/test_contract.py

Each active file must appear exactly once in the registry and must have a filename matching its stable ID. Additional files in a tracked category without registration fail validation.

## Engineering-data rules

1. All geometry-related dimensions and local port coordinates are expressed in **mm**; the current engine room World3D uses **metres**. Coordinate conversion belongs to the future exporter.
2. A nominal pipe DN is not an actual OD, wall thickness, or pipe schedule. The example OD 60 mm and wall 4 mm values are **unverified placeholders**, not a claim to ASME B36.10 compliance.
3. The pipe bore must satisfy **ID = OD - 2 × wall**; port positions and axes must agree with the spool length.
4. Thread interfaces encode actual displayed diameter and pitch; property class 8.8 is **not** a steel grade.
5. Green epoxy is a **finish**, not a structural material. Each part distinguishes base material, grade, coating and visual presentation.
6. Each part references an explicit family, generator name and revision. The generator status is **planned** until G2 actually implements its geometry.
7. A standards landing page establishes document scope and edition but **does not verify a dimensional value**. All initial standard sources have metadata-only licenses and cannot upgrade a part to a verified status.
8. Parts/assemblies are **illustrative** until supported by approved evidence. A draft part reference ending in .TODO is allowed only inside an illustrative assembly.
9. Unassigned MGO/LO fluids retain unknown viscosity, density, flash point and sulfur measurements. Don't infer fuel properties from a commercial abbreviation.
10. The catalog must not replace the existing 33 PSP archetypes, their 16 finishes, five-deck collisions, or Missions 001/002 until the generated replacement is validated.

## Follow-up gates

**G2: Parametric generators.** First implement bolt, nut, washer, pipe, flange and gasket families driven by licensed/OEM or clearly illustrative dimensions. Emit master inspection models and optimized PSP LODs from one ID.

**G3: Deck construction.** Derive rendered floor geometry and physical collision from a shared deck-cell data model. The prior disappearing-floor regression should become an automated camera-path test.

**G4: LO pump assembly.** Connect sourced parts to actual assemblies, with inspection/exploded steps, tagged fasteners, connected pipe spools and starter panel; preserve gameplay.

**Release gate:** Catalog-contract tests plus existing native simulation, desktop build, PSP EBOOT and PPSSPP smoke. Human review and explicit user approval before merging to main.
