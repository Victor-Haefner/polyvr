#ifndef VRPYIMAGE_H_INCLUDED
#define VRPYIMAGE_H_INCLUDED

#include "VRPyBase.h"
#include <OpenSG/OSGImage.h>

struct VRPyImage : VRPyBaseT<OSG::Image> {
    static PyMethodDef methods[];

    OSG::ImageRecPtr img;
    int internal_format = -1;
    static PyObject* New(PyTypeObject *type, PyObject *args, PyObject *kwds);
    static void dealloc(VRPyImage* self);
};

#endif // VRPYIMAGE_H_INCLUDED
