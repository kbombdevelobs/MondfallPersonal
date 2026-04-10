"""
Blender Python script: Generate moonbase exterior model (.glb)
Run: blender --background --python scripts/generate_moonbase_exterior.py

Produces: resources/models/structures/moonbase_exterior.glb

Geodesic dome with metallic panels, 3 airlock corridors,
antenna array, landing pad markings, and solar panel arrays.
All geometry uses vertex colors (no texture files needed).
"""

import bpy
import bmesh
import math
import os
import sys

# ---------------------------------------------------------------------------
# Configuration — matches game constants from config.h
# ---------------------------------------------------------------------------
EXTERIOR_RADIUS = 4.5       # MOONBASE_EXTERIOR_RADIUS
GEODESIC_SEGS = 12          # MOONBASE_GEODESIC_SEGS
DOME_RINGS = 5
DOME_HEIGHT = EXTERIOR_RADIUS * 0.85
DOOR_COUNT = 3
CYLINDER_RADIUS = EXTERIOR_RADIUS * 0.65
CYLINDER_ABOVE = 1.0
CYLINDER_BELOW = 8.0

# Vertex color palette (linear sRGB 0-1)
def c(r, g, b):
    """Convert 0-255 to 0.0-1.0 for vertex colors."""
    return (r / 255.0, g / 255.0, b / 255.0, 1.0)

COL_HULL       = c(60, 60, 65)
COL_PANEL_SEAM = c(90, 90, 95)
COL_AIRLOCK    = c(120, 80, 40)
COL_WARNING    = c(180, 30, 20)
COL_STEEL      = c(75, 78, 82)
COL_STEEL_LT   = c(110, 112, 115)
COL_CONCRETE   = c(145, 140, 135)
COL_CONCRETE_DK = c(100, 95, 90)
COL_ANTENNA    = c(120, 118, 112)
COL_HAZARD_Y   = c(220, 200, 40)
COL_HAZARD_BK  = c(25, 25, 25)
COL_DOOR_DARK  = c(12, 12, 16)
COL_LIGHT_GREEN = c(30, 200, 80)
COL_SOLAR_BLUE = c(40, 50, 90)
COL_SOLAR_FRAME = c(80, 80, 85)
COL_PAD_MARK   = c(200, 200, 50)

OUTPUT_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                           "resources", "models", "structures", "moonbase_exterior.glb")

# ---------------------------------------------------------------------------
# Utility functions
# ---------------------------------------------------------------------------

def clear_scene():
    """Remove all objects from the scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for col in bpy.data.collections:
        bpy.data.collections.remove(col)

def new_mesh_obj(name):
    """Create an empty mesh object and add it to the scene."""
    mesh = bpy.data.meshes.new(name + "_mesh")
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return obj, mesh

def set_vertex_colors(bm, color):
    """Set all face vertex colors in a bmesh to a single color."""
    cl = bm.loops.layers.color.active
    if cl is None:
        cl = bm.loops.layers.color.new("Col")
    for face in bm.faces:
        for loop in face.loops:
            loop[cl] = color

def assign_face_colors(bm, face_indices, color):
    """Set vertex colors for specific faces."""
    cl = bm.loops.layers.color.active
    if cl is None:
        cl = bm.loops.layers.color.new("Col")
    for fi in face_indices:
        if fi < len(bm.faces):
            for loop in bm.faces[fi].loops:
                loop[cl] = color

def create_colored_cube(name, size, location, color, rotation=(0, 0, 0)):
    """Create a cube with uniform vertex color."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    # Scale
    for v in bm.verts:
        v.co.x *= size[0]
        v.co.y *= size[1]
        v.co.z *= size[2]
    # Color
    cl = bm.loops.layers.color.new("Col")
    for face in bm.faces:
        for loop in face.loops:
            loop[cl] = color
    mesh = bpy.data.meshes.new(name + "_mesh")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.location = location
    obj.rotation_euler = rotation
    bpy.context.collection.objects.link(obj)
    return obj

