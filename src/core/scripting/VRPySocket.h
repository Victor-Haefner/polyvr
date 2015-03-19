#ifndef VRPYSOCKET_H_INCLUDED
#define VRPYSOCKET_H_INCLUDED

#include "VRPyBase.h"
#include "../networking/VRSocket.h"

struct VRPySocket : VRPyBaseT<OSG::VRSocket> {
    static PyMethodDef methods[];

    static PyObject* getName(VRPySocket* self);
    static PyObject* destroy(VRPySocket* self);
    static PyObject* send(VRPySocket* self, PyObject* args);
    static PyObject* ping(VRPySocket* self, PyObject* args);
};

#endif // VRPYSOCKET_H_INCLUDED
