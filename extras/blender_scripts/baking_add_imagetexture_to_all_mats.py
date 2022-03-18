import bpy

for mat in bpy.data.materials:
    if not mat.node_tree: 
        continue
    g = mat.node_tree.nodes.new(type='ShaderNodeTexImage')
    g.image = bpy.data.images.get('LeonardoBakeImg', None)
    g.select = True
    mat.node_tree.nodes.active = g
