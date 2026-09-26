import struct
import json
import os
import math

def create_glb(meshes_data, output_path):
    """
    meshes_data: list of dict with:
        'vertices': list of (x, y, z),
        'normals': list of (nx, ny, nz),
        'colors': list of (r, g, b, a),
        'indices': list of uint32
        'material_color': (r, g, b, a)
    """
    bin_data = bytearray()
    buffer_views = []
    accessors = []
    primitives = []
    materials = []

    for m_idx, m in enumerate(meshes_data):
        verts = m['vertices']
        norms = m['normals']
        cols = m['colors']
        inds = m['indices']
        mat_col = m.get('material_color', (0.8, 0.85, 0.9, 1.0))

        # Material
        materials.append({
            "pbrMetallicRoughness": {
                "baseColorFactor": list(mat_col),
                "metallicFactor": 0.8,
                "roughnessFactor": 0.3
            }
        })

        # 1. Position buffer
        pos_offset = len(bin_data)
        min_p = [float('inf')]*3
        max_p = [float('-inf')]*3
        for v in verts:
            bin_data.extend(struct.pack('<fff', v[0], v[1], v[2]))
            for k in range(3):
                min_p[k] = min(min_p[k], v[k])
                max_p[k] = max(max_p[k], v[k])
        pos_len = len(bin_data) - pos_offset

        bv_pos_idx = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": pos_offset,
            "byteLength": pos_len,
            "target": 34962 # ARRAY_BUFFER
        })
        acc_pos_idx = len(accessors)
        accessors.append({
            "bufferView": bv_pos_idx,
            "byteOffset": 0,
            "componentType": 5126, # FLOAT
            "count": len(verts),
            "type": "VEC3",
            "min": min_p,
            "max": max_p
        })

        # 2. Normal buffer
        norm_offset = len(bin_data)
        for n in norms:
            bin_data.extend(struct.pack('<fff', n[0], n[1], n[2]))
        norm_len = len(bin_data) - norm_offset

        bv_norm_idx = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": norm_offset,
            "byteLength": norm_len,
            "target": 34962
        })
        acc_norm_idx = len(accessors)
        accessors.append({
            "bufferView": bv_norm_idx,
            "byteOffset": 0,
            "componentType": 5126,
            "count": len(norms),
            "type": "VEC3"
        })

        # 3. Color buffer
        col_offset = len(bin_data)
        for c in cols:
            bin_data.extend(struct.pack('<ffff', c[0], c[1], c[2], c[3]))
        col_len = len(bin_data) - col_offset

        bv_col_idx = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": col_offset,
            "byteLength": col_len,
            "target": 34962
        })
        acc_col_idx = len(accessors)
        accessors.append({
            "bufferView": bv_col_idx,
            "byteOffset": 0,
            "componentType": 5126,
            "count": len(cols),
            "type": "VEC4"
        })

        # 4. Index buffer
        ind_offset = len(bin_data)
        for idx in inds:
            bin_data.extend(struct.pack('<I', idx))
        ind_len = len(bin_data) - ind_offset

        bv_ind_idx = len(buffer_views)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": ind_offset,
            "byteLength": ind_len,
            "target": 34963 # ELEMENT_ARRAY_BUFFER
        })
        acc_ind_idx = len(accessors)
        accessors.append({
            "bufferView": bv_ind_idx,
            "byteOffset": 0,
            "componentType": 5125, # UNSIGNED_INT
            "count": len(inds),
            "type": "SCALAR"
        })

        primitives.append({
            "attributes": {
                "POSITION": acc_pos_idx,
                "NORMAL": acc_norm_idx,
                "COLOR_0": acc_col_idx
            },
            "indices": acc_ind_idx,
            "material": m_idx
        })

    # Pad binary data to 4-byte boundary
    while len(bin_data) % 4 != 0:
        bin_data.append(0)

    gltf_json = {
        "asset": {
            "version": "2.0",
            "generator": "Overdrive Core Procedural GLB Exporter"
        },
        "scene": 0,
        "scenes": [{ "nodes": [0] }],
        "nodes": [{ "mesh": 0 }],
        "meshes": [{
            "primitives": primitives
        }],
        "materials": materials,
        "buffers": [{
            "byteLength": len(bin_data)
        }],
        "bufferViews": buffer_views,
        "accessors": accessors
    }

    json_bytes = json.dumps(gltf_json).encode('utf-8')
    while len(json_bytes) % 4 != 0:
        json_bytes += b' '

    total_len = 12 + 8 + len(json_bytes) + 8 + len(bin_data)

    header = struct.pack('<4sII', b'glTF', 2, total_len)
    json_chunk_header = struct.pack('<I4s', len(json_bytes), b'JSON')
    bin_chunk_header = struct.pack('<I4s', len(bin_data), b'BIN\0')

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(json_chunk_header)
        f.write(json_bytes)
        f.write(bin_chunk_header)
        f.write(bin_data)
    print(f"[EXPORT] Created GLB: {output_path} ({total_len} bytes)")

