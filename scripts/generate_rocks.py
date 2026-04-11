"""
Generate moon rock .glb models via Blender's Python API.

Usage:
    blender --background --python scripts/generate_rocks.py

Produces 3 rock variants in resources/models/rocks/:
    rock_small.glb   ~0.5m low-poly boulder
    rock_medium.glb  ~1.5m irregular rocky shape
    rock_large.glb   ~3.0m large lunar boulder with flat facets
"""

import bpy
import bmesh
import os
import random
import math
from mathutils import Vector, noise

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                          "resources", "models", "rocks")

# Lunar rock color palette (linear RGB 0-1)
COLOR_BASE    = (120/255, 115/255, 105/255, 1.0)  # lunar regolith gray
COLOR_DARK    = ( 80/255,  75/255,  70/255, 1.0)  # shadow areas
COLOR_LIGHT   = (160/255, 155/255, 145/255, 1.0)  # sun-facing facets

# Rock definitions: (name, scale, subdivisions, displacement_strength, seed)
ROCK_DEFS = [
    ("rock_small",  0.5,  2, 0.08, 42),
    ("rock_medium", 1.5,  3, 0.25, 137),
    ("rock_large",  3.0,  3, 0.50, 314),
]


def clear_scene():
    """Remove all objects from the scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    # Remove orphan meshes
    for mesh in bpy.data.meshes:
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        if mat.users == 0:
            bpy.data.materials.remove(mat)


def lerp_color(a, b, t):
    """Linearly interpolate between two RGBA tuples."""
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(4))


def generate_rock(name, scale, subdivisions, disp_strength, seed):
    """Generate a single rock mesh with vertex colors and export as .glb."""
    clear_scene()
    random.seed(seed)

    # Start with an icosphere
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=subdivisions,
        radius=scale * 0.5,
        location=(0, 0, 0)
    )
    obj = bpy.context.active_object
    obj.name = name

    # Switch to edit mode via bmesh for vertex displacement
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.verts.ensure_lookup_table()

    # Displace vertices for organic rocky shape
    for v in bm.verts:
        # Multi-octave noise displacement
        pos = v.co
        n1 = noise.noise(pos * 2.0 + Vector((seed, 0, 0))) * disp_strength
        n2 = noise.noise(pos * 4.0 + Vector((0, seed, 0))) * disp_strength * 0.5
        n3 = noise.noise(pos * 8.0 + Vector((0, 0, seed))) * disp_strength * 0.25

        displacement = n1 + n2 + n3

        # Random per-vertex jitter for irregularity
        jitter = random.uniform(-disp_strength * 0.15, disp_strength * 0.15)

        # Displace along normal direction
        normal = v.normal.copy()
        if normal.length > 0:
            v.co += normal * (displacement + jitter)

    # For large rocks: add flat facet effect by slightly flattening some faces
    if scale >= 3.0:
        bm.faces.ensure_lookup_table()
        for face in bm.faces:
            if random.random() < 0.3:
                # Flatten face by moving vertices toward face center plane
                center = face.calc_center_median()
                fnorm = face.normal.copy()
                if fnorm.length > 0:
                    for v in face.verts:
                        offset = v.co - center
                        proj = offset.dot(fnorm)
                        v.co -= fnorm * proj * 0.3

    # Squash slightly in Y for a settled-on-ground look
    for v in bm.verts:
        v.co.z *= random.uniform(0.7, 0.9)

    bm.to_mesh(obj.data)
    bm.free()

    # Recalculate normals
    obj.data.calc_normals()

    # Create vertex color layer
    if not obj.data.vertex_colors:
        obj.data.vertex_colors.new(name="Col")
    color_layer = obj.data.vertex_colors["Col"]

    # Assign vertex colors
    random.seed(seed + 1000)
    mesh = obj.data

    # Compute a rough "up-facing" factor per vertex for lighting variation
    # Build a vertex-normal map from face corners
    vert_normals = {}
    for poly in mesh.polygons:
        for idx in poly.vertices:
            if idx not in vert_normals:
                vert_normals[idx] = Vector((0, 0, 0))
            vert_normals[idx] += poly.normal
    for idx in vert_normals:
        if vert_normals[idx].length > 0:
            vert_normals[idx].normalize()

    for poly in mesh.polygons:
        for loop_idx in poly.loop_indices:
            vert_idx = mesh.loops[loop_idx].vertex_index
            vert_pos = mesh.vertices[vert_idx].co

            # Noise-based color variation
            cn = noise.noise(vert_pos * 3.0 + Vector((seed * 0.1, 0, 0)))
            # cn ranges roughly -1..1

            # Blend between dark, base, and light based on noise + height
            vnorm = vert_normals.get(vert_idx, Vector((0, 0, 1)))
            up_factor = max(0.0, vnorm.dot(Vector((0, 0, 1))))  # how much faces up

            # Combine noise and orientation for color selection
            t = (cn + 1.0) * 0.5  # remap to 0..1
            t = t * 0.6 + up_factor * 0.4  # blend with facing direction

            # Per-vertex random scatter
            t += random.uniform(-0.15, 0.15)
            t = max(0.0, min(1.0, t))

            if t < 0.35:
                color = lerp_color(COLOR_DARK, COLOR_BASE, t / 0.35)
            elif t < 0.7:
                color = lerp_color(COLOR_BASE, COLOR_LIGHT, (t - 0.35) / 0.35)
            else:
                color = lerp_color(COLOR_LIGHT, COLOR_BASE, (t - 0.7) / 0.3)

            color_layer.data[loop_idx].color = color

    # Create a simple material that uses vertex colors
    mat = bpy.data.materials.new(name=f"{name}_mat")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    # Clear default nodes
    for node in nodes:
        nodes.remove(node)

    # Vertex Color node -> Principled BSDF -> Output
    output_node = nodes.new(type='ShaderNodeOutputMaterial')
    output_node.location = (400, 0)

    bsdf_node = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf_node.location = (100, 0)
    bsdf_node.inputs['Roughness'].default_value = 0.95
    bsdf_node.inputs['Specular IOR Level'].default_value = 0.1

    vcol_node = nodes.new(type='ShaderNodeVertexColor')
    vcol_node.location = (-200, 0)
    vcol_node.layer_name = "Col"

    links.new(vcol_node.outputs['Color'], bsdf_node.inputs['Base Color'])
    links.new(bsdf_node.outputs['BSDF'], output_node.inputs['Surface'])

    obj.data.materials.clear()
    obj.data.materials.append(mat)

    # Apply transforms
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # Export as .glb
    output_path = os.path.join(OUTPUT_DIR, f"{name}.glb")
    bpy.ops.export_scene.gltf(
        filepath=output_path,
        export_format='GLB',
        export_apply=True,
        export_colors=True,
        export_normals=True,
        use_selection=True,
        export_yup=True,
    )
    print(f"Exported: {output_path}")


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for name, scale, subdiv, disp, seed in ROCK_DEFS:
        print(f"Generating {name} (scale={scale}, subdiv={subdiv})...")
        generate_rock(name, scale, subdiv, disp, seed)

    print(f"\nAll rocks exported to: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
