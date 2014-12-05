#ifndef VRPYMOLECULE_H_INCLUDED
#define VRPYMOLECULE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRMolecule.h"

struct VRPyMolecule : VRPyBaseT<OSG::VRMolecule> {
    static PyMethodDef methods[];

    static PyObject* set(VRPyMolecule* self, PyObject* args);
    static PyObject* setRandom(VRPyMolecule* self, PyObject* args);
    static PyObject* showLabels(VRPyMolecule* self, PyObject* args);
    static PyObject* showCoords(VRPyMolecule* self, PyObject* args);
    static PyObject* substitute(VRPyMolecule* self, PyObject* args);
    static PyObject* attachMolecule(VRPyMolecule* self, PyObject* args);
    static PyObject* rotateBond(VRPyMolecule* self, PyObject* args);
    static PyObject* changeBond(VRPyMolecule* self, PyObject* args);
    static PyObject* getAtomPosition(VRPyMolecule* self, PyObject* args);
    static PyObject* remAtom(VRPyMolecule* self, PyObject* args);
};

#endif // VRPYMOLECULE_H_INCLUDED
