import bpy
data = [ (o.name,len(o.data.vertices)) for o in bpy.context.scene.objects if o.type == 'MESH' ] # get data
data.sort(key=lambda tup: tup[1]) # sort by N verts
print("\n".join(map(str, data))) # print with each element on a line







