"""G2b engineering-aware mating/placement validation for educational assemblies.

The checks verify only geometric/metadata consistency for illustrative records.
They DO NOT certify pressure rating, bolt preload, material compatibility, weld
integrity, thread tolerances, class rules or maintenance safety.
"""
from __future__ import annotations

import math
from collections import Counter

Vec = tuple[float, float, float]


class AssemblyFitError(ValueError):
    pass


def must(condition: bool, message: str) -> None:
    if not condition:
        raise AssemblyFitError(message)


def vadd(a: Vec, b: Vec) -> Vec:
    return (a[0]+b[0],a[1]+b[1],a[2]+b[2])


def vsub(a: Vec, b: Vec) -> Vec:
    return (a[0]-b[0],a[1]-b[1],a[2]-b[2])


def dot(a: Vec, b: Vec) -> float:
    return sum(x*y for x,y in zip(a,b))


def norm(a: Vec) -> float:
    return math.sqrt(dot(a,a))


def transform_vector(v: Vec, rotation_deg: list[float]) -> Vec:
    """Rotate X then Y then Z, degrees. Used for both geometry and ports."""
    x,y,z=v
    a,b,c=[math.radians(d) for d in rotation_deg]
    y,z = y*math.cos(a)-z*math.sin(a),y*math.sin(a)+z*math.cos(a)
    x,z = x*math.cos(b)+z*math.sin(b),-x*math.sin(b)+z*math.cos(b)
    x,y = x*math.cos(c)-y*math.sin(c),x*math.sin(c)+y*math.cos(c)
    return (x,y,z)


def local_to_world(v: Vec, child: dict, explode: Vec = (0,0,0)) -> Vec:
    return vadd(vadd(transform_vector(v,child["rotation_deg"]),
                    tuple(child["translation_mm"])),explode)


def component_ports(comp: dict) -> dict:
    return {p["id"]:p for p in comp["ports"]}


def endpoint(parts: dict, children: dict, value: str) -> tuple[dict,dict,dict,Vec,Vec]:
    inst,port_id=value.split(".",1)
    must(inst in children,f"Unknown instance {inst}")
    child=children[inst]
    comp=parts[child["component_id"]]
    ports=component_ports(comp)
    must(port_id in ports, f"Missing port {value}")
    port=ports[port_id]
    loc=local_to_world(tuple(port["position_mm"]),child)
    axis=transform_vector(tuple(port["axis"]),child["rotation_deg"])
    return child,comp,port,loc,axis


def assert_near(a: float,b: float,tolerance: float,reason: str) -> None:
    must(math.isfinite(a) and math.isfinite(b) and
         abs(a-b) <= tolerance+0.00001,
         f"{reason}: {a:.4f} vs {b:.4f} (tol={tolerance})")


def check_connections(asm: dict,parts: dict) -> None:
    children={c["instance_id"]:c for c in asm["children"]}
    for edge in asm["connections"]:
        a,ac,ap,apos,axis=endpoint(parts,children,edge["from"])
        b,bc,bp,bpos,baxis=endpoint(parts,children,edge["to"])
        kind=edge["kind"]
        must(ap["kind"]==kind and bp["kind"]==kind,
             f"{edge['id']}: port kind differs")
        must(ap["service"]==bp["service"],
             f"{edge['id']}: incompatible service mediums")
        if kind=="thread":
            must(ap["interface"].endswith(":external") and
                 bp["interface"].endswith(":internal"),
                 f"{edge['id']}: expected male-to-female threaded pair")
            must(ap["interface"].rsplit(":",1)[0] ==
                 bp["interface"].rsplit(":",1)[0],
                 f"{edge['id']}: thread nominal diameter/pitch mismatch")
            must(ac["family"]=="fastener.hex_bolt" and
                 bc["family"]=="fastener.hex_nut",
                 f"{edge['id']}: unsupported threaded connection family")
            must(abs(dot(axis,baxis))>0.999,
                 f"{edge['id']}: nut and bolt bore axes not coaxial")
            shank_origin=tuple(a["translation_mm"])
            direction=axis
            between=vsub(tuple(b["translation_mm"]),shank_origin)
            radial=norm(vsub(between,tuple(dot(between,direction)*x
                                          for x in direction)))
            must(radial<=0.1,
                 f"{edge['id']}: off-axis fastener centerline")
            length=ac["parameters"]["length_mm"]
            nut_t=bc["parameters"]["nut_thickness_mm"]
            nut_mid=dot(between,direction)
            must(nut_mid - nut_t/2 >= 0 and
                 nut_mid + nut_t/2 <= length,
                 f"{edge['id']}: nut lies outside usable shank")
            must(nut_mid-nut_t/2 >= length-ac["parameters"]["thread_length_mm"],
                 f"{edge['id']}: nut not inside threaded engagement region")
        else:
            must(ap["interface"]==bp["interface"],
                 f"{edge['id']}: connection interface mismatch")
            must(norm(vsub(apos,bpos))<=0.1,
                 f"{edge['id']}: pipe/flange ports do not physically coincide")
            must(dot(axis,baxis)<-0.999,
                 f"{edge['id']}: port axes must face each other")