def add_box(verts, norms, cols, inds, cx, cy, cz, sx, sy, sz, color):
    base_idx = len(verts)
    hx, hy, hz = sx*0.5, sy*0.5, sz*0.5

    faces = [
        # Front (+Z)
        ([(cx-hx, cy-hy, cz+hz), (cx+hx, cy-hy, cz+hz), (cx+hx, cy+hy, cz+hz), (cx-hx, cy+hy, cz+hz)], (0, 0, 1)),
        # Back (-Z)
        ([(cx+hx, cy-hy, cz-hz), (cx-hx, cy-hy, cz-hz), (cx-hx, cy+hy, cz-hz), (cx+hx, cy+hy, cz-hz)], (0, 0, -1)),
        # Right (+X)
        ([(cx+hx, cy-hy, cz+hz), (cx+hx, cy-hy, cz-hz), (cx+hx, cy+hy, cz-hz), (cx+hx, cy+hy, cz+hz)], (1, 0, 0)),
        # Left (-X)
        ([(cx-hx, cy-hy, cz-hz), (cx-hx, cy-hy, cz+hz), (cx-hx, cy+hy, cz+hz), (cx-hx, cy+hy, cz-hz)], (-1, 0, 0)),
        # Top (+Y)
        ([(cx-hx, cy+hy, cz+hz), (cx+hx, cy+hy, cz+hz), (cx+hx, cy+hy, cz-hz), (cx-hx, cy+hy, cz-hz)], (0, 1, 0)),
        # Bottom (-Y)
        ([(cx-hx, cy-hy, cz-hz), (cx+hx, cy-hy, cz-hz), (cx+hx, cy-hy, cz+hz), (cx-hx, cy-hy, cz+hz)], (0, -1, 0)),
    ]

    for face_verts, n in faces:
        idx_start = len(verts)
        for v in face_verts:
            verts.append(v)
            norms.append(n)
            cols.append(color)
        inds.extend([idx_start, idx_start+1, idx_start+2, idx_start, idx_start+2, idx_start+3])

