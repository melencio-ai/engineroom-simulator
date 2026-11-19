"""Deterministic parametric inspection and PSP-LOD meshes, in millimetres.

Dimensions come ONLY from validated catalog records. The current sample records
are illustrative. These are independent polygonal geometry generators, not
certified CAD, pressure-component design or thread fit specifications.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field

PI = math.pi
TAU = 2.0 * PI
Vec = tuple[float, float, float]
Tri = tuple[Vec, Vec, Vec]


@dataclass
class Mesh:
    triangles: list[Tri] = field(default_factory=list)

    def tri(self, a: Vec, b: Vec, c: Vec) -> None:
        ab = tuple(b[i] - a[i] for i in range(3))
        ac = tuple(c[i] - a[i] for i in range(3))
        cross = (
            ab[1] * ac[2] - ab[2] * ac[1],
            ab[2] * ac[0] - ab[0] * ac[2],
            ab[0] * ac[1] - ab[1] * ac[0],
        )
        if sum(x*x for x in cross) < 1e-16:
            return  # Skip degenerate triangles at exact circular endpoints.
        self.triangles.append((a, b, c))

    def quad(self, a: Vec, b: Vec, c: Vec, d: Vec) -> None:
        self.tri(a, b, c)
        self.tri(a, c, d)

    @property
    def triangle_count(self) -> int:
        return len(self.triangles)

    def bounds(self) -> tuple[Vec, Vec]:
        points = (p for triangle in self.triangles for p in triangle)
        vertices = list(points)
        return (
            tuple(min(v[i] for v in vertices) for i in range(3)),
            tuple(max(v[i] for v in vertices) for i in range(3)),
        )


def require_positive(*values: float) -> None:
    if not all(type(x) in (float, int) and math.isfinite(x) and x > 0 for x in values):
        raise ValueError("Parametric dimensions must be positive finite values")


def point_z(radius: float, angle: float, z: float) -> Vec:
    return (radius * math.cos(angle), radius * math.sin(angle), z)


def point_x(radius: float, angle: float, x: float) -> Vec:
    return (x, radius * math.cos(angle), radius * math.sin(angle))


def annulus_z(mesh: Mesh, inner: float, outer: float, z0: float, z1: float,
              segments: int) -> None:
    require_positive(inner, outer)
    if not (outer > inner and z1 > z0 and segments >= 6):
        raise ValueError("Invalid annulus_z")
    for j in range(segments):
        a, b = TAU * j / segments, TAU * (j + 1) / segments
        ol, or_ = point_z(outer,a,z0), point_z(outer,b,z0)
        oh, ohr = point_z(outer,a,z1), point_z(outer,b,z1)
        il, ir = point_z(inner,a,z0), point_z(inner,b,z0)
        ih, ihr = point_z(inner,a,z1), point_z(inner,b,z1)
        mesh.quad(oh,ohr,ihr,ih)  # exposed top annulus
        mesh.quad(il,ir,or_,ol)  # exposed bottom annulus
        mesh.quad(ol,or_,ohr,oh) # outer cylindrical wall
        mesh.quad(ir,il,ih,ihr)  # inner bore wall


def annulus_x(mesh: Mesh, inner: float, outer: float, x0: float, x1: float,
              segments: int) -> None:
    require_positive(inner, outer)
    if not (outer > inner and x1 > x0 and segments >= 6):
        raise ValueError("Invalid annulus_x")
    for j in range(segments):
        a, b = TAU*j/segments, TAU*(j+1)/segments
        ol, or_ = point_x(outer,a,x0), point_x(outer,b,x0)
        oh, ohr = point_x(outer,a,x1), point_x(outer,b,x1)
        il, ir = point_x(inner,a,x0), point_x(inner,b,x0)
        ih, ihr = point_x(inner,a,x1), point_x(inner,b,x1)
        mesh.quad(oh,ohr,ihr,ih)
        mesh.quad(il,ir,or_,ol)
        mesh.quad(ol,or_,ohr,oh)
        mesh.quad(ir,il,ih,ihr)


def cylinder_z(mesh: Mesh, radius: float, z0: float, z1: float,
               segments: int, cap: bool = True) -> None:
    require_positive(radius)
    if z1 <= z0 or segments < 6:
        raise ValueError("Invalid cylinder")
    for j in range(segments):
        a, b = TAU*j/segments, TAU*(j+1)/segments
        low_a, low_b = point_z(radius,a,z0),point_z(radius,b,z0)
        top_a, top_b = point_z(radius,a,z1),point_z(radius,b,z1)
        mesh.quad(low_a,low_b,top_b,top_a)
        if cap:
            mesh.tri((0,0,z1),top_a,top_b)
            mesh.tri((0,0,z0),low_b,low_a)


def hex_outline(radius: float, samples_per_side: int) -> list[tuple[float,float]]:
    vertices = [
        (radius*math.cos(TAU*j/6),radius*math.sin(TAU*j/6))
        for j in range(6)
    ]
    result = []
    for j in range(6):
        a,b = vertices[j],vertices[(j+1)%6]
        for k in range(samples_per_side):
            t=k/samples_per_side
            result.append((a[0]*(1-t)+b[0]*t,a[1]*(1-t)+b[1]*t))
    return result


def bolt(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    d=float(p["nominal_diameter_mm"])
    length=float(p["length_mm"])
    pitch=float(p["thread_pitch_mm"])
    thread_len=float(p["thread_length_mm"])
    thread_depth=float(p["thread_depth_mm"])
    across_flats=float(p["head_across_flats_mm"])
    head_height=float(p["head_height_mm"])
    require_positive(d,length,pitch,thread_len,thread_depth,across_flats,head_height)
    if not (thread_len <= length and thread_depth < d/5 and
            across_flats > d and pitch < d):
        raise ValueError("Impossible illustrative bolt section")
    m=Mesh()
    seg=32 if detailed else 10
    radius=d/2
    root=radius-thread_depth if detailed else radius
    thread_top=-length+thread_len
    cylinder_z(m,root,-length,thread_top,seg)
    if thread_top < -1e-5:
        cylinder_z(m,radius,thread_top,0.0,seg)
    if detailed:
        # A real 3D helical relief, not painted black rings. Ridge cross
        # section is triangular but NOT an ISO 68-1 profile / tolerance fit.
        n=max(1,int(math.floor(thread_len/pitch)))
        steps=n*24
        for i in range(steps):
            def row(k: int) -> tuple[Vec,Vec,Vec]:
                theta=TAU*k/24
                center=-length + pitch*0.60 + k*(pitch/24)
                # The first and last triangles remain inside the shaft span.
                return (
                    point_z(root,theta,center-0.32*pitch),
                    point_z(radius,theta,center),
                    point_z(root,theta,center+0.32*pitch),
                )
            aa=row(i);bb=row(i+1)
            m.quad(aa[0],bb[0],bb[1],aa[1])
            m.quad(aa[1],bb[1],bb[2],aa[2])
    # Hexagonal, bevelled head with six non-collinear sides.
    head_r=across_flats/math.sqrt(3.0)
    a=hex_outline(head_r,1)
    upper=hex_outline(head_r*(0.94 if detailed else 1.0),1)
    cap=(0.0,0.0,head_height)
    for j in range(6):
        k=(j+1)%6
        p0=(a[j][0],a[j][1],0.0);p1=(a[k][0],a[k][1],0.0)
        t0=(a[j][0],a[j][1],head_height*0.84)
        t1=(a[k][0],a[k][1],head_height*0.84)
        b0=(upper[j][0],upper[j][1],head_height)
        b1=(upper[k][0],upper[k][1],head_height)
        m.quad(p0,p1,t1,t0)
        m.quad(t0,t1,b1,b0)
        m.tri(cap,b0,b1)
        m.tri((0,0,0),p1,p0)
    return m


def hex_nut(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    d=float(p["nominal_diameter_mm"])
    pitch=float(p["thread_pitch_mm"])
    bore=float(p["bore_diameter_mm"])
    thickness=float(p["nut_thickness_mm"])
    across=float(p["across_flats_mm"])
    depth=float(p["thread_depth_mm"])
    require_positive(d,pitch,bore,thickness,across,depth)
    if not (bore >= d and depth < bore/8 and across > bore):
        raise ValueError("Impossible illustrative nut")
    m=Mesh()
    nside=8 if detailed else 2
    polygon=hex_outline(across/math.sqrt(3.0),nside)
    n=len(polygon)
    z0,z1=-thickness/2,thickness/2
    bore_radius=bore/2
    for i in range(n):
        k=(i+1)%n
        a,b=polygon[i],polygon[k]
        theta0,theta1=TAU*i/n,TAU*k/n
        top_a=(a[0],a[1],z1);top_b=(b[0],b[1],z1)
        bot_a=(a[0],a[1],z0);bot_b=(b[0],b[1],z0)
        inner_top_a=point_z(bore_radius,theta0,z1)
        inner_top_b=point_z(bore_radius,theta1,z1)
        inner_bot_a=point_z(bore_radius,theta0,z0)
        inner_bot_b=point_z(bore_radius,theta1,z0)
        m.quad(top_a,top_b,inner_top_b,inner_top_a)
        m.quad(inner_bot_a,inner_bot_b,bot_b,bot_a)
        m.quad(bot_a,bot_b,top_b,top_a)
        m.quad(inner_bot_b,inner_bot_a,inner_top_a,inner_top_b)
    if detailed:
        # Interior helically-raised thread profile for inspection only.
        turns=max(1,int(math.floor(thickness/pitch)))
        steps=turns*24
        for i in range(steps):
            def row(k: int) -> tuple[Vec,Vec,Vec]:
                theta=TAU*k/24
                z=z0+pitch*0.55+k*pitch/24
                return (
                    point_z(bore_radius,theta,z-0.27*pitch),
                    point_z(bore_radius-depth,theta,z),
                    point_z(bore_radius,theta,z+0.27*pitch),
                )
            a,b=row(i),row(i+1)
            if a[0][2]<z0 or b[2][2]>z1:
                continue
            m.quad(a[0],a[1],b[1],b[0])
            m.quad(a[1],a[2],b[2],b[1])
    return m


def flat_washer(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    inner=float(p["inner_diameter_mm"])/2
    outer=float(p["outer_diameter_mm"])/2
    thickness=float(p["thickness_mm"])
    require_positive(inner,outer,thickness)
    m=Mesh()
    annulus_z(m,inner,outer,-thickness/2,thickness/2,64 if detailed else 12)
    return m


def pipe_spool(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    outer=float(p["outside_diameter_mm"])/2
    inner=float(p["inside_diameter_mm"])/2
    length=float(p["length_mm"])
    require_positive(outer,inner,length)
    if not math.isclose(2*(outer-inner),float(p["wall_thickness_mm"])*2,
                        abs_tol=0.001):
        raise ValueError("Pipe bore differs from wall thickness")
    m=Mesh()
    annulus_x(m,inner,outer,-length/2,length/2,72 if detailed else 12)
    return m


def ring_gasket(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    inner=float(p["inner_diameter_mm"])/2
    outer=float(p["outer_diameter_mm"])/2
    thickness=float(p["thickness_mm"])
    require_positive(inner,outer,thickness)
    m=Mesh()
    annulus_x(m,inner,outer,-thickness/2,thickness/2,72 if detailed else 12)
    return m


def flange_plate(mesh: Mesh, inner: float, outer: float,
                 pcd: float, hole_r: float, hole_count: int,
                 thickness: float, detailed: bool) -> None:
    """A real through-drilled annular flange: no filled discs under bolt holes.

    Partition the bolt-circle annulus into radial sectors. Each sector is a
    quadrilateral patch with one central polygonal hole. Joining its perimeter
    to the circle leaves an open bore that can be inspected from either side.
    """
    step=TAU/hole_count
    pad=max(1.0,hole_r*0.24)
    r0=pcd-hole_r-pad
    r1=pcd+hole_r+pad
    if not (inner+1.0 < r0 and r1 < outer-1.0 and
            2*math.sin(step/2)*pcd > 2*hole_r + 1.0):
        raise ValueError("Bolt holes collide with bore, rim or adjacent holes")
    patch=12 if detailed else 3
    samples=hole_count*patch*4
    annulus_x(mesh,inner,r0,0,thickness,samples)
    annulus_x(mesh,r1,outer,0,thickness,samples)
    for j in range(hole_count):
        phi=j*step
        lo,hi=phi-step/2,phi+step/2
        outline=[]
        for k in range(patch):
            t=k/patch
            outline.append((r0+(r1-r0)*t,lo))
        for k in range(patch):
            t=k/patch
            outline.append((r1,lo+(hi-lo)*t))
        for k in range(patch):
            t=k/patch
            outline.append((r1-(r1-r0)*t,hi))
        for k in range(patch):
            t=k/patch
            outline.append((r0,hi-(hi-lo)*t))
        n=len(outline)
        cy=pcd*math.cos(phi)
        cz=pcd*math.sin(phi)
        for k in range(n):
            nxt=(k+1)%n
            r_a,a=outline[k];r_b,b=outline[nxt]
            outer_a=(r_a*math.cos(a),r_a*math.sin(a))
            outer_b=(r_b*math.cos(b),r_b*math.sin(b))
            ha=phi-3*PI/4+TAU*k/n
            hb=phi-3*PI/4+TAU*nxt/n
            hole_a=(cy+hole_r*math.cos(ha),cz+hole_r*math.sin(ha))
            hole_b=(cy+hole_r*math.cos(hb),cz+hole_r*math.sin(hb))
            oa,ob=(thickness,*outer_a),(thickness,*outer_b)
            ia,ib=(thickness,*hole_a),(thickness,*hole_b)
            mesh.quad(oa,ob,ib,ia)
            oa0,ob0=(0,*outer_a),(0,*outer_b)
            ia0,ib0=(0,*hole_a),(0,*hole_b)
            mesh.quad(ia0,ib0,ob0,oa0)
            mesh.quad(ib0,ia0,ia,ib)  # cylindrical drilled hole wall


def weld_neck_flange(record: dict, detailed: bool) -> Mesh:
    p=record["parameters"]
    od=float(p["outside_diameter_mm"])
    thickness=float(p["flange_thickness_mm"])
    bore=float(p["bore_diameter_mm"])
    bolt_circle=float(p["bolt_circle_diameter_mm"])
    bolt_hole=float(p["bolt_hole_diameter_mm"])
    bolt_count=int(p["bolt_hole_count"])
    hub_len=float(p["hub_length_mm"])
    neck_od=float(p["neck_outside_diameter_mm"])
    require_positive(od,thickness,bore,bolt_circle,bolt_hole,hub_len,neck_od)
    if bolt_count<3 or not bore<neck_od<od:
        raise ValueError("Invalid weld-neck flange dimensions")
    mesh=Mesh()
    flange_plate(mesh,bore/2,od/2,bolt_circle/2,bolt_hole/2,
                 bolt_count,thickness,detailed)
    segments=72 if detailed else 12
    # Hollow tapered hub runs axially from the flange to the pipe weld end.
    hub_base=min(od/2*0.75,max(neck_od/2*1.25,bore/2+2))
    for j in range(segments):
        a,b=TAU*j/segments,TAU*(j+1)/segments
        x0=thickness
        x1=thickness+hub_len
        base_a,base_b=point_x(hub_base,a,x0),point_x(hub_base,b,x0)
        end_a,end_b=point_x(neck_od/2,a,x1),point_x(neck_od/2,b,x1)
        mesh.quad(base_a,base_b,end_b,end_a)
        in_a,in_b=point_x(bore/2,a,x0),point_x(bore/2,b,x0)
        end_in_a,end_in_b=point_x(bore/2,a,x1),point_x(bore/2,b,x1)
        mesh.quad(in_b,in_a,end_in_a,end_in_b)
        mesh.quad(end_a,end_b,end_in_b,end_in_a)
    return mesh


GENERATORS = {
    "hex_bolt": bolt,
    "hex_nut": hex_nut,
    "flat_washer": flat_washer,
    "pipe_spool": pipe_spool,
    "weld_neck_flange": weld_neck_flange,
    "ring_gasket": ring_gasket,
}


def build_mesh(record: dict, lod: str) -> Mesh:
    if lod not in {"inspection", "psp"}:
        raise ValueError("LOD must be inspection or psp")
    generator=record["generator"]
    if generator["implementation_status"] != "available":
        raise ValueError("Generator is not available")
    fn=GENERATORS.get(generator["name"])
    if fn is None:
        raise ValueError(f"Generator not found: {generator['name']}")
    mesh=fn(record,lod=="inspection")
    if not mesh.triangles:
        raise ValueError("Empty output mesh")
    for tri in mesh.triangles:
        for vertex in tri:
            if not all(math.isfinite(x) for x in vertex):
                raise ValueError("Nonfinite mesh vertex")
    return mesh
