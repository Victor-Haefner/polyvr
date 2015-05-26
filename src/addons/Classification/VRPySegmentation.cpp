#include "VRPySegmentation.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRSegmentation>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Segmentation",             /*tp_name*/
    sizeof(VRPySegmentation),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRSegmentation binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPySegmentation::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMethodDef VRPySegmentation::methods[] = {
    {"extractPatches", (PyCFunction)VRPySegmentation::extractPatches, METH_VARARGS, "Extract patches from the geometry" },
    {"fillHoles", (PyCFunction)VRPySegmentation::fillHoles, METH_VARARGS, "Fill the holes of a geometry" },
    {"convexDecompose", (PyCFunction)VRPySegmentation::convexDecompose, METH_VARARGS, "Decompose the geometry in convex parts" },
    {"removeDuplicates", (PyCFunction)VRPySegmentation::removeDuplicates, METH_VARARGS, "Make all vertices single index and remove the duplicates" },
    {NULL}  /* Sentinel */
};

PyObject* VRPySegmentation::convexDecompose(VRPySegmentation* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    if (geo == NULL) { PyErr_SetString(err, "VRPySegmentation::convexDecompose: Missing geometry parameter"); return NULL; }
    OSG::VRObject* objects = self->obj->convexDecompose(geo->obj);
    return VRPyTypeCaster::cast( objects );
}

PyObject* VRPySegmentation::fillHoles(VRPySegmentation* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    if (geo == NULL) { PyErr_SetString(err, "VRPySegmentation::fillHoles: Missing geometry parameter"); return NULL; }
    self->obj->fillHoles(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPySegmentation::removeDuplicates(VRPySegmentation* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    if (! PyArg_ParseTuple(args, "O", &geo)) return NULL;
    if (geo == NULL) { PyErr_SetString(err, "VRPySegmentation::removeDuplicates: Missing geometry parameter"); return NULL; }
    self->obj->removeDuplicates(geo->obj);
    Py_RETURN_TRUE;
}

PyObject* VRPySegmentation::extractPatches(VRPySegmentation* self, PyObject* args) {
    VRPyGeometry* geo = NULL;
    int algo = 0;
    float curv, curv_d;
    PyObject *norm, *norm_d;
    if (! PyArg_ParseTuple(args, "OiffOO", &geo, &algo, &curv, &curv_d, &norm, &norm_d)) return NULL;
    if (geo == NULL) { PyErr_SetString(err, "VRPySegmentation::extractPatches: Missing geometry parameter"); return NULL; }

    OSG::Vec3f vnorm = parseVec3fList(norm);
    OSG::Vec3f vnorm_d = parseVec3fList(norm_d);

    OSG::VRObject* patches = self->obj->extractPatches(geo->obj, OSG::SEGMENTATION_ALGORITHM(algo), curv, curv_d, vnorm, vnorm_d);
    return VRPyTypeCaster::cast( patches );
}
