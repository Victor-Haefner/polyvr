#ifndef VRPYNUMBERINGENGINE_H_INCLUDED
#define VRPYNUMBERINGENGINE_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRNumberingEngine.h"

struct VRPyNumberingEngine : VRPyBaseT<VRNumberingEngine> {
    static PyMethodDef methods[];

    static PyObject* add(VRPyNumberingEngine* self, PyObject* args);
    static PyObject* set(VRPyNumberingEngine* self, PyObject* args);
    static PyObject* clear(VRPyNumberingEngine* self);
};

#endif // VRPYNUMBERINGENGINE_H_INCLUDED


/*

ne = VR.NumerEngine()
root.addChild(ne)

N = 10
ne.add(N)

for i in range(N):
    ne.set(i, [i,0,0], i) # ID, position, value

*/
