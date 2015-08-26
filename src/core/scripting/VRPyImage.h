#ifndef VRPYIMAGE_H_INCLUDED
#define VRPYIMAGE_H_INCLUDED

#include "VRPyBase.h"
#include <OpenSG/OSGImage.h>

struct VRPyImage : VRPyBaseT<OSG::Image> {
    static PyMethodDef methods[];

    static PyObject* NewImg(PyTypeObject *type, PyObject *args, PyObject *kwds);
};

#endif // VRPYIMAGE_H_INCLUDED
