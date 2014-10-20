#ifndef VRPYTYPECASTER_H_INCLUDED
#define VRPYTYPECASTER_H_INCLUDED

#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>

using namespace std;

namespace OSG{ class VRObject; }

class VRPyTypeCaster {
    public:
        VRPyTypeCaster();
        static PyObject* err;

        static PyObject* cast(OSG::VRObject* obj);
};

#endif // VRPYTYPECASTER_H_INCLUDED
