#include "VRPyBaseFactory.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"

using namespace OSG;

// Stuff for proxy setter

template<> bool parseValue<float>(PyObject* args, float& t) {
    if (!PyArg_ParseTuple(args, "f", &t)) return false;
    return true;
}

template<> bool parseValue<VREntityPtr>(PyObject* args, VREntityPtr& t) {
    VRPyEntity* e = 0;
    if (!PyArg_ParseTuple(args, "O", &e)) return false;
    if (!e) return false;
    t = e->objPtr;
    return true;
}

template<> bool parseValue<string>(PyObject* args, string& t) {
    const char* c = 0;
    if (!PyArg_ParseTuple(args, "s", &c)) return false;
    if (c == 0) return false;
    t = string(c);
    return true;
}



// Stuff for proxy getter

template<> PyObject* toPyObject<VREntityPtr>(VREntityPtr e) { return VRPyEntity::fromSharedPtr(e); }
template<> PyObject* toPyObject<int>(int i) { return PyInt_FromLong(i); }
template<> PyObject* toPyObject<string>(string s) { return PyString_FromString(s.c_str()); }