def create_cylinder(name, radius, depth, location, color, segments=16, rotation=(0, 0, 0)):
    """Create a cylinder with vertex colors."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False,
                          segments=segments, radius1=radius, radius2=radius,
                          depth=depth)
    cl = bm.loops.layers.color.new("Col")
    for face in bm.faces:
        for loop in face.loops:
            loop[cl] = color
    mesh = bpy.data.meshes.new(name + "_mesh")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    obj.location = location
    obj.rotation_euler = rotation
    bpy.context.collection.objects.link(obj)
    return obj

# ---------------------------------------------------------------------------
# Geometry builders
# ---------------------------------------------------------------------------

def build_underground_cylinder():
    """Cylindrical body that extends underground beneath the dome."""
    total = CYLINDER_ABOVE + CYLINDER_BELOW
    cy = CYLINDER_ABOVE - total * 0.5
    return create_cylinder("cylinder_body", CYLINDER_RADIUS, total,
                           (0, cy, 0), COL_CONCRETE_DK, segments=20)

def build_cylinder_cap():
    """Steel cap on top of the cylinder."""
    cap_y = CYLINDER_ABOVE + 0.1
    return create_colored_cube("cylinder_cap",
                               (CYLINDER_RADIUS * 2.1, 0.2, CYLINDER_RADIUS * 2.1),
                               (0, cap_y, 0), COL_STEEL)

def build_cylinder_rim():
    """Steel rim ring at the top of the cylinder."""
    parts = []
    cap_y = CYLINDER_ABOVE
    for i in range(20):
        a = i / 20.0 * 2.0 * math.pi
        px = math.cos(a) * (CYLINDER_RADIUS + 0.1)
        pz = math.sin(a) * (CYLINDER_RADIUS + 0.1)
        rot_y = -a + math.pi / 2
        p = create_colored_cube(f"rim_{i}",
                                (CYLINDER_RADIUS * 0.45, 0.3, 0.2),
                                (px, cap_y, pz), COL_STEEL,
                                rotation=(0, rot_y, 0))
        parts.append(p)
    return parts

def build_geodesic_dome():
    """Geodesic dome panels sitting on top of the cylinder."""
    parts = []
    base_y = CYLINDER_ABOVE + 0.2
    r = EXTERIOR_RADIUS
    h = DOME_HEIGHT

    # Door angles (3 doors at 120 degree intervals)
    door_angles = [i * 2.0 * math.pi / DOOR_COUNT for i in range(DOOR_COUNT)]

    for ring in range(DOME_RINGS):
        t0 = ring / DOME_RINGS
        t1 = (ring + 1) / DOME_RINGS
        a0 = t0 * math.pi * 0.5
        a1 = t1 * math.pi * 0.5
        r0 = r * math.cos(a0)
        r1 = r * math.cos(a1)
        y0 = base_y + h * math.sin(a0)
        y1 = base_y + h * math.sin(a1)

        shade = 135 + (ring % 2) * 15
        ring_col = c(shade, shade - 3, shade - 6)

        for seg in range(GEODESIC_SEGS):
            ang0 = seg / GEODESIC_SEGS * 2.0 * math.pi
            ang1 = (seg + 1) / GEODESIC_SEGS * 2.0 * math.pi
            mid_ang = (ang0 + ang1) * 0.5
            mid_r = (r0 + r1) * 0.5
            mid_y = (y0 + y1) * 0.5

            # Skip panels overlapping door openings (bottom ring only)
            if ring == 0:
                skip = False
                for da in door_angles:
                    diff = mid_ang - da
                    while diff > math.pi:
                        diff -= 2.0 * math.pi
                    while diff < -math.pi:
                        diff += 2.0 * math.pi
                    if abs(diff) < (2.0 * math.pi / GEODESIC_SEGS) * 0.9:
                        skip = True
                        break
                if skip:
                    continue

            px = math.cos(mid_ang) * mid_r
            pz = math.sin(mid_ang) * mid_r

            panel_w = 2.0 * mid_r * math.sin(math.pi / GEODESIC_SEGS) * 1.08
            panel_h = (y1 - y0) * 1.15
            if panel_w < 0.1:
                panel_w = 0.1

            rot_y = -mid_ang + math.pi / 2
            tilt = (a0 + a1) * 0.5

            panel = create_colored_cube(
                f"panel_{ring}_{seg}",
                (panel_w, panel_h, 0.18),
                (px, mid_y, pz),
                ring_col,
                rotation=(tilt, rot_y, 0)
            )
            parts.append(panel)

            # Panel seam wireframe effect — thinner cubes on edges
            seam = create_colored_cube(
                f"seam_{ring}_{seg}",
                (panel_w * 1.01, panel_h * 1.01, 0.02),
                (px, mid_y, pz),
                COL_PANEL_SEAM,
                rotation=(tilt, rot_y, 0)
            )
            parts.append(seam)

    # Dome cap
    cap = create_colored_cube("dome_cap",
                              (r * 0.35, 0.25, r * 0.35),
                              (0, base_y + h, 0), COL_STEEL)
    parts.append(cap)

    # Inner fill — prevents seeing void
    body_h = h * 0.55
    body_r = r * 0.7
    fill = create_cylinder("dome_fill", body_r, body_h,
                           (0, base_y + body_h * 0.5, 0), COL_CONCRETE_DK, segments=12)
    parts.append(fill)

    # Upper fill
    upper_fill = create_colored_cube("dome_upper_fill",
                                     (r * 0.75, (h - body_h) * 0.8, r * 0.75),
                                     (0, base_y + body_h + (h - body_h) * 0.4, 0),
                                     COL_CONCRETE_DK)
    parts.append(upper_fill)

    return parts

def build_red_band():
    """Red accent band around dome base."""
    parts = []
    base_y = CYLINDER_ABOVE + 0.2
    r = EXTERIOR_RADIUS
    for i in range(20):
        a = i / 20.0 * 2.0 * math.pi
        px = math.cos(a) * (r + 0.03)
        pz = math.sin(a) * (r + 0.03)
        rot_y = -a + math.pi / 2
        band = create_colored_cube(f"band_{i}",
                                   (r * 0.35, 0.15, 0.06),
                                   (px, base_y + 0.25, pz), COL_WARNING,
                                   rotation=(0, rot_y, 0))
        parts.append(band)
    return parts

def build_airlock_corridor(door_idx, angle):
    """Build one airlock corridor extending from the dome."""
    parts = []
    base_y = CYLINDER_ABOVE + 0.2
    door_dist = EXTERIOR_RADIUS * 1.1
    dx = math.cos(angle) * door_dist
    dz = math.sin(angle) * door_dist
    rot_y = -angle + math.pi / 2

    corr_len = 2.8
    corr_w = 2.0
    corr_h = 2.2

    prefix = f"airlock_{door_idx}"

    # Left wall
    parts.append(create_colored_cube(
        f"{prefix}_wall_l", (0.25, corr_h, corr_len),
        (dx + math.cos(angle + math.pi/2) * corr_w * 0.5,
         base_y + corr_h * 0.5,
         dz + math.sin(angle + math.pi/2) * corr_w * 0.5),
        COL_CONCRETE, rotation=(0, rot_y, 0)))

    # Right wall
    parts.append(create_colored_cube(
        f"{prefix}_wall_r", (0.25, corr_h, corr_len),
        (dx - math.cos(angle + math.pi/2) * corr_w * 0.5,
         base_y + corr_h * 0.5,
         dz - math.sin(angle + math.pi/2) * corr_w * 0.5),
        COL_CONCRETE, rotation=(0, rot_y, 0)))

    # Roof
    parts.append(create_colored_cube(
        f"{prefix}_roof", (corr_w + 0.3, 0.25, corr_len),
        (dx, base_y + corr_h + 0.12, dz),
        COL_CONCRETE, rotation=(0, rot_y, 0)))

    # Floor
    parts.append(create_colored_cube(
        f"{prefix}_floor", (corr_w + 0.3, 0.12, corr_len),
        (dx, base_y - 0.06, dz),
        COL_STEEL, rotation=(0, rot_y, 0)))

    # Interior dark fill
    parts.append(create_colored_cube(
        f"{prefix}_interior", (corr_w - 0.3, corr_h - 0.2, corr_len - 0.2),
        (dx, base_y + corr_h * 0.5, dz),
        COL_DOOR_DARK, rotation=(0, rot_y, 0)))

    # Hazard chevrons on roof
    for ci in range(5):
        rz_off = -0.1 - ci * 0.6
        chevron_col = COL_HAZARD_Y if ci % 2 == 0 else COL_HAZARD_BK
        # Position along corridor direction
        chev_x = dx + math.cos(angle) * rz_off
        chev_z = dz + math.sin(angle) * rz_off
        parts.append(create_colored_cube(
            f"{prefix}_chevron_{ci}", (corr_w + 0.1, 0.08, 0.25),
            (chev_x, base_y + corr_h + 0.2, chev_z),
            chevron_col, rotation=(0, rot_y, 0)))

    # Door frame at the end
    end_x = dx + math.cos(angle) * (-corr_len)
    end_z = dz + math.sin(angle) * (-corr_len)

    # Frame posts
    for side in [-1, 1]:
        post_x = end_x + math.cos(angle + math.pi/2) * (corr_w * 0.5 + 0.25) * side
        post_z = end_z + math.sin(angle + math.pi/2) * (corr_w * 0.5 + 0.25) * side
        parts.append(create_colored_cube(
            f"{prefix}_frame_{side}", (0.5, corr_h + 0.3, 0.35),
            (post_x, base_y + corr_h * 0.5, post_z),
            COL_STEEL, rotation=(0, rot_y, 0)))

        # Warning stripes on frame posts
        for si in range(4):
            stripe_col = COL_WARNING if si % 2 == 0 else COL_HAZARD_Y
            parts.append(create_colored_cube(
                f"{prefix}_stripe_{side}_{si}", (0.1, 0.2, 0.1),
                (post_x, base_y + 0.3 + si * 0.5, post_z),
                stripe_col, rotation=(0, rot_y, 0)))

    # Top frame
    parts.append(create_colored_cube(
        f"{prefix}_frame_top", (corr_w + 1.2, 0.35, 0.35),
        (end_x, base_y + corr_h + 0.2, end_z),
        COL_STEEL, rotation=(0, rot_y, 0)))

    # Door opening (dark rectangle)
    parts.append(create_colored_cube(
        f"{prefix}_door", (corr_w - 0.4, 2.0, 0.1),
        (end_x, base_y + 1.0, end_z),
        COL_DOOR_DARK, rotation=(0, rot_y, 0)))

    # Threshold step
    parts.append(create_colored_cube(
        f"{prefix}_threshold", (corr_w + 0.2, 0.12, 0.5),
        (end_x - math.cos(angle) * 0.25,
         base_y + 0.06,
         end_z - math.sin(angle) * 0.25),
        COL_STEEL, rotation=(0, rot_y, 0)))

    # Status light
    parts.append(create_colored_cube(
        f"{prefix}_light", (0.5, 0.15, 0.1),
        (end_x, base_y + corr_h + 0.45, end_z),
        COL_LIGHT_GREEN, rotation=(0, rot_y, 0)))

    # Airlock brass door accent
    parts.append(create_colored_cube(
        f"{prefix}_brass", (corr_w - 0.6, 1.8, 0.06),
        (end_x, base_y + 0.95, end_z),
        COL_AIRLOCK, rotation=(0, rot_y, 0)))

    return parts

def build_antenna():
    """Antenna array on top of the dome."""
    parts = []
    base_y = CYLINDER_ABOVE + 0.2
    dome_top = base_y + DOME_HEIGHT

    # Main mast
    parts.append(create_colored_cube("antenna_mast",
                                     (0.06, 3.0, 0.06),
                                     (0, dome_top + 1.5, 0), COL_ANTENNA))
    # Cross bar
    parts.append(create_colored_cube("antenna_cross",
                                     (0.5, 0.03, 0.5),
                                     (0, dome_top + 3.2, 0), COL_STEEL))
    # Beacon light
    parts.append(create_colored_cube("antenna_beacon",
                                     (0.1, 0.1, 0.1),
                                     (0, dome_top + 3.4, 0), COL_WARNING))

    # Additional antenna elements
    parts.append(create_colored_cube("antenna_dish_arm",
                                     (0.04, 0.04, 0.6),
                                     (0.15, dome_top + 2.8, 0), COL_ANTENNA))
    parts.append(create_colored_cube("antenna_dish",
                                     (0.3, 0.3, 0.04),
                                     (0.15, dome_top + 2.8, 0.32), COL_STEEL_LT))

    return parts

def build_solar_panels():
    """Solar panel arrays around the base."""
    parts = []
    base_y = CYLINDER_ABOVE + 0.2

    for i in range(4):
        angle = i * math.pi / 2.0 + math.pi / 4.0
        dist = EXTERIOR_RADIUS + 2.5
        px = math.cos(angle) * dist
        pz = math.sin(angle) * dist
        rot_y = -angle

        # Support pole
        parts.append(create_colored_cube(
            f"solar_pole_{i}", (0.08, 1.8, 0.08),
            (px, base_y + 0.9, pz), COL_STEEL))

        # Panel
        parts.append(create_colored_cube(
            f"solar_panel_{i}", (2.0, 0.04, 1.2),
            (px, base_y + 1.9, pz), COL_SOLAR_BLUE,
            rotation=(0.3, rot_y, 0)))

        # Frame
        parts.append(create_colored_cube(
            f"solar_frame_{i}", (2.1, 0.06, 0.06),
            (px, base_y + 1.9, pz), COL_SOLAR_FRAME,
            rotation=(0.3, rot_y, 0)))

    return parts

def build_landing_pad_markings():
    """Landing pad markings around the base perimeter."""
    parts = []

    # Cross markings at cardinal points
    for i in range(4):
        angle = i * math.pi / 2.0
        dist = EXTERIOR_RADIUS + 5.0
        px = math.cos(angle) * dist
        pz = math.sin(angle) * dist

        # H-pattern landing mark
        parts.append(create_colored_cube(
            f"pad_h_{i}", (1.5, 0.02, 0.12),
            (px, 0.01, pz), COL_PAD_MARK))
        parts.append(create_colored_cube(
            f"pad_v1_{i}", (0.12, 0.02, 1.0),
            (px - 0.65, 0.01, pz), COL_PAD_MARK))
        parts.append(create_colored_cube(
            f"pad_v2_{i}", (0.12, 0.02, 1.0),
            (px + 0.65, 0.01, pz), COL_PAD_MARK))

    # Circle ring markings
    ring_segs = 24
    ring_r = EXTERIOR_RADIUS + 3.0
    for i in range(ring_segs):
        a0 = i / ring_segs * 2.0 * math.pi
        if i % 3 == 0:
            continue  # gaps in the ring
        px = math.cos(a0) * ring_r
        pz = math.sin(a0) * ring_r
        parts.append(create_colored_cube(
            f"ring_{i}", (0.4, 0.02, 0.08),
            (px, 0.01, pz), COL_PAD_MARK,
            rotation=(0, a0, 0)))

    return parts

# ---------------------------------------------------------------------------
# Material setup for vertex color export
# ---------------------------------------------------------------------------

def setup_vertex_color_material():
    """Create a material that uses vertex colors for glTF export."""
    mat = bpy.data.materials.new(name="MoonbaseExteriorMat")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    # Clear defaults
    for node in nodes:
        nodes.remove(node)

    # Create nodes
    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (100, 0)
    bsdf.inputs['Metallic'].default_value = 0.3
    bsdf.inputs['Roughness'].default_value = 0.7

    vcol = nodes.new('ShaderNodeVertexColor')
    vcol.location = (-200, 0)
    vcol.layer_name = "Col"

    # Link vertex color -> Base Color
    links.new(vcol.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    clear_scene()

    all_parts = []

    # Build all geometry
    cyl = build_underground_cylinder()
    all_parts.append(cyl)

    cap = build_cylinder_cap()
    all_parts.append(cap)

    all_parts.extend(build_cylinder_rim())
    all_parts.extend(build_geodesic_dome())
    all_parts.extend(build_red_band())

    # 3 airlock corridors at 120-degree intervals
    for i in range(DOOR_COUNT):
        angle = i * 2.0 * math.pi / DOOR_COUNT
        all_parts.extend(build_airlock_corridor(i, angle))

    all_parts.extend(build_antenna())
    all_parts.extend(build_solar_panels())
    all_parts.extend(build_landing_pad_markings())

    # Apply vertex color material to all objects
    mat = setup_vertex_color_material()
    for obj in all_parts:
        if obj.type == 'MESH':
            obj.data.materials.append(mat)

    # Select all mesh objects and join into one
    bpy.ops.object.select_all(action='DESELECT')
    for obj in all_parts:
        if obj.type == 'MESH':
            obj.select_set(True)
    if all_parts:
        bpy.context.view_layer.objects.active = all_parts[0]
        bpy.ops.object.join()

    # Rename final object
    final = bpy.context.active_object
    final.name = "moonbase_exterior"

    # Ensure output directory exists
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)

    # Export as glb
    bpy.ops.export_scene.gltf(
        filepath=OUTPUT_PATH,
        export_format='GLB',
        export_colors=True,
        export_normals=True,
        export_apply=True,
        use_selection=True,
    )

    print(f"Exported moonbase exterior to: {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
