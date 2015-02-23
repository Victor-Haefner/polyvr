#-----------------------------------------------------------------------------
#remove duplicates v1.3
#best way to remove duplicates, just select the objects you want the duplicates removed, then run this scrpit
import bpy 

for obj in bpy.context.selected_objects:
    if obj.type == 'MESH':
        bpy.data.scenes[0].objects.active = obj # make obj active to do operations on it
        bpy.ops.object.mode_set(mode='OBJECT', toggle=False)    # set 3D View to Object Mode (probably redundant)
        bpy.ops.object.mode_set(mode='EDIT', toggle=False)      # set 3D View to Edit Mode
        bpy.context.tool_settings.mesh_select_mode = [False, False, True]   # set to face select in 3D View Editor
        bpy.ops.mesh.select_all(action='SELECT')    # make sure all faces in mesh are selected
        bpy.ops.object.mode_set(mode='OBJECT', toggle=False)    # very silly, you have to be in object mode to select faces!!
        
        found = set([])              # set of found sorted vertices pairs
 
        for face in obj.data.polygons:
            facevertsorted = sorted(face.vertices[:])           # sort vertices of the face to compare later
            if str(facevertsorted) not in found:                # if sorted vertices are not in the set
                found.add(str(facevertsorted))                  # add them in the set
                obj.data.polygons[face.index].select = False    # deselect faces i want to keep
 
        bpy.ops.object.mode_set(mode='EDIT', toggle=False)      # set to Edit Mode AGAIN
        bpy.ops.mesh.delete(type='FACE')                        # delete double faces
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.normals_make_consistent(inside=False)      # recalculate normals
        bpy.ops.mesh.remove_doubles(threshold=0.0001, use_unselected=False) #remove doubles
        bpy.ops.mesh.normals_make_consistent(inside=False)      # recalculate normals (this one or two lines above is redundant)
        bpy.ops.object.mode_set(mode='OBJECT', toggle=False)    # set to Object Mode AGAIN