def generate_kinetic_rifle():
    # Multi-part tactical mech rifle: RF-024
    # Frame, barrel, cooling shroud, magazine, optical sight, muzzle brake
    verts, norms, cols, inds = [], [], [], []

    # Dark gunmetal body
    c_gunmetal = (0.22, 0.24, 0.28, 1.0)
    # White ceramic plating (AC style accent)
    c_white = (0.92, 0.94, 0.98, 1.0)
    # Orange neon emitter/sensor
    c_orange = (1.0, 0.6, 0.15, 1.0)
    # Metallic steel
    c_steel = (0.45, 0.48, 0.52, 1.0)

    # 1. Main Receiver
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.10, 0.18, 0.22, 0.70, c_white)
    # 2. Lower Receiver & Trigger Guard
    add_box(verts, norms, cols, inds, 0.0, -0.10, -0.12, 0.14, 0.10, 0.45, c_gunmetal)
    # 3. Grip & Mount Socket
    add_box(verts, norms, cols, inds, 0.0, -0.16, -0.25, 0.12, 0.16, 0.12, c_gunmetal)
    # 4. Long Tactical Barrel
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.42, 0.09, 0.09, 0.55, c_steel)
    # 5. Hexagonal Barrel Shroud & Cooling Vents
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.30, 0.15, 0.15, 0.38, c_gunmetal)
    # 6. Quad-Port Muzzle Brake
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.72, 0.13, 0.13, 0.12, c_gunmetal)
    # 7. Optical Sensor / Scope on Top
    add_box(verts, norms, cols, inds, 0.0, 0.15, -0.05, 0.11, 0.09, 0.42, c_gunmetal)
    # 8. Sensor Lens (Orange Glow)
    add_box(verts, norms, cols, inds, 0.0, 0.15, 0.165, 0.08, 0.07, 0.02, c_orange)
    # 9. Extended Box Magazine (Slanted)
    add_box(verts, norms, cols, inds, 0.0, -0.18, 0.05, 0.12, 0.22, 0.18, c_gunmetal)

    mesh = {
        'vertices': verts,
        'normals': norms,
        'colors': cols,
        'indices': inds,
        'material_color': (0.9, 0.92, 0.95, 1.0)
    }
    create_glb([mesh], "assets/models/weapons/RF-024.glb")

def generate_missile_pod():
    # 4-cell Vertical Micro-Missile Pod: ML-080
    verts, norms, cols, inds = [], [], [], []

    c_pod = (0.28, 0.30, 0.35, 1.0)
    c_accent = (0.85, 0.25, 0.20, 1.0) # Hazard Red
    c_hatch = (0.15, 0.16, 0.18, 1.0)
    c_warhead = (0.95, 0.90, 0.40, 1.0) # Yellow tips

    # 1. Main Pod Armor Box
    add_box(verts, norms, cols, inds, 0.0, 0.0, 0.0, 0.38, 0.42, 0.85, c_pod)
    # 2. Side Reinforcement Struts
    add_box(verts, norms, cols, inds, -0.20, 0.0, 0.0, 0.05, 0.32, 0.75, c_pod)
    add_box(verts, norms, cols, inds,  0.20, 0.0, 0.0, 0.05, 0.32, 0.75, c_pod)
    # 3. Hazard Stripes on Side
    add_box(verts, norms, cols, inds, -0.21, 0.08, 0.10, 0.04, 0.08, 0.35, c_accent)
    add_box(verts, norms, cols, inds,  0.21, 0.08, 0.10, 0.04, 0.08, 0.35, c_accent)
    # 4. Top 4 Silo Hatches & Missile Tips (Facing upward +Y)
    silo_coords = [(-0.09, -0.18), (0.09, -0.18), (-0.09, 0.18), (0.09, 0.18)]
    for sx, sz in silo_coords:
        # Hatch Rim
        add_box(verts, norms, cols, inds, sx, 0.215, sz, 0.14, 0.03, 0.14, c_hatch)
        # Warhead Cone Tip visible inside
        add_box(verts, norms, cols, inds, sx, 0.235, sz, 0.08, 0.05, 0.08, c_warhead)
    # 5. Rear Exhaust Vents (-Z)
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.435, 0.28, 0.28, 0.04, c_hatch)

    mesh = {
        'vertices': verts,
        'normals': norms,
        'colors': cols,
        'indices': inds,
        'material_color': (0.35, 0.38, 0.42, 1.0)
    }
    create_glb([mesh], "assets/models/weapons/ML-080.glb")

