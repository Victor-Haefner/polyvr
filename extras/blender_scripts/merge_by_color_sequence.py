import bpy

colp = None
col = None
_o = None
for o in bpy.context.scene.objects:
    colp = col
    col = o.active_material.diffuse_color

    if col[0] == colp[0] and col[1] == colp[1] and col[2] == colp[2]:
        bpy.ops.object.select_all(action='DESELECT')
        o.select = True
        _o.select = True
        bpy.context.scene.objects.active = o
        bpy.ops.object.join()
        o = bpy.context.scene.objects.active
        while len(o.data.materials) > 1:
            o.data.materials.pop(1, update_data=True)

    _o = o
