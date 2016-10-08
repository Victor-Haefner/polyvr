#ifndef VRPYTYPECASTER_H_INCLUDED
#define VRPYTYPECASTER_H_INCLUDED

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"

using namespace std;

class VRPyTypeCaster {
    public:
        VRPyTypeCaster();
        static PyObject* err;

        static PyObject* cast(OSG::VRObjectPtr obj);
        static PyObject* cast(OSG::VRDevicePtr dev);
};

#endif // VRPYTYPECASTER_H_INCLUDED
