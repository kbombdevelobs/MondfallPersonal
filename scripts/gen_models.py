#!/usr/bin/env python3
"""Generate MagicaVoxel-style .obj models for Mondfall."""
import os

OUT = os.path.join(os.path.dirname(__file__), '..', 'assets', 'models')
os.makedirs(OUT, exist_ok=True)

# ---- OBJ writer ----
class VoxelModel:
    def __init__(self, name):
        self.name = name
        self.voxels = []  # (x, y, z, r, g, b)

    def add(self, x, y, z, r, g, b):
        self.voxels.append((x, y, z, r, g, b))

    def box(self, x0, y0, z0, x1, y1, z1, r, g, b):
        """Fill a box region with voxels."""
        for x in range(x0, x1):
            for y in range(y0, y1):
                for z in range(z0, z1):
                    self.add(x, y, z, r, g, b)

    def save(self, scale=0.05):
        """Write .obj and .mtl files. Groups faces by material to avoid excessive usemtl switches."""
        obj_path = os.path.join(OUT, f'{self.name}.obj')
        mtl_path = os.path.join(OUT, f'{self.name}.mtl')

        # Collect unique colors -> material names
        colors = {}
        for v in self.voxels:
            c = (v[3], v[4], v[5])
            if c not in colors:
                colors[c] = f'mat_{len(colors)}'

        # Write MTL
        with open(mtl_path, 'w') as f:
            for (r, g, b), name in colors.items():
                f.write(f'newmtl {name}\n')
                f.write(f'Kd {r/255:.4f} {g/255:.4f} {b/255:.4f}\n')
                f.write(f'Ka 0.1 0.1 0.1\n')
                f.write(f'Ks 0.3 0.3 0.3\n')
                f.write(f'Ns 20\n\n')

        # Build occupied set
        occupied = set()
        for v in self.voxels:
            occupied.add((v[0], v[1], v[2]))

        # Group faces by material
        mat_faces = {}  # mat_name -> list of quads (each quad = 4 vertices)
        for vx, vy, vz, r, g, b in self.voxels:
            c = (r, g, b)
            mat = colors[c]
            if mat not in mat_faces:
                mat_faces[mat] = []

            x, y, z = vx * scale, vy * scale, vz * scale
            s = scale

            if (vx+1, vy, vz) not in occupied:
                mat_faces[mat].append(([x+s,y,z], [x+s,y+s,z], [x+s,y+s,z+s], [x+s,y,z+s]))
            if (vx-1, vy, vz) not in occupied:
                mat_faces[mat].append(([x,y,z+s], [x,y+s,z+s], [x,y+s,z], [x,y,z]))
            if (vx, vy+1, vz) not in occupied:
                mat_faces[mat].append(([x,y+s,z], [x,y+s,z+s], [x+s,y+s,z+s], [x+s,y+s,z]))
            if (vx, vy-1, vz) not in occupied:
                mat_faces[mat].append(([x,y,z+s], [x,y,z], [x+s,y,z], [x+s,y,z+s]))
            if (vx, vy, vz+1) not in occupied:
                mat_faces[mat].append(([x+s,y,z+s], [x+s,y+s,z+s], [x,y+s,z+s], [x,y,z+s]))
            if (vx, vy, vz-1) not in occupied:
                mat_faces[mat].append(([x,y,z], [x,y+s,z], [x+s,y+s,z], [x+s,y,z]))

        # Write OBJ — one usemtl per material, all its faces grouped together
        with open(obj_path, 'w') as f:
            f.write(f'mtllib {self.name}.mtl\n')
            vi = 1

            for mat, quads in mat_faces.items():
                f.write(f'usemtl {mat}\n')
                for quad in quads:
                    # Write 4 vertices, then 2 triangles (not 1 quad)
                    for vert in quad:
                        f.write(f'v {vert[0]:.4f} {vert[1]:.4f} {vert[2]:.4f}\n')
                    f.write(f'f {vi} {vi+1} {vi+2}\n')
                    f.write(f'f {vi} {vi+2} {vi+3}\n')
                    vi += 4

        num_mats = len(mat_faces)
        print(f'  {self.name}: {len(self.voxels)} voxels, {vi-1} verts, {num_mats} materials -> {obj_path}')


