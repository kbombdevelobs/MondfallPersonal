"""
Blender Python script: Generate moonbase interior model (.glb)
Run: blender --background --python scripts/generate_moonbase_interior.py

Produces: resources/models/structures/moonbase_interior.glb

Rectangular room (~24x20m) with 3 doorways, industrial floor grating,
wall panels with riveted seams, ceiling with pipe runs and lighting,
resupply closet, He-3 tank area, bar/lounge area, and propaganda decor.
All geometry uses vertex colors (no texture files needed).
"""

import bpy
import bmesh
import math
import os

# ---------------------------------------------------------------------------
# Configuration — matches game constants from config.h
# ---------------------------------------------------------------------------
INTERIOR_W = 24.0    # MOONBASE_INTERIOR_W
INTERIOR_D = 20.0    # MOONBASE_INTERIOR_D
INTERIOR_H = 5.0     # MOONBASE_INTERIOR_H
HALF_W = INTERIOR_W * 0.5
HALF_D = INTERIOR_D * 0.5

# Vertex color palette (linear sRGB 0-1)
def c(r, g, b):
    return (r / 255.0, g / 255.0, b / 255.0, 1.0)

COL_FLOOR       = c(50, 50, 55)
COL_FLOOR_GRATE = c(40, 40, 45)
COL_FLOOR_LINE  = c(35, 35, 38)
COL_WALLS       = c(70, 70, 75)
COL_WALL_LOWER  = c(55, 55, 60)
COL_WALL_RIVET  = c(80, 80, 85)
COL_CEILING     = c(40, 40, 45)
COL_CEILING_PIPE = c(50, 52, 56)
COL_BRASS       = c(160, 130, 60)
COL_RED         = c(180, 30, 20)
COL_DOOR_DARK   = c(12, 12, 16)
COL_DOOR_FRAME  = c(90, 80, 60)
COL_STEEL       = c(75, 78, 82)
COL_LIGHT_WARM  = c(255, 240, 190)
COL_CLOSET_BODY = c(60, 70, 60)
COL_CLOSET_DOOR = c(70, 85, 70)
COL_CLOSET_GLOW = c(50, 240, 110)
COL_TANK_BLUE   = c(50, 200, 235)
COL_TANK_BODY   = c(55, 65, 75)
COL_BAR_TOP     = c(80, 50, 30)
COL_BAR_BODY    = c(50, 35, 22)
COL_STOOL_SEAT  = c(60, 60, 65)
COL_TABLE_TOP   = c(70, 45, 28)
COL_CHAIR       = c(55, 55, 60)
COL_PROPAGANDA_BG = c(200, 190, 170)
COL_EAGLE_DARK  = c(30, 30, 30)
COL_SWASTIKA    = c(20, 20, 20)
COL_BANNER_RED  = c(180, 30, 20)
COL_BANNER_WHITE = c(220, 215, 200)
COL_IRON_CROSS  = c(60, 60, 60)

OUTPUT_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                           "resources", "models", "structures", "moonbase_interior.glb")

# ---------------------------------------------------------------------------
# Utility
# ---------------------------------------------------------------------------

def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for col in bpy.data.collections:
        bpy.data.collections.remove(col)