def generate_beam_rifle():
    # High-cycle Laser / Beam Rifle: EN-010
    verts, norms, cols, inds = [], [], [], []

    c_dark = (0.16, 0.18, 0.22, 1.0)
    c_cyan_glow = (0.25, 0.85, 1.0, 1.0)
    c_silver = (0.80, 0.82, 0.86, 1.0)

    # 1. Receiver
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.10, 0.16, 0.22, 0.65, c_dark)
    # 2. Twin Energy Focusing Rails (Top & Bottom)
    add_box(verts, norms, cols, inds, 0.0,  0.08, 0.35, 0.06, 0.04, 0.60, c_silver)
    add_box(verts, norms, cols, inds, 0.0, -0.08, 0.35, 0.06, 0.04, 0.60, c_silver)
    # 3. Center Laser Accelerator Core (Glowing Cyan)
    add_box(verts, norms, cols, inds, 0.0, 0.0, 0.30, 0.08, 0.08, 0.45, c_cyan_glow)
    # 4. Energy Battery Pack at Back
    add_box(verts, norms, cols, inds, 0.0, 0.02, -0.42, 0.18, 0.16, 0.20, c_cyan_glow)
    # 5. Mount interface
    add_box(verts, norms, cols, inds, 0.0, -0.14, -0.15, 0.10, 0.12, 0.14, c_dark)

    mesh = {
        'vertices': verts,
        'normals': norms,
        'colors': cols,
        'indices': inds,
        'material_color': (0.2, 0.22, 0.26, 1.0)
    }
    create_glb([mesh], "assets/models/weapons/EN-010.glb")

def generate_heavy_plasma():
    # Heavy Plasma Cannon: PL-040
    verts, norms, cols, inds = [], [], [], []
    c_plasma = (0.85, 0.20, 1.00, 1.0) # Violet glow
    c_armor = (0.32, 0.22, 0.40, 1.0)
    c_heat = (0.20, 0.20, 0.25, 1.0)

    # Bulky heavy plasma body
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.15, 0.26, 0.30, 0.75, c_armor)
    # Dual plasma induction coils
    add_box(verts, norms, cols, inds, 0.0, 0.05, 0.35, 0.22, 0.22, 0.50, c_plasma)
    # Heavy exhaust heat sinks
    add_box(verts, norms, cols, inds, 0.0, 0.16, -0.10, 0.20, 0.08, 0.40, c_heat)
    # Muzzle emitter ring
    add_box(verts, norms, cols, inds, 0.0, 0.05, 0.65, 0.24, 0.24, 0.10, c_heat)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.35, 0.2, 0.45, 1.0) }
    create_glb([mesh], "assets/models/weapons/PL-040.glb")

def generate_rapid_burst():
    # Rapid Burst Submachine Gun: MG-014
    verts, norms, cols, inds = [], [], [], []
    c_body = (0.35, 0.32, 0.30, 1.0)
    c_mag = (0.90, 0.75, 0.20, 1.0) # Gold accent
    c_barrel = (0.20, 0.20, 0.22, 1.0)

    # Compact SMG frame
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.05, 0.16, 0.20, 0.55, c_body)
    # Short dual barrels
    add_box(verts, norms, cols, inds, -0.04, 0.02, 0.35, 0.05, 0.05, 0.35, c_barrel)
    add_box(verts, norms, cols, inds,  0.04, 0.02, 0.35, 0.05, 0.05, 0.35, c_barrel)
    # High-capacity helical drum magazine
    add_box(verts, norms, cols, inds, 0.0, -0.14, 0.05, 0.18, 0.14, 0.24, c_mag)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.38, 0.35, 0.32, 1.0) }
    create_glb([mesh], "assets/models/weapons/MG-014.glb")

def generate_heavy_bazooka():
    # Titan Bazooka: BZ-033
    verts, norms, cols, inds = [], [], [], []
    c_barrel = (0.24, 0.25, 0.27, 1.0)
    c_warn = (1.00, 0.38, 0.08, 1.0) # Hazard Orange
    c_breech = (0.18, 0.19, 0.20, 1.0)

    # Massive long cylindrical cannon barrel
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.25, 0.28, 0.28, 1.40, c_barrel)
    # Heavy rear breech & counterweight
    add_box(verts, norms, cols, inds, 0.0, 0.02, -0.65, 0.32, 0.34, 0.45, c_breech)
    # Reinforced blast shield / thermal collar
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.45, 0.32, 0.32, 0.25, c_warn)
    # Massive flared muzzle brake
    add_box(verts, norms, cols, inds, 0.0, 0.02, 0.98, 0.34, 0.34, 0.12, c_barrel)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.22, 0.24, 0.26, 1.0) }
    create_glb([mesh], "assets/models/weapons/BZ-033.glb")

