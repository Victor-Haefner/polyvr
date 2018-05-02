#include "VRPyNature.h"
#include "core/scripting/VRPyMaterial.h"
#include "core/scripting/VRPyImage.h"
#include "core/scripting/VRPyPolygon.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Tree, New_VRObjects_ptr);
simpleVRPyType(Nature, New_VRObjects_ptr);

PyMethodDef VRPyTree::methods[] = {
    {"setup", PyWrap( Tree, setup, "Quick setup", void, int, int, int, Vec4d, Vec4d ) },
    {"addBranching", PyWrap( Tree, addBranching, "Add branching layer, int nodes = 1, int branching = 5, float[n_angle = 0.2, p_angle = 0.6, length = 0.8, radius = 0.1], float[n_angle_v = 0.2, p_angle_v = 0.4, length_v = 0.2, radius_v = 0.2]", void, int, int, Vec4d, Vec4d) },
    {"grow", PyWrapOpt( Tree, grow, "Set the tree parameters, int seed", "0", void, int ) },
    {"addLeafs", PyWrapOpt( Tree, addLeafs, "Add a leaf layer, int lvl, int amount, flt size", "0.03", void, int, int, float) },
    {"setLeafMaterial", PyWrap( Tree, setLeafMaterial, "Set custom leaf material", void, VRMaterialPtr ) },
    {"createLOD", PyWrap( Tree, createLOD, "Create an LOD from the tree, int number of branching layers to keep", VRGeometryPtr, int ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyNature::methods[] = {
    {"addTree", PyWrapOpt(Nature, addTree, "Add a copy of the passed tree to the woods and return the copy", "0|1", VRTreePtr, VRTreePtr, bool, bool ) },
    {"addGrassPatch", PyWrapOpt(Nature, addGrassPatch, "Add a grass patch from polygon", "0|0|0", void, VRPolygonPtr, bool, bool) },
    {"computeLODs", PyWrapOpt(Nature, computeAllLODs, "Compute LODs", "0", void, bool ) },
    {"addCollisionModels", PyWrap(Nature, addCollisionModels, "Add collision box to trees and bushes - addCollisionModels() ", void ) },
    {"clear", PyWrap(Nature, clear, "Clear woods", void ) },
    {"getTree", PyWrap(Nature, getTree, "Get a tree by id", VRTreePtr, int ) },
    {"removeTree", PyWrap(Nature, removeTree, "Remove a tree by id", void, int ) },
    {"simpleInit", PyWrap(Nature, simpleInit, "Add a few random tree and bush types", void, int, int) },
    {"createRandomTree", PyWrap(Nature, createRandomTree, "create a random tree", VRTreePtr, Vec3d) },
    {NULL}  /* Sentinel */
};