def create_colored_cube(name, size, location, color, rotation=(0, 0, 0)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co.x *= size[0]
        v.co.y *= size[1]
        v.co.z *= size[2]
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
# Floor
# ---------------------------------------------------------------------------

def build_floor(y):
    parts = []

    # Main floor slab
    parts.append(create_colored_cube("floor_main",
        (INTERIOR_W, 0.2, INTERIOR_D), (0, y - 0.1, 0), COL_FLOOR))

    # Grating pattern — longitudinal lines
    for fx in range(-int(HALF_W) + 1, int(HALF_W), 1):
        parts.append(create_colored_cube(f"grate_x_{fx}",
            (0.03, 0.01, INTERIOR_D), (fx + 0.5, y + 0.005, 0), COL_FLOOR_LINE))

    # Cross grating lines
    for fz in range(-int(HALF_D) + 1, int(HALF_D), 2):
        parts.append(create_colored_cube(f"grate_z_{fz}",
            (INTERIOR_W, 0.01, 0.03), (0, y + 0.005, fz + 0.5), COL_FLOOR_LINE))

    # Darker grating panels (alternating)
    for gx in range(-int(HALF_W) + 1, int(HALF_W), 3):
        for gz in range(-int(HALF_D) + 1, int(HALF_D), 3):
            parts.append(create_colored_cube(f"grate_panel_{gx}_{gz}",
                (1.4, 0.015, 1.4), (gx, y + 0.008, gz), COL_FLOOR_GRATE))

    return parts

# ---------------------------------------------------------------------------
# Walls
# ---------------------------------------------------------------------------

def build_walls(y):
    parts = []
    h = INTERIOR_H
    thick = 0.3
    wain_h = 1.5   # lower industrial panel height
    upper_h = h - wain_h

    # --- NORTH WALL (resupply closet area) ---
    parts.append(create_colored_cube("wall_n_lower",
        (INTERIOR_W, wain_h, thick), (0, y + wain_h * 0.5, HALF_D), COL_WALL_LOWER))
    parts.append(create_colored_cube("wall_n_upper",
        (INTERIOR_W, upper_h, thick), (0, y + wain_h + upper_h * 0.5, HALF_D), COL_WALLS))

    # Riveted seam between lower and upper
    parts.append(create_colored_cube("wall_n_seam",
        (INTERIOR_W, 0.1, 0.06), (0, y + wain_h, HALF_D - 0.01), COL_WALL_RIVET))
    # Rivet dots
    for rx in range(-int(HALF_W) + 2, int(HALF_W), 2):
        parts.append(create_colored_cube(f"rivet_n_{rx}",
            (0.06, 0.06, 0.04), (rx, y + wain_h, HALF_D - 0.02), COL_BRASS))

    # --- SOUTH WALL (He-3 tank area, door 0 center) ---
    door_w = 2.6
    side_w = (INTERIOR_W - door_w) * 0.5
    # Left section
    parts.append(create_colored_cube("wall_s_left_lower",
        (side_w, wain_h, thick), (-HALF_W + side_w * 0.5, y + wain_h * 0.5, -HALF_D), COL_WALL_LOWER))
    parts.append(create_colored_cube("wall_s_left_upper",
        (side_w, upper_h, thick), (-HALF_W + side_w * 0.5, y + wain_h + upper_h * 0.5, -HALF_D), COL_WALLS))
    # Right section
    parts.append(create_colored_cube("wall_s_right_lower",
        (side_w, wain_h, thick), (HALF_W - side_w * 0.5, y + wain_h * 0.5, -HALF_D), COL_WALL_LOWER))
    parts.append(create_colored_cube("wall_s_right_upper",
        (side_w, upper_h, thick), (HALF_W - side_w * 0.5, y + wain_h + upper_h * 0.5, -HALF_D), COL_WALLS))
    # Above door
    parts.append(create_colored_cube("wall_s_above_door",
        (door_w + 0.4, 1.0, thick), (0, y + h - 0.5, -HALF_D), COL_WALLS))
    # Seams
    parts.append(create_colored_cube("wall_s_seam_l",
        (side_w, 0.1, 0.06), (-HALF_W + side_w * 0.5, y + wain_h, -HALF_D + 0.01), COL_WALL_RIVET))
    parts.append(create_colored_cube("wall_s_seam_r",
        (side_w, 0.1, 0.06), (HALF_W - side_w * 0.5, y + wain_h, -HALF_D + 0.01), COL_WALL_RIVET))
    for rx in range(-int(HALF_W) + 2, int(HALF_W), 2):
        parts.append(create_colored_cube(f"rivet_s_{rx}",
            (0.06, 0.06, 0.04), (rx, y + wain_h, -HALF_D + 0.02), COL_BRASS))

    # --- EAST WALL (door 1 at Z=2) ---
    edoor_z = 2.0
    e_below = edoor_z + HALF_D
    e_above = HALF_D - edoor_z - door_w * 0.5

    if e_below - door_w * 0.5 > 0.5:
        sec_len = e_below - door_w * 0.5
        sec_z = -HALF_D + sec_len * 0.5
        parts.append(create_colored_cube("wall_e_south_lower",
            (thick, wain_h, sec_len), (HALF_W, y + wain_h * 0.5, sec_z), COL_WALL_LOWER))
        parts.append(create_colored_cube("wall_e_south_upper",
            (thick, upper_h, sec_len), (HALF_W, y + wain_h + upper_h * 0.5, sec_z), COL_WALLS))
        parts.append(create_colored_cube("wall_e_south_seam",
            (0.06, 0.1, sec_len), (HALF_W - 0.01, y + wain_h, sec_z), COL_WALL_RIVET))

    if e_above > 0.5:
        sec_z = edoor_z + door_w * 0.5 + e_above * 0.5
        parts.append(create_colored_cube("wall_e_north_lower",
            (thick, wain_h, e_above), (HALF_W, y + wain_h * 0.5, sec_z), COL_WALL_LOWER))
        parts.append(create_colored_cube("wall_e_north_upper",
            (thick, upper_h, e_above), (HALF_W, y + wain_h + upper_h * 0.5, sec_z), COL_WALLS))
        parts.append(create_colored_cube("wall_e_north_seam",
            (0.06, 0.1, e_above), (HALF_W - 0.01, y + wain_h, sec_z), COL_WALL_RIVET))

    # Above east door
    parts.append(create_colored_cube("wall_e_above_door",
        (thick, 1.0, door_w + 0.4), (HALF_W, y + h - 0.5, edoor_z), COL_WALLS))

    # --- WEST WALL (door 2 at Z=2) ---
    if e_below - door_w * 0.5 > 0.5:
        sec_len = e_below - door_w * 0.5
        sec_z = -HALF_D + sec_len * 0.5
        parts.append(create_colored_cube("wall_w_south_lower",
            (thick, wain_h, sec_len), (-HALF_W, y + wain_h * 0.5, sec_z), COL_WALL_LOWER))
        parts.append(create_colored_cube("wall_w_south_upper",
            (thick, upper_h, sec_len), (-HALF_W, y + wain_h + upper_h * 0.5, sec_z), COL_WALLS))
        parts.append(create_colored_cube("wall_w_south_seam",
            (0.06, 0.1, sec_len), (-HALF_W + 0.01, y + wain_h, sec_z), COL_WALL_RIVET))

    if e_above > 0.5:
        sec_z = edoor_z + door_w * 0.5 + e_above * 0.5
        parts.append(create_colored_cube("wall_w_north_lower",
            (thick, wain_h, e_above), (-HALF_W, y + wain_h * 0.5, sec_z), COL_WALL_LOWER))
        parts.append(create_colored_cube("wall_w_north_upper",
            (thick, upper_h, e_above), (-HALF_W, y + wain_h + upper_h * 0.5, sec_z), COL_WALLS))
        parts.append(create_colored_cube("wall_w_north_seam",
            (0.06, 0.1, e_above), (-HALF_W + 0.01, y + wain_h, sec_z), COL_WALL_RIVET))

    # Above west door
    parts.append(create_colored_cube("wall_w_above_door",
        (thick, 1.0, door_w + 0.4), (-HALF_W, y + h - 0.5, edoor_z), COL_WALLS))

    # Brass trim along all walls at seam height
    parts.append(create_colored_cube("trim_n", (INTERIOR_W, 0.04, 0.04), (0, y + wain_h + 0.03, HALF_D - 0.02), COL_BRASS))
    parts.append(create_colored_cube("trim_s", (INTERIOR_W, 0.04, 0.04), (0, y + wain_h + 0.03, -HALF_D + 0.02), COL_BRASS))
    parts.append(create_colored_cube("trim_e", (0.04, 0.04, INTERIOR_D), (HALF_W - 0.02, y + wain_h + 0.03, 0), COL_BRASS))
    parts.append(create_colored_cube("trim_w", (0.04, 0.04, INTERIOR_D), (-HALF_W + 0.02, y + wain_h + 0.03, 0), COL_BRASS))

    return parts

# ---------------------------------------------------------------------------
# Ceiling with pipes and lights
# ---------------------------------------------------------------------------

def build_ceiling(y):
    parts = []
    ceil_y = y + INTERIOR_H

    # Main ceiling slab
    parts.append(create_colored_cube("ceiling_main",
        (INTERIOR_W, 0.1, INTERIOR_D), (0, ceil_y + 0.05, 0), COL_CEILING))

    # Exposed pipe runs (east-west across the ceiling)
    for pz in range(-int(HALF_D) + 3, int(HALF_D), 4):
        parts.append(create_cylinder(f"pipe_main_{pz}",
            0.08, INTERIOR_W - 1.0,
            (0, ceil_y - 0.15, pz), COL_CEILING_PIPE, segments=8,
            rotation=(0, 0, math.pi / 2)))

        # Pipe supports / brackets
        for px in range(-int(HALF_W) + 4, int(HALF_W), 6):
            parts.append(create_colored_cube(f"pipe_bracket_{pz}_{px}",
                (0.06, 0.15, 0.12), (px, ceil_y - 0.07, pz), COL_STEEL))

    # Smaller cross-pipes (north-south)
    for px in range(-int(HALF_W) + 5, int(HALF_W), 8):
        parts.append(create_cylinder(f"pipe_cross_{px}",
            0.05, INTERIOR_D - 2.0,
            (px, ceil_y - 0.25, 0), COL_CEILING_PIPE, segments=8,
            rotation=(math.pi / 2, 0, 0)))

    # Lighting fixtures — rectangular industrial lights
    for lx in range(-int(HALF_W) + 4, int(HALF_W), 6):
        for lz in range(-int(HALF_D) + 3, int(HALF_D), 5):
            # Housing
            parts.append(create_colored_cube(f"light_housing_{lx}_{lz}",
                (1.0, 0.1, 0.4), (lx, ceil_y - 0.35, lz), COL_STEEL))
            # Warm light panel
            parts.append(create_colored_cube(f"light_panel_{lx}_{lz}",
                (0.8, 0.04, 0.3), (lx, ceil_y - 0.42, lz), COL_LIGHT_WARM))

    return parts

# ---------------------------------------------------------------------------
# Doorways
# ---------------------------------------------------------------------------

def build_doors(y):
    parts = []
    door_h = 2.6
    door_w = 2.2

    # Door 0: south wall center
    for side in [-1, 1]:
        parts.append(create_colored_cube(f"door0_frame_{side}",
            (0.2, door_h, 0.25),
            (side * (door_w * 0.5 + 0.15), y + door_h * 0.5, -HALF_D + 0.12), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door0_lintel",
        (door_w + 0.5, 0.2, 0.25), (0, y + door_h + 0.1, -HALF_D + 0.12), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door0_fill",
        (door_w - 0.1, door_h - 0.1, 0.05), (0, y + door_h * 0.5, -HALF_D + 0.05), COL_DOOR_DARK))

    # Door 1: east wall at Z=2
    ez = 2.0
    for side in [-1, 1]:
        parts.append(create_colored_cube(f"door1_frame_{side}",
            (0.25, door_h, 0.2),
            (HALF_W - 0.12, y + door_h * 0.5, ez + side * (door_w * 0.5 + 0.15)), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door1_lintel",
        (0.25, 0.2, door_w + 0.5), (HALF_W - 0.12, y + door_h + 0.1, ez), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door1_fill",
        (0.05, door_h - 0.1, door_w - 0.1), (HALF_W - 0.05, y + door_h * 0.5, ez), COL_DOOR_DARK))

    # Door 2: west wall at Z=2
    for side in [-1, 1]:
        parts.append(create_colored_cube(f"door2_frame_{side}",
            (0.25, door_h, 0.2),
            (-HALF_W + 0.12, y + door_h * 0.5, ez + side * (door_w * 0.5 + 0.15)), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door2_lintel",
        (0.25, 0.2, door_w + 0.5), (-HALF_W + 0.12, y + door_h + 0.1, ez), COL_DOOR_FRAME))
    parts.append(create_colored_cube("door2_fill",
        (0.05, door_h - 0.1, door_w - 0.1), (-HALF_W + 0.05, y + door_h * 0.5, ez), COL_DOOR_DARK))

    # Red warning strips above each door
    parts.append(create_colored_cube("door0_warning",
        (door_w + 0.3, 0.08, 0.06), (0, y + door_h + 0.25, -HALF_D + 0.12), COL_RED))
    parts.append(create_colored_cube("door1_warning",
        (0.06, 0.08, door_w + 0.3), (HALF_W - 0.12, y + door_h + 0.25, ez), COL_RED))
    parts.append(create_colored_cube("door2_warning",
        (0.06, 0.08, door_w + 0.3), (-HALF_W + 0.12, y + door_h + 0.25, ez), COL_RED))

    return parts

# ---------------------------------------------------------------------------
# Resupply closet (north wall)
# ---------------------------------------------------------------------------

def build_resupply_closet(y):
    parts = []
    closet_z = HALF_D - 1.0
    closet_y = y + 1.5

    # Cabinet body
    parts.append(create_colored_cube("closet_body",
        (2.4, 2.8, 1.2), (0, closet_y, closet_z), COL_CLOSET_BODY))
    # Doors
    parts.append(create_colored_cube("closet_door_l",
        (1.0, 2.6, 0.1), (-0.55, closet_y, closet_z - 0.55), COL_CLOSET_DOOR))
    parts.append(create_colored_cube("closet_door_r",
        (1.0, 2.6, 0.1), (0.55, closet_y, closet_z - 0.55), COL_CLOSET_DOOR))
    # Door gap
    parts.append(create_colored_cube("closet_gap",
        (0.04, 2.6, 0.02), (0, closet_y, closet_z - 0.56), COL_DOOR_DARK))
    # Handles (brass)
    parts.append(create_colored_cube("closet_handle_l",
        (0.06, 0.3, 0.06), (-0.15, closet_y, closet_z - 0.62), COL_BRASS))
    parts.append(create_colored_cube("closet_handle_r",
        (0.06, 0.3, 0.06), (0.15, closet_y, closet_z - 0.62), COL_BRASS))
    # Glow panel
    parts.append(create_colored_cube("closet_glow",
        (1.8, 0.3, 0.08), (0, closet_y + 1.6, closet_z - 0.5), COL_CLOSET_GLOW))
    # Status lights
    parts.append(create_colored_cube("closet_status_l",
        (0.1, 0.1, 0.1), (-1.3, closet_y + 0.8, closet_z - 0.5), COL_CLOSET_GLOW))
    parts.append(create_colored_cube("closet_status_r",
        (0.1, 0.1, 0.1), (1.3, closet_y + 0.8, closet_z - 0.5), COL_CLOSET_GLOW))
    # Label plaque
    parts.append(create_colored_cube("closet_label",
        (1.4, 0.2, 0.04), (0, closet_y + 1.95, closet_z - 0.45), COL_STEEL))

    return parts

# ---------------------------------------------------------------------------
# He-3 tank area (south wall)
# ---------------------------------------------------------------------------

def build_he3_tank(y):
    parts = []
    tank_x = INTERIOR_W * 0.3
    tank_z = -(HALF_D - 2.5)
    tank_y = y + 2.0
    bowl_r = 1.0
    wall_z = -(HALF_D - 0.4)

    # Tripod legs
    for li in range(3):
        la = li * (2.0 * math.pi / 3.0) + 0.5
        lx = tank_x + math.cos(la) * 0.7
        lz = tank_z + math.sin(la) * 0.7
        leg_top = tank_y - bowl_r - 0.05
        parts.append(create_colored_cube(f"tank_leg_{li}",
            (0.08, leg_top - y, 0.08), (lx, y + (leg_top - y) * 0.5, lz), COL_STEEL))
        parts.append(create_colored_cube(f"tank_foot_{li}",
            (0.12, 0.03, 0.12), (lx, y + 0.02, lz), COL_STEEL))

    # Support ring
    parts.append(create_colored_cube("tank_ring",
        (0.5, 0.04, 0.5), (tank_x, tank_y - bowl_r - 0.05, tank_z), COL_STEEL))

    # Tank sphere (approximated with icosphere-like shape)
    parts.append(create_cylinder("tank_sphere", bowl_r * 0.85, bowl_r * 1.5,
        (tank_x, tank_y, tank_z), COL_TANK_BLUE, segments=12))

    # Wall-mounted cylinders
    parts.append(create_cylinder("tank_cyl_l", 0.12, 1.2,
        (tank_x - 0.5, y + 0.6, wall_z), COL_TANK_BODY, segments=8))
    parts.append(create_cylinder("tank_cyl_r", 0.1, 1.0,
        (tank_x + 0.5, y + 0.5, wall_z), COL_TANK_BODY, segments=8))

    # Brass cap on top
    parts.append(create_colored_cube("tank_cap",
        (0.35, 0.14, 0.35), (tank_x, tank_y + bowl_r - 0.05, tank_z), COL_BRASS))

    # Valve
    parts.append(create_colored_cube("tank_valve",
        (0.14, 0.14, 0.14), (tank_x, tank_y + bowl_r + 0.08, tank_z), COL_STEEL))

    # Red handle
    parts.append(create_colored_cube("tank_handle",
        (0.22, 0.04, 0.04), (tank_x + 0.15, tank_y + bowl_r + 0.15, tank_z), COL_RED))

    # Status indicator
    parts.append(create_colored_cube("tank_status",
        (0.1, 0.1, 0.1), (tank_x + bowl_r + 0.2, tank_y, tank_z), COL_TANK_BLUE))

    # Label plaque
    parts.append(create_colored_cube("tank_label",
        (0.8, 0.25, 0.03), (tank_x, tank_y - bowl_r - 0.22, tank_z + 0.01), COL_DOOR_DARK))

    return parts

# ---------------------------------------------------------------------------
# Bar / lounge area (east side)
# ---------------------------------------------------------------------------

def build_bar_lounge(y):
    parts = []

    # Bar counter (east wall area)
    bar_x = HALF_W - 3.0
    bar_z = -3.0
    bar_len = 6.0

    # Bar body
    parts.append(create_colored_cube("bar_body",
        (1.2, 1.1, bar_len), (bar_x, y + 0.55, bar_z), COL_BAR_BODY))
    # Bar top
    parts.append(create_colored_cube("bar_top",
        (1.4, 0.08, bar_len + 0.3), (bar_x, y + 1.12, bar_z), COL_BAR_TOP))
    # Brass rail
    parts.append(create_colored_cube("bar_rail",
        (0.04, 0.04, bar_len - 0.5), (bar_x - 0.65, y + 0.85, bar_z), COL_BRASS))

    # Bar stools (4 in front of bar)
    for si in range(4):
        sz = bar_z - bar_len * 0.5 + 1.0 + si * 1.5
        # Leg
        parts.append(create_colored_cube(f"stool_leg_{si}",
            (0.06, 0.7, 0.06), (bar_x - 1.2, y + 0.35, sz), COL_STEEL))
        # Seat
        parts.append(create_colored_cube(f"stool_seat_{si}",
            (0.35, 0.06, 0.35), (bar_x - 1.2, y + 0.72, sz), COL_STOOL_SEAT))
        # Foot rest ring
        parts.append(create_colored_cube(f"stool_rest_{si}",
            (0.3, 0.03, 0.3), (bar_x - 1.2, y + 0.25, sz), COL_STEEL))

    # Shelving behind bar (against east wall)
    for shelf_y_off in [0.6, 1.2, 1.8]:
        parts.append(create_colored_cube(f"bar_shelf_{shelf_y_off}",
            (0.3, 0.04, bar_len - 1.0),
            (HALF_W - 0.2, y + shelf_y_off, bar_z), COL_STEEL))

    # Bottle placeholders on shelves
    for bi in range(8):
        bz = bar_z - bar_len * 0.5 + 0.8 + bi * 0.7
        parts.append(create_colored_cube(f"bottle_{bi}",
            (0.06, 0.2, 0.06), (HALF_W - 0.2, y + 1.3, bz), COL_BRASS))

    # Lounge tables and chairs (west-center area)
    for ti in range(3):
        tx = -HALF_W + 4.0 + ti * 4.5
        tz = 4.0

        # Table
        parts.append(create_colored_cube(f"table_top_{ti}",
            (1.2, 0.06, 0.8), (tx, y + 0.75, tz), COL_TABLE_TOP))
        # Table leg
        parts.append(create_colored_cube(f"table_leg_{ti}",
            (0.1, 0.75, 0.1), (tx, y + 0.375, tz), COL_STEEL))

        # 2 chairs per table
        for ci, coff in enumerate([-0.7, 0.7]):
            # Seat
            parts.append(create_colored_cube(f"chair_seat_{ti}_{ci}",
                (0.5, 0.06, 0.5), (tx + coff, y + 0.45, tz), COL_CHAIR))
            # Back
            parts.append(create_colored_cube(f"chair_back_{ti}_{ci}",
                (0.5, 0.5, 0.06),
                (tx + coff, y + 0.7, tz + 0.22 * (1 if ci == 0 else -1)), COL_CHAIR))
            # Legs (4)
            for lx, lz in [(-0.2, -0.2), (0.2, -0.2), (-0.2, 0.2), (0.2, 0.2)]:
                parts.append(create_colored_cube(f"chair_leg_{ti}_{ci}_{lx}_{lz}",
                    (0.04, 0.45, 0.04),
                    (tx + coff + lx, y + 0.225, tz + lz), COL_STEEL))

    return parts

# ---------------------------------------------------------------------------
# Nazi propaganda posters — flat quads with vertex-colored emblems
# ---------------------------------------------------------------------------

def build_propaganda_posters(y):
    parts = []

    poster_h = y + 3.2

    def build_poster(name, px, py, pz, facing, poster_type):
        """
        facing: 0=north wall(-Z), 1=east wall(-X), 2=west wall(+X), 3=south wall(+Z)
        poster_type: 0=eagle, 1=iron cross, 2=banner
        """
        poster_parts = []
        rot_y = 0
        if facing == 1:
            rot_y = -math.pi / 2
        elif facing == 2:
            rot_y = math.pi / 2
        elif facing == 3:
            rot_y = math.pi

        # Background quad
        poster_parts.append(create_colored_cube(f"{name}_bg",
            (1.0, 1.4, 0.02), (px, py, pz), COL_PROPAGANDA_BG, rotation=(0, rot_y, 0)))

        # Red border
        border = 0.04
        for bx_off, by_off, bw, bh in [
            (0, 0.68, 1.02, border),
            (0, -0.68, 1.02, border),
            (-0.49, 0, border, 1.38),
            (0.49, 0, border, 1.38),
        ]:
            poster_parts.append(create_colored_cube(f"{name}_border_{bx_off}_{by_off}",
                (bw, bh, 0.025), (px, py, pz), COL_RED, rotation=(0, rot_y, 0)))

        if poster_type == 0:
            # Eagle emblem — simplified geometric eagle
            # Body
            poster_parts.append(create_colored_cube(f"{name}_eagle_body",
                (0.15, 0.25, 0.025), (px, py + 0.1, pz), COL_EAGLE_DARK, rotation=(0, rot_y, 0)))
            # Wings (spread)
            poster_parts.append(create_colored_cube(f"{name}_eagle_wing_l",
                (0.3, 0.08, 0.025), (px, py + 0.15, pz), COL_EAGLE_DARK, rotation=(0, rot_y, 0.15)))
            poster_parts.append(create_colored_cube(f"{name}_eagle_wing_r",
                (0.3, 0.08, 0.025), (px, py + 0.15, pz), COL_EAGLE_DARK, rotation=(0, rot_y, -0.15)))
            # Head
            poster_parts.append(create_colored_cube(f"{name}_eagle_head",
                (0.08, 0.08, 0.025), (px, py + 0.28, pz), COL_EAGLE_DARK, rotation=(0, rot_y, 0)))
            # Circle beneath (wreath)
            poster_parts.append(create_colored_cube(f"{name}_wreath",
                (0.2, 0.2, 0.025), (px, py - 0.15, pz), COL_BRASS, rotation=(0, rot_y, math.pi/4)))

            # Swastika in the wreath — built from cross bars
            sw_sz = 0.06
            # Vertical bar
            poster_parts.append(create_colored_cube(f"{name}_sw_v",
                (sw_sz, sw_sz * 3, 0.026), (px, py - 0.15, pz), COL_SWASTIKA, rotation=(0, rot_y, 0)))
            # Horizontal bar
            poster_parts.append(create_colored_cube(f"{name}_sw_h",
                (sw_sz * 3, sw_sz, 0.026), (px, py - 0.15, pz), COL_SWASTIKA, rotation=(0, rot_y, 0)))
            # Arms (4 stubs at ends of the cross)
            poster_parts.append(create_colored_cube(f"{name}_sw_a1",
                (sw_sz, sw_sz, 0.026), (px, py - 0.15, pz), COL_SWASTIKA, rotation=(0, rot_y, 0)))

        elif poster_type == 1:
            # Iron cross
            poster_parts.append(create_colored_cube(f"{name}_cross_v",
                (0.08, 0.35, 0.025), (px, py, pz), COL_IRON_CROSS, rotation=(0, rot_y, 0)))
            poster_parts.append(create_colored_cube(f"{name}_cross_h",
                (0.35, 0.08, 0.025), (px, py, pz), COL_IRON_CROSS, rotation=(0, rot_y, 0)))
            # Gold edging
            poster_parts.append(create_colored_cube(f"{name}_cross_gold",
                (0.06, 0.33, 0.026), (px, py, pz), COL_BRASS, rotation=(0, rot_y, 0)))
            poster_parts.append(create_colored_cube(f"{name}_cross_gold_h",
                (0.33, 0.06, 0.026), (px, py, pz), COL_BRASS, rotation=(0, rot_y, 0)))

        elif poster_type == 2:
            # Red banner with white circle and swastika
            poster_parts.append(create_colored_cube(f"{name}_banner_bg",
                (0.8, 1.1, 0.022), (px, py, pz), COL_BANNER_RED, rotation=(0, rot_y, 0)))
            # White circle (diamond rotated)
            poster_parts.append(create_colored_cube(f"{name}_banner_circle",
                (0.35, 0.35, 0.023), (px, py, pz), COL_BANNER_WHITE, rotation=(0, rot_y, math.pi/4)))
            # Swastika cross
            poster_parts.append(create_colored_cube(f"{name}_banner_sw_v",
                (0.04, 0.22, 0.024), (px, py, pz), COL_SWASTIKA, rotation=(0, rot_y, 0)))
            poster_parts.append(create_colored_cube(f"{name}_banner_sw_h",
                (0.22, 0.04, 0.024), (px, py, pz), COL_SWASTIKA, rotation=(0, rot_y, 0)))

        return poster_parts

    # North wall — 2 posters flanking the closet
    parts.extend(build_poster("poster_n1", -5.0, poster_h, HALF_D - 0.16, 0, 0))
    parts.extend(build_poster("poster_n2", 5.0, poster_h, HALF_D - 0.16, 0, 2))

    # East wall — 1 poster
    parts.extend(build_poster("poster_e1", HALF_W - 0.16, poster_h, -5.0, 1, 1))

    # West wall — 1 poster
    parts.extend(build_poster("poster_w1", -HALF_W + 0.16, poster_h, -5.0, 2, 0))

    # South wall — 1 poster above door (banner type)
    parts.extend(build_poster("poster_s1", 0, y + INTERIOR_H - 1.2, -HALF_D + 0.16, 3, 2))

    return parts

# ---------------------------------------------------------------------------
# Material setup
# ---------------------------------------------------------------------------

def setup_vertex_color_material():
    mat = bpy.data.materials.new(name="MoonbaseInteriorMat")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    for node in nodes:
        nodes.remove(node)

    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (100, 0)
    bsdf.inputs['Metallic'].default_value = 0.4
    bsdf.inputs['Roughness'].default_value = 0.6

    vcol = nodes.new('ShaderNodeVertexColor')
    vcol.location = (-200, 0)
    vcol.layer_name = "Col"

    links.new(vcol.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    clear_scene()

    # Interior is built at y=0; the game offsets it to interiorY at runtime
    y = 0.0

    all_parts = []
    all_parts.extend(build_floor(y))
    all_parts.extend(build_walls(y))
    all_parts.extend(build_ceiling(y))
    all_parts.extend(build_doors(y))
    all_parts.extend(build_resupply_closet(y))
    all_parts.extend(build_he3_tank(y))
    all_parts.extend(build_bar_lounge(y))
    all_parts.extend(build_propaganda_posters(y))

    # Apply material
    mat = setup_vertex_color_material()
    for obj in all_parts:
        if obj.type == 'MESH':
            obj.data.materials.append(mat)

    # Select and join all
    bpy.ops.object.select_all(action='DESELECT')
    for obj in all_parts:
        if obj.type == 'MESH':
            obj.select_set(True)
    if all_parts:
        bpy.context.view_layer.objects.active = all_parts[0]
        bpy.ops.object.join()

    final = bpy.context.active_object
    final.name = "moonbase_interior"

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)

    bpy.ops.export_scene.gltf(
        filepath=OUTPUT_PATH,
        export_format='GLB',
        export_colors=True,
        export_normals=True,
        export_apply=True,
        use_selection=True,
    )

    print(f"Exported moonbase interior to: {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