# ============================================================
# MOND-MP40: Space-age MP40
# ============================================================
def gen_mond_mp40():
    m = VoxelModel('mond_mp40')
    S = (175, 180, 190)  # silver/chrome
    D = (140, 145, 155)  # dark metal
    C = (0, 200, 255)    # cyan energy
    G = (55, 55, 60)     # grip dark
    E = (0, 255, 210)    # energy indicator

    # Receiver body (z=0..8, centered roughly)
    for z in range(0, 9):
        m.box(0, 0, z, 2, 2, z+1, *S)
    # Upper rail
    for z in range(0, 8):
        m.add(0, 2, z, *D); m.add(1, 2, z, *D)
    # Barrel (extends forward)
    for z in range(9, 14):
        m.add(0, 0, z, *D); m.add(1, 0, z, *D)
        m.add(0, 1, z, *D); m.add(1, 1, z, *D)
    # Muzzle brake
    for dy in range(-1, 3):
        for dx in range(-1, 3):
            m.add(dx, dy, 14, *D)
    # Cooling fins
    for z in [9, 10, 11, 12]:
        m.add(-1, 0, z, *S); m.add(2, 0, z, *S)
        m.add(-1, 1, z, *S); m.add(2, 1, z, *S)
    # Energy coils (sides)
    for z in range(2, 12):
        m.add(-1, 1, z, *C)
        m.add(2, 1, z, *C)
    # Side magazine (left, hanging down)
    for y in range(-4, 1):
        m.add(-1, y, 2, *S); m.add(-2, y, 2, *S)
        m.add(-1, y, 3, *S); m.add(-2, y, 3, *S)
    # Magazine energy window
    for y in range(-3, 0):
        m.add(-3, y, 2, *C); m.add(-3, y, 3, *C)
    # Pistol grip
    for y in range(-3, 0):
        m.add(0, y, 1, *G); m.add(1, y, 1, *G)
    # Folding stock (wire style)
    for z in range(-5, 0):
        m.add(0, 0, z, *S)
        m.add(1, 0, z, *S)
    m.add(0, -1, -5, *S); m.add(1, -1, -5, *S)
    m.add(0, 2, -5, *S); m.add(1, 2, -5, *S)
    # Rear energy indicator
    m.add(0, 2, 0, *E); m.add(1, 2, 0, *E)

    m.save(scale=0.04)

# ============================================================
# RAKETENFAUST: Wacky 60s space Panzerfaust
# ============================================================
def gen_raketenfaust():
    m = VoxelModel('raketenfaust')
    OD = (115, 125, 108)  # olive drab
    DK = (90, 100, 85)    # dark
    Y  = (200, 180, 45)   # hazard yellow
    G  = (80, 70, 50)     # grip
    GL = (200, 220, 255)  # glass/lens
    OR = (255, 160, 50)   # rocket orange
    CH = (155, 160, 150)  # chrome fins

    # Main tube (3x3 cross section, long)
    for z in range(-4, 10):
        m.box(0, 0, z, 3, 3, z+1, *OD)
    # Flared bell (front)
    for z in [10, 11]:
        m.box(-1, -1, z, 4, 4, z+1, *OD)
    for z in [12]:
        m.box(-2, -2, z, 5, 5, z+1, *DK)
    # Tail fins (4 directions)
    for z in range(-6, -3):
        m.add(3, 1, z, *CH); m.add(-1, 1, z, *CH)  # left/right
        m.add(1, 3, z, *CH); m.add(1, -1, z, *CH)   # top/bottom
    # Bubble sight on top
    m.add(1, 4, 4, *GL); m.add(1, 4, 5, *GL)
    m.add(1, 3, 4, *DK); m.add(1, 3, 5, *DK)
    # Grip underneath
    for y in range(-3, 0):
        m.add(1, y, -1, *G); m.add(1, y, 0, *G)
    # Hazard stripes
    for z in [-2, 1, 4]:
        m.add(0, -1, z, *Y); m.add(1, -1, z, *Y); m.add(2, -1, z, *Y)
    # Rear exhaust
    m.box(-1, -1, -6, 4, 4, -5, *DK)
    # Rocket visible inside
    m.add(1, 1, 8, *OR); m.add(1, 1, 9, *OR)

    m.save(scale=0.045)

