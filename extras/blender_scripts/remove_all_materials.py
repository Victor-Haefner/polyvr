import bpy
for o in bpy.context.scene.objects:
    while len(o.data.materials) > 1:
        o.data.materials.pop(1, update_data=True)