def generate_laser_blade():
    # Moonlight Laser Blade: LB-077
    verts, norms, cols, inds = [], [], [], []
    c_hilt = (0.15, 0.25, 0.22, 1.0)
    c_emitter = (0.20, 0.95, 0.65, 1.0) # Moonlight emerald green
    c_silver = (0.85, 0.88, 0.90, 1.0)

    # Forearm gauntlet mount & hilt
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.10, 0.18, 0.20, 0.45, c_hilt)
    # Plasma emitter housing
    add_box(verts, norms, cols, inds, 0.0, 0.0, 0.22, 0.14, 0.16, 0.22, c_silver)
    # Emitter nozzle slit
    add_box(verts, norms, cols, inds, 0.0, 0.0, 0.34, 0.08, 0.12, 0.05, c_emitter)
    # Folded energy channel guide
    add_box(verts, norms, cols, inds, 0.0, 0.11, 0.05, 0.10, 0.06, 0.40, c_emitter)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.15, 0.25, 0.22, 1.0) }
    create_glb([mesh], "assets/models/weapons/LB-077.glb")

def generate_vulcan_gatling():
    # 6-Barrel Vulcan Gatling: GT-090
    verts, norms, cols, inds = [], [], [], []
    c_body = (0.22, 0.24, 0.26, 1.0)
    c_barrel = (0.35, 0.36, 0.38, 1.0)
    c_motor = (0.85, 0.60, 0.15, 1.0)

    # Motor housing & receiver
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.20, 0.28, 0.30, 0.55, c_body)
    # Rotary motor assembly
    add_box(verts, norms, cols, inds, 0.0, 0.0, 0.15, 0.26, 0.26, 0.18, c_motor)
    # 6-barrel rotary cluster (approximated by circular layout of long barrel tubes)
    for i in range(6):
        ang = i * (2.0 * math.pi / 6.0)
        bx = math.cos(ang) * 0.08
        by = math.sin(ang) * 0.08
        add_box(verts, norms, cols, inds, bx, by, 0.65, 0.04, 0.04, 0.85, c_barrel)
    # Support retaining ring at muzzle
    add_box(verts, norms, cols, inds, 0.0, 0.0, 1.05, 0.22, 0.22, 0.06, c_body)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.25, 0.26, 0.28, 1.0) }
    create_glb([mesh], "assets/models/weapons/GT-090.glb")

def generate_breaker_shotgun():
    # Breaker Shotgun: SG-020
    verts, norms, cols, inds = [], [], [], []
    c_body = (0.30, 0.28, 0.26, 1.0)
    c_mag = (0.90, 0.45, 0.15, 1.0) # Blaze orange
    c_barrel = (0.42, 0.44, 0.46, 1.0)

    # Receiver
    add_box(verts, norms, cols, inds, 0.0, 0.0, -0.10, 0.20, 0.26, 0.60, c_body)
    # Twin wide-bore shotgun barrels
    add_box(verts, norms, cols, inds, -0.06, 0.04, 0.38, 0.09, 0.09, 0.50, c_barrel)
    add_box(verts, norms, cols, inds,  0.06, 0.04, 0.38, 0.09, 0.09, 0.50, c_barrel)
    # Under-barrel tube magazine
    add_box(verts, norms, cols, inds, 0.0, -0.08, 0.25, 0.14, 0.10, 0.45, c_mag)
    # Heavy heat shield on top
    add_box(verts, norms, cols, inds, 0.0, 0.11, 0.15, 0.18, 0.05, 0.40, c_body)

    mesh = { 'vertices': verts, 'normals': norms, 'colors': cols, 'indices': inds, 'material_color': (0.30, 0.28, 0.26, 1.0) }
    create_glb([mesh], "assets/models/weapons/SG-020.glb")

if __name__ == '__main__':
    generate_kinetic_rifle()
    generate_missile_pod()
    generate_beam_rifle()
    generate_heavy_plasma()
    generate_rapid_burst()
    generate_heavy_bazooka()
    generate_laser_blade()
    generate_vulcan_gatling()
    generate_breaker_shotgun()
    print("All 9 GLB weapon models generated successfully.")

