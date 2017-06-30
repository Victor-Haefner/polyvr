import bpy
from bpy import context

N = 0

verts = []

for obj in context.scene.objects:
    if obj.type == 'MESH':
        m = obj.matrix_world
        vs = obj.data.vertices
        N += len(vs)
        
        for v in vs:
            verts.append(m * v.co)

path = 'pointcloud.ply'
file = open(path, 'w')
file.write('ply\n')
file.write('format ascii 1.0\n')
file.write('element vertex '+str(N)+'\n')
file.write('property float x\n')
file.write('property float y\n')
file.write('property float z\n')
file.write('element face 0\n')
file.write('property list uchar uint vertex_indices\n')
file.write('end_header\n')
for v in verts:
    file.write(str(v[0])+' '+str(v[1])+' '+str(v[2])+'\n')
file.close()
