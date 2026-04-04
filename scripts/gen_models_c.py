#!/usr/bin/env python3
"""Generate voxel models as C source code — compiled into the binary, no file loading."""
import os

OUT = os.path.join(os.path.dirname(__file__), '..', 'src')

class VoxelModel:
    def __init__(self, name):
        self.name = name
        self.voxels = []

    def add(self, x, y, z, r, g, b):
        self.voxels.append((x, y, z, r, g, b))

    def box(self, x0, y0, z0, x1, y1, z1, r, g, b):
        for x in range(x0, x1):
            for y in range(y0, y1):
                for z in range(z0, z1):
                    self.add(x, y, z, r, g, b)

    def generate_c(self, scale=0.04):
        """Return C code that builds a raylib Mesh with colored vertices."""
        occupied = set((v[0], v[1], v[2]) for v in self.voxels)

        # Collect triangles with vertex colors
        tris = []  # list of (v0, v1, v2, r, g, b) where v = (x,y,z)
        for vx, vy, vz, r, g, b in self.voxels:
            x, y, z, s = vx * scale, vy * scale, vz * scale, scale
            faces = []
            if (vx+1, vy, vz) not in occupied:
                faces.append(((x+s,y,z), (x+s,y+s,z), (x+s,y+s,z+s), (x+s,y,z+s)))
            if (vx-1, vy, vz) not in occupied:
                faces.append(((x,y,z+s), (x,y+s,z+s), (x,y+s,z), (x,y,z)))
            if (vx, vy+1, vz) not in occupied:
                faces.append(((x,y+s,z), (x,y+s,z+s), (x+s,y+s,z+s), (x+s,y+s,z)))
            if (vx, vy-1, vz) not in occupied:
                faces.append(((x,y,z+s), (x,y,z), (x+s,y,z), (x+s,y,z+s)))
            if (vx, vy, vz+1) not in occupied:
                faces.append(((x+s,y,z+s), (x+s,y+s,z+s), (x,y+s,z+s), (x,y,z+s)))
            if (vx, vy, vz-1) not in occupied:
                faces.append(((x,y,z), (x,y+s,z), (x+s,y+s,z), (x+s,y,z)))
            for q in faces:
                tris.append((q[0], q[1], q[2], r, g, b))
                tris.append((q[0], q[2], q[3], r, g, b))

        nv = len(tris) * 3
        lines = []
        lines.append(f'// Auto-generated voxel model: {self.name}')
        lines.append(f'// {len(self.voxels)} voxels, {nv} vertices, {len(tris)} triangles')
        lines.append(f'static Model GenModel_{self.name}(void) {{')
        lines.append(f'    Mesh mesh = {{0}};')
        lines.append(f'    mesh.vertexCount = {nv};')
        lines.append(f'    mesh.triangleCount = {len(tris)};')
        lines.append(f'    mesh.vertices = RL_CALLOC({nv}*3, sizeof(float));')
        lines.append(f'    mesh.colors = RL_CALLOC({nv}*4, sizeof(unsigned char));')
        lines.append(f'    float *v = mesh.vertices;')
        lines.append(f'    unsigned char *c = mesh.colors;')

        # Write vertex and color data
        for tri in tris:
            v0, v1, v2, r, g, b = tri
            for vt in (v0, v1, v2):
                lines.append(f'    *v++={vt[0]:.4f}f; *v++={vt[1]:.4f}f; *v++={vt[2]:.4f}f; *c++={r}; *c++={g}; *c++={b}; *c++=255;')

        lines.append(f'    UploadMesh(&mesh, false);')
        lines.append(f'    return LoadModelFromMesh(mesh);')
        lines.append(f'}}')
        return '\n'.join(lines)


# ---- MODEL DEFINITIONS (same as before) ----