# ============================================================
# JACKHAMMER
# ============================================================
def gen_jackhammer():
    m = VoxelModel('jackhammer')
    YL = (180, 170, 50)  # yellow body
    GR = (100, 100, 92)  # grip
    MT = (150, 150, 145) # metal piston
    RD = (200, 50, 30)   # hazard red
    ST = (200, 200, 195) # steel bit

    # Main shaft
    for y in range(0, 10):
        m.add(1, y, 1, *YL); m.add(2, y, 1, *YL)
        m.add(1, y, 2, *YL); m.add(2, y, 2, *YL)
    # Grip bars
    for x in range(0, 4):
        m.add(x, 9, 1, *GR); m.add(x, 9, 2, *GR)
        m.add(x, 7, 1, *GR); m.add(x, 7, 2, *GR)
    # Piston housing
    m.box(0, -3, 0, 4, 0, 4, *MT)
    # Warning stripes
    m.add(0, -1, 0, *RD); m.add(0, -1, 3, *RD)
    m.add(3, -1, 0, *RD); m.add(3, -1, 3, *RD)
    # Drill bit
    for y in range(-7, -3):
        m.add(1, y, 1, *ST); m.add(2, y, 1, *ST)
        m.add(1, y, 2, *ST); m.add(2, y, 2, *ST)
    # Tip
    m.add(1, -8, 1, *ST); m.add(2, -8, 2, *ST)

    m.save(scale=0.04)

# ============================================================
# SOVIET PPSh (space age)
# ============================================================
def gen_soviet_gun():
    m = VoxelModel('soviet_ppsh')
    CH = (170, 175, 182)  # chrome
    DK = (145, 150, 158)  # dark chrome
    RD = (255, 60, 40)    # red energy
    G  = (55, 55, 60)     # grip

    # Body
    for z in range(0, 8):
        m.box(0, 0, z, 2, 2, z+1, *CH)
    # Stubby barrel
    for z in range(8, 11):
        m.add(0, 0, z, *DK); m.add(1, 0, z, *DK)
        m.add(0, 1, z, *DK); m.add(1, 1, z, *DK)
    # Muzzle
    m.box(-1, -1, 11, 3, 3, 12, *DK)
    # Drum magazine
    for dx in range(-1, 3):
        for dz in range(1, 4):
            m.add(dx, -1, dz, *CH)
            m.add(dx, -2, dz, *CH)
    # Red energy strips
    for z in range(1, 9):
        m.add(0, 2, z, *RD)
        m.add(1, -1, z, *RD)
    # Stock
    for z in range(-4, 0):
        m.add(0, 0, z, *CH); m.add(1, 0, z, *CH)

    m.save(scale=0.03)

# ============================================================
# AMERICAN RAY GUN
# ============================================================
def gen_american_gun():
    m = VoxelModel('american_raygun')
    WH = (210, 215, 222)  # white chrome
    CH = (190, 195, 202)  # chrome
    BL = (80, 160, 255)   # blue energy
    BW = (180, 220, 255)  # blue white
    G  = (195, 200, 208)  # grip

    # Sleek body
    for z in range(0, 7):
        m.add(0, 0, z, *WH); m.add(1, 0, z, *WH)
        m.add(0, 1, z, *WH); m.add(1, 1, z, *WH)
    # Antenna dish barrel (flared)
    m.box(-1, -1, 7, 3, 3, 8, *CH)
    m.box(-2, -2, 8, 4, 4, 9, *CH)
    # Blue energy orb on top
    m.add(0, 2, 2, *BL); m.add(1, 2, 2, *BL)
    m.add(0, 2, 3, *BL); m.add(1, 2, 3, *BL)
    m.add(0, 3, 2, *BW); m.add(1, 3, 3, *BW)
    # Energy strip
    for z in range(0, 7):
        m.add(0, 2, z, *BL)
    # Pistol grip
    for y in range(-3, 0):
        m.add(0, y, 1, *G); m.add(1, y, 1, *G)

    m.save(scale=0.03)

