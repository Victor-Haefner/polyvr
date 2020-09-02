#include "VRPyPath.h"
#include "VRPyBaseT.h"
#include "VRPyPose.h"
#include "VRPyMath.h"

using namespace OSG;

simplePyType( Path, New_ptr);

PyMethodDef VRPyPath::methods[] = {
    {"set", PyWrap2(Path, set, "Set the path - set(start, end, resolution)", void, PosePtr, PosePtr, int ) },
    {"invert", PyWrap2(Path, invert, "Invert start && end point of path", void ) },
    {"compute", PyWrap2(Path, compute, "Compute path with given resolution, allways call this after adding all path points - compute( int resolution )", void, int ) },
    {"update", PyWrap2(Path, update, "Update path", void ) },
    {"addPoint", PyWrapOpt2(Path, addPoint2, "Add a point to the path ( | vec3 pos, vec3 dir, vec3 col, vec3 up)", "0 0 0|0 0 -1|0 0 0|0 1 0", int, Vec3d, Vec3d, Color3f, Vec3d ) },
    {"addPoint2", PyWrapOpt2(Path, addPoint, "Add a point to the path ( Pose pose | vec3 color)", "1 1 1", int, Pose p, Color3f ) },
    {"getPose", PyWrapOpt2(Path, getPose, "Return the pose at the path length t, t on the interval between point i and j - pose getPose( float t | int i, int j)", "0|0|1", PosePtr, float, int, int, bool ) },
    {"getPoints", PyWrap2(Path, getPoints, "Return a list of the path points", vector<Pose> ) },
    {"getControlPoints", PyWrap2(Path, getControlPoints, "Return a list of the path control points", vector<Vec3d> ) },
    {"close", PyWrap2(Path, close, "Close the path - close()", void ) },
    {"getPositions", PyWrap2(Path, getPositions, "Return the positions from the computed path - [[x,y,z]] getPositions()", vector<Vec3d> ) },
    {"getDirections", PyWrap2(Path, getDirections, "Return the directions from the computed path - [[x,y,z]] getDirections()", vector<Vec3d> ) },
    {"getUpVectors", PyWrap2(Path, getUpVectors, "Return the up vectors from the computed path - [[x,y,z]] getUpVectors()", vector<Vec3d> ) },
    {"getColors", PyWrap2(Path, getColors, "Return the colors from the computed path - [[x,y,z]] getColors()", vector<Vec3d> ) },
    {"getSize", PyWrap2(Path, size, "Return the number of path nodes - int getSize()", int ) },
    {"getLength", PyWrapOpt2(Path, getLength, "Return the approximated path length - float getLength( | int i, int j )", "0|0", float, int, int ) },
    {"getDistance", PyWrap2(Path, getDistance, "Return the distance from point to path - float getDistance( [x,y,z] )", float, Vec3d ) },
    {"getDistanceToHull", PyWrap2(Path, getDistanceToHull, "Return the distance from point to path hull - float getDistanceToHull( [x,y,z] )", float, Vec3d ) },
    {"getClosestPoint", PyWrap2(Path, getClosestPoint, "Return the closest point on path in path coordinate t - float getClosestPoint( [x,y,z] ) Return value from 0 (path start) to 1 (path end)", float, Vec3d ) },
    {"approximate", PyWrap2(Path, approximate, "Convert the cubic bezier spline in a quadratic or linear one (currently only quadratic) - approximate(int degree)", void, int ) },
    {"isStraight", PyWrapOpt2(Path, isStraight, "Check if the path is straight between point i and j - bool isStraight( | int i, int j )", "0|0", bool, int, int ) },
    {"isCurve", PyWrapOpt2(Path, isCurve, "Check if the path is curved between point i and j - bool isCurve( | int i, int j )", "0|0", bool, int, int ) },
    {"isSinuous", PyWrapOpt2(Path, isSinuous, "Check if the path is sinuous between point i and j - bool isSinuous( | int i, int j )", "0|0", bool, int, int ) },
    {"translate", PyWrap2(Path, translate, "Move path points", void, Vec3d ) },
    {NULL}  /* Sentinel */
};



