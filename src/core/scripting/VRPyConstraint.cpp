#include "VRPyConstraint.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Constraint, New_ptr);

template<> bool toValue(PyObject* o, VRConstraint::TCMode& v) {
    if (!PyUnicode_Check(o)) return 0;
    string s = PyUnicode_AsUTF8(o);
    if (s == "POINT") { v = VRConstraint::POINT; return 1; }
    if (s == "LINE") { v = VRConstraint::LINE; return 1; }
    if (s == "PLANE") { v = VRConstraint::PLANE; return 1; }
    return 0;
}

PyMethodDef VRPyConstraint::methods[] = {
    {"constrain", PyWrap(Constraint, setMinMax, "Set the constraint range of one of the six degrees of freedom", void, int, float, float ) },
    {"setLocal", PyWrap(Constraint, setLocal, "Set the local flag of the constraints", void, bool ) },
    {"lock", PyWrapOpt(Constraint, lock, "Lock a list of DoFs", "0", void, vector<int>, float ) },
    {"free", PyWrap(Constraint, free, "Free a list of DoFs", void, vector<int> ) },
    {"lockRotation", PyWrapOpt(Constraint, lockRotation, "Lock the rotation", "1", void, bool ) },
    {"lockTranslation", PyWrapOpt(Constraint, lockTranslation, "Lock the translation", "1", void, bool ) },
    {"isLocked", PyWrap(Constraint, isLocked, "Return if DoF is locked", bool, int ) },
    {"setMin", PyWrap(Constraint, setMin, "Set min value of DoF", void, int, float ) },
    {"setMax", PyWrap(Constraint, setMax, "Set max value of DoF", void, int, float ) },
    {"getMin", PyWrap(Constraint, getMin, "Get min value of DoF", float, int ) },
    {"getMax", PyWrap(Constraint, getMax, "Get max value of DoF", float, int ) },
    {"setReference", PyWrap(Constraint, setReferenceA, "Set the reference matrix, setReference( transform )", void, PosePtr ) },
    {"setReferential", PyWrap(Constraint, setReferential, "Set the local referential, setReferential( transform )", void, VRTransformPtr ) },
    {"setOffsetA", PyWrap(Constraint, setReferenceA, "Set the offset in A", void, PosePtr ) },
    {"setOffsetB", PyWrap(Constraint, setReferenceB, "Set the offset in B", void, PosePtr ) },
    {"setRotationConstraint", PyWrapOpt(Constraint, setRConstraint, "Set a rotation constraint, [axis/normal], mode, bool global\n\tmode can be 'POINT', 'LINE', 'PLANE'", "0", void, Vec3d, VRConstraint::TCMode, bool ) },
    {"setTranslationConstraint", PyWrapOpt(Constraint, setTConstraint, "Set a translation constraint, [axis/normal], mode, bool global\n\tmode can be 'POINT', 'LINE', 'PLANE'", "0", void, Vec3d, VRConstraint::TCMode, bool ) },
    {"getReferenceA", PyWrap(Constraint, getReferenceA, "Get reference in A", PosePtr ) },
    {"getReferenceB", PyWrap(Constraint, getReferenceB, "Get reference in B", PosePtr ) },
    {"setActive", PyWrap(Constraint, setActive, "Set active or not", void, bool ) },
    {"isActive", PyWrap(Constraint, isActive, "Get if active or not", bool ) },
    {NULL}  /* Sentinel */
};