def make_mond_mp40():
    m = VoxelModel('mond_mp40')
    S, D, C, G, E = (175,180,190), (140,145,155), (0,200,255), (55,55,60), (0,255,210)
    for z in range(0,9): m.box(0,0,z,2,2,z+1,*S)
    for z in range(0,8): m.add(0,2,z,*D); m.add(1,2,z,*D)
    for z in range(9,14):
        for dx in range(2):
            for dy in range(2): m.add(dx,dy,z,*D)
    for dy in range(-1,3):
        for dx in range(-1,3): m.add(dx,dy,14,*D)
    for z in [9,10,11,12]:
        m.add(-1,0,z,*S); m.add(2,0,z,*S); m.add(-1,1,z,*S); m.add(2,1,z,*S)
    for z in range(2,12): m.add(-1,1,z,*C); m.add(2,1,z,*C)
    for y in range(-4,1): m.box(-2,y,2,-1+1,y+1,4,*S)
    for y in range(-3,0): m.add(-3,y,2,*C); m.add(-3,y,3,*C)
    for y in range(-3,0): m.add(0,y,1,*G); m.add(1,y,1,*G)
    for z in range(-5,0): m.add(0,0,z,*S); m.add(1,0,z,*S)
    m.add(0,-1,-5,*S); m.add(1,-1,-5,*S); m.add(0,2,-5,*S); m.add(1,2,-5,*S)
    m.add(0,2,0,*E); m.add(1,2,0,*E)
    return m

def make_raketenfaust():
    m = VoxelModel('raketenfaust')
    OD,DK,Y,G,GL,OR,CH = (115,125,108),(90,100,85),(200,180,45),(80,70,50),(200,220,255),(255,160,50),(155,160,150)
    for z in range(-4,10): m.box(0,0,z,3,3,z+1,*OD)
    for z in [10,11]: m.box(-1,-1,z,4,4,z+1,*OD)
    m.box(-2,-2,12,5,5,13,*DK)
    for z in range(-6,-3):
        m.add(3,1,z,*CH); m.add(-1,1,z,*CH); m.add(1,3,z,*CH); m.add(1,-1,z,*CH)
    m.add(1,4,4,*GL); m.add(1,4,5,*GL); m.add(1,3,4,*DK); m.add(1,3,5,*DK)
    for y in range(-3,0): m.add(1,y,-1,*G); m.add(1,y,0,*G)
    for z in [-2,1,4]: m.add(0,-1,z,*Y); m.add(1,-1,z,*Y); m.add(2,-1,z,*Y)
    m.box(-1,-1,-6,4,4,-5,*DK)
    m.add(1,1,8,*OR); m.add(1,1,9,*OR)
    return m

def make_jackhammer():
    m = VoxelModel('jackhammer')
    YL,GR,MT,RD,ST = (180,170,50),(100,100,92),(150,150,145),(200,50,30),(200,200,195)
    for y in range(0,10): m.box(1,y,1,3,y+1,3,*YL)
    for x in range(0,4): m.add(x,9,1,*GR); m.add(x,9,2,*GR); m.add(x,7,1,*GR); m.add(x,7,2,*GR)
    m.box(0,-3,0,4,0,4,*MT)
    m.add(0,-1,0,*RD); m.add(0,-1,3,*RD); m.add(3,-1,0,*RD); m.add(3,-1,3,*RD)
    for y in range(-7,-3): m.box(1,y,1,3,y+1,3,*ST)
    m.add(1,-8,1,*ST); m.add(2,-8,2,*ST)
    return m

def make_soviet_gun():
    m = VoxelModel('soviet_ppsh')
    CH,DK,RD = (170,175,182),(145,150,158),(255,60,40)
    for z in range(0,8): m.box(0,0,z,2,2,z+1,*CH)
    for z in range(8,11):
        for dx in range(2):
            for dy in range(2): m.add(dx,dy,z,*DK)
    m.box(-1,-1,11,3,3,12,*DK)
    for dx in range(-1,3):
        for dz in range(1,4): m.add(dx,-1,dz,*CH); m.add(dx,-2,dz,*CH)
    for z in range(1,9): m.add(0,2,z,*RD); m.add(1,-1,z,*RD)
    for z in range(-4,0): m.add(0,0,z,*CH); m.add(1,0,z,*CH)
    return m

