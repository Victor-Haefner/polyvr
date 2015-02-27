#include "VRPyMaterial.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

template<> PyTypeObject VRPyBaseT<OSG::VRMaterial>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Material",             /*tp_name*/
    sizeof(VRPyMaterial),             /*tp_basicsize*/
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
    "VRMaterial binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyMaterial::methods, /* tp_methods */
    0, /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,        /* tp_init */
    0,                     /* tp_alloc */
    New_VRObjects,       /* tp_new */
};

PyMethodDef VRPyMaterial::methods[] = {
    {"getAmbient", (PyCFunction)VRPyMaterial::getAmbient, METH_NOARGS, "Returns the ambient color" },
    {"setAmbient", (PyCFunction)VRPyMaterial::setAmbient, METH_VARARGS, "Sets the ambient color" },
    {"getDiffuse", (PyCFunction)VRPyMaterial::getDiffuse, METH_NOARGS, "Returns the diffuse color" },
    {"setDiffuse", (PyCFunction)VRPyMaterial::setDiffuse, METH_VARARGS, "Sets the diffuse color" },
    {"getSpecular", (PyCFunction)VRPyMaterial::getSpecular, METH_NOARGS, "Returns the specular color" },
    {"setSpecular", (PyCFunction)VRPyMaterial::setSpecular, METH_VARARGS, "Sets the specular color" },
    {"getTransparency", (PyCFunction)VRPyMaterial::getTransparency, METH_NOARGS, "Returns the transparency" },
    {"setTransparency", (PyCFunction)VRPyMaterial::setTransparency, METH_VARARGS, "Sets the transparency" },
    {"getShininess", (PyCFunction)VRPyMaterial::getShininess, METH_NOARGS, "Returns the shininess" },
    {"setShininess", (PyCFunction)VRPyMaterial::setShininess, METH_VARARGS, "Sets the shininess" },
    {"setPointSize", (PyCFunction)VRPyMaterial::setPointSize, METH_VARARGS, "Sets the GL point size" },
    {"setLineWidth", (PyCFunction)VRPyMaterial::setLineWidth, METH_VARARGS, "Sets the GL line width" },
    {"setPerlin", (PyCFunction)VRPyMaterial::setPerlin, METH_VARARGS, "Set a perlin noise texture - setPerlin(col1[r,g,b], col2[r,g,b], seed)" },
    {"setQRCode", (PyCFunction)VRPyMaterial::setQRCode, METH_VARARGS, "Encode a string as QR code texture - setQRCode(string, fg[r,g,b], bg[r,g,b], offset)" },
    {"setMagMinFilter", (PyCFunction)VRPyMaterial::setMagMinFilter, METH_VARARGS, "Set the mag && min filtering mode - setMagMinFilter( mag, min)\n possible values for mag are GL_X && min can be GL_X or GL_X_MIPMAP_Y, where X && Y can be NEAREST or LINEAR" },
    {"setVertexProgram", (PyCFunction)VRPyMaterial::setVertexProgram, METH_VARARGS, "Set vertex program - setVertexProgram( myScript )" },
    {"setFragmentProgram", (PyCFunction)VRPyMaterial::setFragmentProgram, METH_VARARGS, "Set fragment program - setFragmentProgram( myScript )" },
    {"setGeometryProgram", (PyCFunction)VRPyMaterial::setGeometryProgram, METH_VARARGS, "Set geometry program - setGeometryProgram( myScript )" },
    {"setWireFrame", (PyCFunction)VRPyMaterial::setWireFrame, METH_VARARGS, "Set wireframe mode" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMaterial::setWireFrame(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setWireFrame, C obj is invalid"); return NULL; }
	self->obj->setWireFrame(parseBool(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setMagMinFilter(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setMagMinFilter, C obj is invalid"); return NULL; }
	PyObject *mag, *min;
    if (! PyArg_ParseTuple(args, "OO", &mag, &min)) return NULL;
	self->obj->setMagMinFilter(PyString_AsString(mag), PyString_AsString(min));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setVertexProgram(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setVertexProgram, C obj is invalid"); return NULL; }
	self->obj->setVertexScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setFragmentProgram(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setFragmentProgram, C obj is invalid"); return NULL; }
	self->obj->setFragmentScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setGeometryProgram(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setGeometryProgram, C obj is invalid"); return NULL; }
	self->obj->setGeometryScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getAmbient(VRPyMaterial* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::getAmbient, C obj is invalid"); return NULL; }
    return toPyTuple(self->obj->getAmbient());
}

PyObject* VRPyMaterial::setAmbient(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setAmbient, C obj is invalid"); return NULL; }
	self->obj->setAmbient(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getDiffuse(VRPyMaterial* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::getDiffuse, C obj is invalid"); return NULL; }
    return toPyTuple(self->obj->getDiffuse());
}

PyObject* VRPyMaterial::setDiffuse(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setDiffuse, C obj is invalid"); return NULL; }
	self->obj->setDiffuse(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getSpecular(VRPyMaterial* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::getSpecular, C obj is invalid"); return NULL; }
    return toPyTuple(self->obj->getSpecular());
}

PyObject* VRPyMaterial::setTransparency(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setTransparency, C obj is invalid"); return NULL; }
	self->obj->setTransparency(parseFloat(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getTransparency(VRPyMaterial* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::getTransparency, C obj is invalid"); return NULL; }
    return PyFloat_FromDouble(self->obj->getTransparency());
}

PyObject* VRPyMaterial::setShininess(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setShininess, C obj is invalid"); return NULL; }
	self->obj->setShininess(parseFloat(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getShininess(VRPyMaterial* self) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::getShininess, C obj is invalid"); return NULL; }
    return PyFloat_FromDouble(self->obj->getShininess());
}

PyObject* VRPyMaterial::setSpecular(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setSpecular, C obj is invalid"); return NULL; }
	self->obj->setSpecular(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setPointSize(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setPointSize, C obj is invalid"); return NULL; }
	self->obj->setPointSize(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setLineWidth(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setLineWidth, C obj is invalid"); return NULL; }
	self->obj->setLineWidth(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setQRCode(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setQRCode, C obj is invalid"); return NULL; }
	PyObject *data, *fg, *bg; int i;
    if (! PyArg_ParseTuple(args, "OOOi", &data, &fg, &bg, &i)) return NULL;
	self->obj->setQRCode(PyString_AsString(data), parseVec3fList(fg), parseVec3fList(bg), i);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setPerlin(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setQRCode, C obj is invalid"); return NULL; }
	PyObject *c1, *c2; int seed; float amount;
    if (! PyArg_ParseTuple(args, "OOif", &c1, &c2, &seed, &amount)) return NULL;
	OSG::VRTextureGenerator tgen;
	tgen.addNoise(OSG::PERLIN, amount, parseVec3fList(c1), parseVec3fList(c2));
	self->obj->setTexture(tgen.compose(seed));
	Py_RETURN_TRUE;
}
