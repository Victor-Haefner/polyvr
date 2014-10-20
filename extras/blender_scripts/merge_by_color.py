import bpy
cols = {}
for o in bpy.context.scene.objects:
    c = o.active_material.diffuse_color
    if not c[0] in cols:
        cols[c[0]] = {}
    if not c[1] in cols[c[0]]:
        cols[c[0]][c[1]] = {}
    if not c[2] in cols[c[0]][c[1]]:
        cols[c[0]][c[1]][c[2]] = []
        
    cols[c[0]][c[1]][c[2]].append(o)
    
for r in cols:
    for g in cols[r]:
        for b in cols[r][g]:
            bpy.ops.object.select_all(action='DESELECT')
            lo = cols[r][g][b]
            if len(lo) <= 1:
                continue
            bpy.context.scene.objects.active = lo[0]
            for o in lo:
                o.select = True
            bpy.ops.object.join()
            o = bpy.context.scene.objects.active
            while len(o.data.materials) > 1:
                o.data.materials.pop(1, update_data=True)