def make_american_gun():
    m = VoxelModel('american_raygun')
    WH,CH,BL,BW,G = (210,215,222),(190,195,202),(80,160,255),(180,220,255),(195,200,208)
    for z in range(0,7): m.box(0,0,z,2,2,z+1,*WH)
    m.box(-1,-1,7,3,3,8,*CH); m.box(-2,-2,8,4,4,9,*CH)
    m.add(0,2,2,*BL); m.add(1,2,2,*BL); m.add(0,2,3,*BL); m.add(1,2,3,*BL)
    m.add(0,3,2,*BW); m.add(1,3,3,*BW)
    for z in range(0,7): m.add(0,2,z,*BL)
    for y in range(-3,0): m.add(0,y,1,*G); m.add(1,y,1,*G)
    return m

def make_rock(name, seed_offset, size):
    import random
    random.seed(42 + seed_offset)
    m = VoxelModel(name)
    sx,sy,sz = size
    for x in range(sx):
        for y in range(sy):
            for z in range(sz):
                cx,cy,cz = sx/2, sy/2, sz/2
                dx,dy,dz = (x-cx)/cx, (y-cy)/cy, (z-cz)/cz
                dist = (dx*dx+dy*dy+dz*dz)**0.5
                if dist < 0.85 + random.random()*0.35:
                    shade = [80,95,110,125][random.randint(0,3)]
                    m.add(x,y,z,shade,shade,shade-8)
    return m

def make_helmet():
    import random; random.seed(99)
    m = VoxelModel('helmet')
    r = 5
    for x in range(-r,r+1):
        for y in range(-r,r+1):
            for z in range(-r,r+1):
                d = (x*x+y*y+z*z)**0.5
                if d <= r and d > r-1.8:
                    shade = [205,212,220][random.randint(0,2)]
                    m.add(x+r,y+r,z+r, shade,shade,shade-5)
    return m

def make_torso():
    m = VoxelModel('torso')
    W,S,BT = (220,218,210),(200,198,190),(160,158,150)
    m.box(0,0,0,7,10,5,*W)
    for x in range(7): m.add(x,5,0,*S); m.add(x,5,4,*S)
    for z in range(5): m.add(3,5,z,*S)
    for x in range(7):
        for z in range(5): m.add(x,0,z,*BT)
    return m

def make_backpack():
    m = VoxelModel('backpack')
    G,D,Y = (180,178,172),(140,138,132),(200,180,50)
    m.box(0,0,0,5,7,3,*G)
    for y in [1,3,5]: m.box(1,y,0,4,y+1,1,*D)
    m.box(1,1,0,4,3,1,*Y)
    m.add(1,7,1,*D); m.add(3,7,1,*D)
    return m


if __name__ == '__main__':
    models = [
        (make_mond_mp40(), 0.04),
        (make_raketenfaust(), 0.045),
        (make_jackhammer(), 0.04),
        (make_soviet_gun(), 0.03),
        (make_american_gun(), 0.03),
        (make_rock('rock_large', 0, (10,7,9)), 0.12),
        (make_rock('rock_medium', 10, (7,5,6)), 0.12),
        (make_rock('rock_small', 20, (5,4,5)), 0.12),
        (make_torso(), 0.12),
        (make_helmet(), 0.07),
        (make_backpack(), 0.08),
    ]

    header = []
    header.append('#ifndef MODELS_GEN_H')
    header.append('#define MODELS_GEN_H')
    header.append('#include "raylib.h"')
    header.append('#include "rlgl.h"')
    header.append('#include <stdlib.h>')
    header.append('')

    for mdl, scale in models:
        header.append(mdl.generate_c(scale))
        header.append('')
        print(f'  {mdl.name}: {len(mdl.voxels)} voxels')

    header.append('#endif')

    path = os.path.join(OUT, 'models_gen.h')
    with open(path, 'w') as f:
        f.write('\n'.join(header))
    print(f'Written to {path}')