# ============================================================
# MOON ROCKS (3 variants)
# ============================================================
def gen_rock(name, seed_offset, size_range):
    import random
    random.seed(42 + seed_offset)
    m = VoxelModel(name)

    sx, sy, sz = size_range
    for x in range(sx):
        for y in range(sy):
            for z in range(sz):
                # Roughly spherical with noise
                cx, cy, cz = sx/2, sy/2, sz/2
                dx, dy, dz = (x - cx) / cx, (y - cy) / cy, (z - cz) / cz
                dist = (dx*dx + dy*dy + dz*dz) ** 0.5
                threshold = 0.85 + random.random() * 0.35
                if dist < threshold:
                    # Quantize to 4 shades to keep material count low
                    shade = [80, 95, 110, 125][random.randint(0, 3)]
                    m.add(x, y, z, shade, shade, shade - 8)

    m.save(scale=0.12)

def gen_rock_large():
    gen_rock('rock_large', 0, (10, 7, 9))

def gen_rock_medium():
    gen_rock('rock_medium', 10, (7, 5, 6))

def gen_rock_small():
    gen_rock('rock_small', 20, (5, 4, 5))

# ============================================================
# ASTRONAUT PARTS
# ============================================================
def gen_astronaut_torso():
    m = VoxelModel('astro_torso')
    W = (220, 218, 210)  # white suit
    S = (200, 198, 190)  # seam
    BT = (160, 158, 150) # belt

    # Main torso block
    m.box(0, 0, 0, 7, 10, 5, *W)
    # Seam lines
    for x in range(7):
        m.add(x, 5, 0, *S); m.add(x, 5, 4, *S)
    for z in range(5):
        m.add(3, 5, z, *S)
    # Belt
    for x in range(7):
        for z in range(5):
            m.add(x, 0, z, *BT)

    m.save(scale=0.12)

def gen_astronaut_helmet():
    m = VoxelModel('astro_helmet')
    W = (215, 215, 210)
    import random
    random.seed(99)

    # Sphere-ish helmet
    r = 5
    for x in range(-r, r+1):
        for y in range(-r, r+1):
            for z in range(-r, r+1):
                d = (x*x + y*y + z*z) ** 0.5
                if d <= r and d > r - 1.8:
                    shade = [205, 212, 220][random.randint(0, 2)]
                    m.add(x + r, y + r, z + r, shade, shade, shade - 5)

    m.save(scale=0.07)

def gen_astronaut_backpack():
    m = VoxelModel('astro_backpack')
    G = (180, 178, 172)
    D = (140, 138, 132)
    Y = (200, 180, 50)

    m.box(0, 0, 0, 5, 7, 3, *G)
    # Vents
    for y in [1, 3, 5]:
        m.box(1, y, 0, 4, y+1, 1, *D)
    # Warning label
    m.box(1, 1, 0, 4, 3, 1, *Y)
    # Hose connectors on top
    m.add(1, 7, 1, *D); m.add(3, 7, 1, *D)

    m.save(scale=0.08)


# ============================================================
# GENERATE ALL
# ============================================================
if __name__ == '__main__':
    print('Generating Mondfall voxel models...')
    gen_mond_mp40()
    gen_raketenfaust()
    gen_jackhammer()
    gen_soviet_gun()
    gen_american_gun()
    gen_rock_large()
    gen_rock_medium()
    gen_rock_small()
    gen_astronaut_torso()
    gen_astronaut_helmet()
    gen_astronaut_backpack()
    print('Done!')