def validate_flanged_joint(asm: dict,parts: dict) -> dict:
    fit=asm["fit_spec"]
    name=asm["id"]
    must(asm["status"]=="illustrative" and
         fit["engineering_approved"] is False,
         f"{name}: no unverified flanged joint may be claimed approved")
    children={c["instance_id"]:c for c in asm["children"]}
    tol=fit["axial_tolerance_mm"]
    must(tol<=0.5,f"{name}: use submillimetre contact tolerance")
    axis=tuple(fit["joint_axis"])
    must(norm(axis)>0.999 and norm(axis)<1.001,
         f"{name}: joint axis must be unit-length")
    # The current factory constructs joints along +X in the master model.
    must(abs(dot(axis,(1,0,0))-1)<0.001,
         f"{name}: current joint constructor expects positive-X axis")
    ids=fit["flanges"]+fit["pipes"]+[fit["gasket"]]
    must(len(set(ids))==5 and all(x in children for x in ids),
         f"{name}: five distinct flanges, pipes and gasket instances required")
    l,r=[children[x] for x in fit["flanges"]]
    lp,rp=[children[x] for x in fit["pipes"]]
    gasket=children[fit["gasket"]]
    lf,rf=[parts[x["component_id"]] for x in (l,r)]
    lpp,rpp=[parts[x["component_id"]] for x in (lp,rp)]
    gp=parts[gasket["component_id"]]
    must(lf["family"]==rf["family"]=="piping.weld_neck_flange" and
         lpp["family"]==rpp["family"]=="piping.pipe_spool" and
         gp["family"]=="piping.ring_gasket",
         f"{name}: fitting types are not flange/pipe/gasket")
    must(not any(c["maintainable"] for c in (l,r,lp,rp)),
         f"{name}: weld-neck flange and pipe pairs must not be removable")
    must(gasket["maintainable"],f"{name}: gasket must be removable")
    for f in (lf,rf):
        must(f["parameters"]["pressure_class"]=="unassigned",
             f"{name}: illustrative joint cannot silently claim rated pressure")
        must(f["parameters"]["bolt_hole_count"]==len(fit["fasteners"]),
             f"{name}: fastener number must exactly match flange drilling")
        must(f["parameters"]["bore_diameter_mm"]==
             lpp["parameters"]["inside_diameter_mm"]==
             rpp["parameters"]["inside_diameter_mm"],
             f"{name}: flange and pipe clear bores do not match")
        must(f["parameters"]["neck_outside_diameter_mm"]==
             lpp["parameters"]["outside_diameter_mm"]==
             rpp["parameters"]["outside_diameter_mm"],
             f"{name}: weld neck and pipe OD mismatch")
    must(gp["parameters"]["inner_diameter_mm"]==
         lf["parameters"]["bore_diameter_mm"],
         f"{name}: gasket obstructs the pipe clear bore")
    pcd=lf["parameters"]["bolt_circle_diameter_mm"]
    must(pcd==rf["parameters"]["bolt_circle_diameter_mm"],
         f"{name}: flange bolt circles mismatch")
    must(lf["parameters"]["bolt_hole_diameter_mm"]==
         rf["parameters"]["bolt_hole_diameter_mm"],
         f"{name}: flange hole sizes mismatch")
    must(gp["parameters"]["outer_diameter_mm"]/2 <
         pcd/2 - lf["parameters"]["bolt_hole_diameter_mm"]/2,
         f"{name}: gasket overlaps bolt drilling")
    lface=local_to_world(tuple(component_ports(lf)["gasket_face"]["position_mm"]),l)[0]
    rface=local_to_world(tuple(component_ports(rf)["gasket_face"]["position_mm"]),r)[0]
    gc=tuple(gasket["translation_mm"])[0]
    gth=gp["parameters"]["thickness_mm"]
    must(lface<rface,f"{name}: flange facing orientation reversed")
    assert_near(rface-lface,gth,tol,f"{name}: gasket axial gap")
    assert_near(gc,(lface+rface)/2,tol,f"{name}: gasket midplane")
    left_back=lface-lf["parameters"]["flange_thickness_mm"]
    right_back=rface+rf["parameters"]["flange_thickness_mm"]
    min_protrusion=fit["minimum_thread_protrusion_mm"]
    used_holes=[]
    for row in fit["fasteners"]:
        i=row["hole_index"]
        used_holes.append(i)
        ids4=[row[n] for n in ("bolt","nut","near_washer","far_washer")]
        must(len(set(ids4))==4 and all(x in children for x in ids4),
             f"{name}: fastener set {i} references distinct valid instances")
        bolt,nut,near,far=[children[x] for x in ids4]
        bc,nc,nwc,fwc=[parts[x["component_id"]] for x in (bolt,nut,near,far)]
        must(bc["family"]=="fastener.hex_bolt" and
             nc["family"]=="fastener.hex_nut" and
             nwc["family"]==fwc["family"]=="fastener.flat_washer",
             f"{name}: wrong component type in bolt set {i}")
        dp=bc["parameters"]["nominal_diameter_mm"]
        must(dp==nc["parameters"]["nominal_diameter_mm"]==
             nwc["parameters"]["nominal_diameter_mm"]==
             fwc["parameters"]["nominal_diameter_mm"],
             f"{name}: fastener diameters incompatible")
        must(nc["parameters"]["thread_pitch_mm"]==
             bc["parameters"]["thread_pitch_mm"],
             f"{name}: nut/bolt pitches incompatible")
        for w in (nwc,fwc):
            must(w["parameters"]["inner_diameter_mm"]>=dp+0.5 and
                 w["parameters"]["outer_diameter_mm"]>
                 w["parameters"]["inner_diameter_mm"],
                 f"{name}: washer cannot clear bolt")
        hdiam=lf["parameters"]["bolt_hole_diameter_mm"]
        must(dp+0.5<=hdiam<=dp+4.0,
             f"{name}: bolt-to-flange hole clearance unrealistic")
        theta=2*math.pi*i/len(fit["fasteners"])
        expected=(pcd/2*math.cos(theta),pcd/2*math.sin(theta))
        for c in (bolt,nut,near,far):
            cy,cz=c["translation_mm"][1:3]
            assert_near(cy,expected[0],tol,f"{name}: bolt circle Y {i}")
            assert_near(cz,expected[1],tol,f"{name}: bolt circle Z {i}")
        xbolt=bolt["translation_mm"][0]
        xn=nut["translation_mm"][0]
        xnear=near["translation_mm"][0]
        xfar=far["translation_mm"][0]
        wn=nwc["parameters"]["thickness_mm"]
        wf=fwc["parameters"]["thickness_mm"]
        nt=nc["parameters"]["nut_thickness_mm"]
        assert_near(xnear+wn/2,left_back,tol,f"{name}: left washer seats on flange")
        assert_near(xnear-wn/2,xbolt,tol,f"{name}: bolt head seats on washer")
        assert_near(xfar-wf/2,right_back,tol,f"{name}: right washer seats on flange")
        assert_near(xn-nt/2,xfar+wf/2,tol,f"{name}: nut seats on washer")
        must(bolt["rotation_deg"]==[0,-90,0] and
             nut["rotation_deg"]==[0,90,0],
             f"{name}: fastener must run in positive-X direction")
        assert_near(xbolt+bc["parameters"]["length_mm"]-(xn+nt/2),
                    min_protrusion+1.0,1.01,
                    f"{name}: threaded shank protrusion")
        must(xbolt+bc["parameters"]["length_mm"]-(xn+nt/2)>=min_protrusion-tol,
             f"{name}: threaded shank too short")
    must(sorted(used_holes)==list(range(len(fit["fasteners"]))),
         f"{name}: duplicate or missing flange hole positions")
    # Welded pipe/flange pieces move together during the visualization.
    exploded={x["instance_id"]:x["translation_mm"] for x in asm["exploded_steps"]}
    if fit["pipes"][1] in exploded or fit["flanges"][1] in exploded:
        must(exploded.get(fit["pipes"][1])==exploded.get(fit["flanges"][1]),
             f"{name}: welded pipe/flange must remain a single moving assembly")
    return {"joint":name,"fastener_sets":len(fit["fasteners"]),
            "nominal_size":lf["parameters"]["nominal_size"],
            "bolt_circle_diameter_mm":pcd,
            "certified":False}


def validate_mated_assembly(asm: dict,parts: dict) -> dict:
    all_resolved=all(c["component_id"] in parts for c in asm["children"])
    if not all_resolved:
        must(asm["status"]=="illustrative" and "fit_spec" not in asm,
             f"{asm['id']}: fitted joint requires registered components")
        return {"assembly":asm["id"],"resolved":False}
    check_connections(asm,parts)
    if "fit_spec" in asm:
        result=validate_flanged_joint(asm,parts)
        result["resolved"]=True
        return result
    return {"assembly":asm["id"],"resolved":True}


def bill_of_materials(asm: dict) -> list[dict]:
    quantities=Counter(child["component_id"] for child in asm["children"])
    return [{"component_id":part_id,"quantity":amount}
            for part_id,amount in sorted(quantities.items())]
