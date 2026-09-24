import bpy
import bmesh
import os

from mathutils import Vector

def importTS(filepath, offset, scale):
    def parse_vector3(data, i1, i2, i3):
        return Vector(( float(data[i1]) + offset[0], float(data[i2]) + offset[1], -float(data[i3]) + offset[2])) * scale # Swapped Z and Y
          
    verts_dict = {}
    tris = []
    current_name = "mesh"
    meshes = []
    geo_positions = {}
    
    def getMaterial():
        if "tsMat" not in bpy.data.materials:
            mat = bpy.data.materials.new(name="tsMat")
            mat.use_nodes = True
            bsdf = mat.node_tree.nodes.get("Principled BSDF")
            if bsdf:
                bsdf.inputs["Base Color"].default_value = (0.8, 0.8, 0.6, 1)
                #bsdf.inputs["Specular"].default_value = 0.1
                bsdf.inputs["Roughness"].default_value = 0.9
        else:
            mat = bpy.data.materials["tsMat"]
        return mat

    def makeGeometry(name, verts, faces):
        if not verts or not faces: return

        mesh = bpy.data.meshes.new(name)
        obj = bpy.data.objects.new(name, mesh)

        bpy.context.collection.objects.link(obj)
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        mat = getMaterial()
        obj.data.materials.append(mat)
        
    def pushGeometry(geo_positions, tris):
        sorted_keys = sorted(geo_positions.keys())
        key_to_index = {k: i for i, k in enumerate(sorted_keys)}
        verts = [geo_positions[k] for k in sorted_keys]
        faces = [(key_to_index[a], key_to_index[b], key_to_index[c]) for (a, b, c) in tris]
        makeGeometry(current_name, verts, faces)
        geo_positions.clear()
        tris.clear()

    with open(filepath, 'r') as f:
        for line in f:
            tokens = line.strip().split()
            if not tokens:
                continue

            cmd = tokens[0]

            if cmd == "NAME":
                if '"' in line:
                    parts = line.split('"')
                    if len(parts) > 1:
                        current_name = parts[1]

            elif cmd in ("VRTX", "PVRTX"):
                idx = int(tokens[1])
                pos = parse_vector3(tokens, 2, 3, 4)
                geo_positions[idx] = pos

            elif cmd in ("ATOM", "PATOM"):
                idx = int(tokens[1])
                ref = int(tokens[2])
                if ref in geo_positions:
                    geo_positions[idx] = geo_positions[ref]

            elif cmd == "TRGL":
                i1, i2, i3 = map(int, tokens[1:4])
                tris.append((i1, i2, i3))

            elif cmd == "END":
                if geo_positions: pushGeometry(geo_positions, tris)

        # final push
        if geo_positions: pushGeometry(geo_positions, tris)


# TODO: blender file selector
path = bpy.path.abspath( "//09TopFS.ts" )
offset = (3457617.55033924, 5439522.54827968, -1196.73616469629)
scale = 0.001
importTS(path, offset, scale)

