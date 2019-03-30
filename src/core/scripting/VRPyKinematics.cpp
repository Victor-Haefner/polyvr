#include "VRPyKinematics.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/objects/VRTransform.h"
#include "VRPyPose.h"
#include "VRPyGraph.h"
#include "VRPyMath.h"

using namespace OSG;
simpleVRPyType(Kinematics, New_ptr);

PyMethodDef VRPyKinematics::methods[] = {
    {"addJoint", PyWrap( Kinematics, addJoint, "Set graph - setGraph(graph)", int, int, int, VRConstraintPtr ) },
    {"addBody", PyWrap( Kinematics, addBody, "Set graph - setGraph(graph)4", int, VRTransformPtr) },
    {"setupHinge", PyWrap( Kinematics, setupHinge, "Set graph - setGraph(graph)1", int, int, int, PosePtr, PosePtr, int, float, float) },
    {"setupBallJoint", PyWrap( Kinematics, setupBallJoint, "Set graph - setGraph(graph)2", int, int, int, PosePtr, PosePtr) },
    {"setupFixedJoint", PyWrap( Kinematics, setupFixedJoint, "Set graph - setGraph(graph)2", int, int, int, PosePtr, PosePtr) },
    {"setupCustomJoint", PyWrap( Kinematics, setupCustomJoint, "Set graph - setGraph(graph)3", int, int, int, PosePtr, PosePtr, vector<int>, vector<float>, vector<float>) },
    {"getGraph", PyWrap( Kinematics, getGraph, "Set graph - setGraph(graph)4", GraphPtr) },


    {NULL}  /* Sentinel */
};
