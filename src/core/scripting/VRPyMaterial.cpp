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
    {"getAmbient", (PyCFunction)VRPyMaterial::getAmbient, METH_NOARGS, "Returns the ambient color - [f,f,f] getAmbient()" },
    {"setAmbient", (PyCFunction)VRPyMaterial::setAmbient, METH_VARARGS, "Sets the ambient color - setAmbient([f,f,f])" },
    {"getDiffuse", (PyCFunction)VRPyMaterial::getDiffuse, METH_NOARGS, "Returns the diffuse color - [f,f,f] getDiffuse()" },
    {"setDiffuse", (PyCFunction)VRPyMaterial::setDiffuse, METH_VARARGS, "Sets the diffuse color - setDiffuse([f,f,f])" },
    {"getSpecular", (PyCFunction)VRPyMaterial::getSpecular, METH_NOARGS, "Returns the specular color - [f,f,f] getSpecular()" },
    {"setSpecular", (PyCFunction)VRPyMaterial::setSpecular, METH_VARARGS, "Sets the specular color - setSpecular([f,f,f])" },
    {"getTransparency", (PyCFunction)VRPyMaterial::getTransparency, METH_NOARGS, "Returns the transparency - f getTransparency()" },
    {"setTransparency", (PyCFunction)VRPyMaterial::setTransparency, METH_VARARGS, "Sets the transparency - setTransparency(f)" },
    {"getShininess", (PyCFunction)VRPyMaterial::getShininess, METH_NOARGS, "Returns the shininess - f getShininess()" },
    {"setShininess", (PyCFunction)VRPyMaterial::setShininess, METH_VARARGS, "Sets the shininess - setShininess(f)" },
    {"setPointSize", (PyCFunction)VRPyMaterial::setPointSize, METH_VARARGS, "Sets the GL point size - setPointSize(i)" },
    {"setLineWidth", (PyCFunction)VRPyMaterial::setLineWidth, METH_VARARGS, "Sets the GL line width - setLineWidth(i)" },
    {"setPerlin", (PyCFunction)VRPyMaterial::setPerlin, METH_VARARGS, "Set a perlin noise texture - setPerlin(col1[r,g,b], col2[r,g,b], int seed, float amount)" },
    {"setQRCode", (PyCFunction)VRPyMaterial::setQRCode, METH_VARARGS, "Encode a string as QR code texture - setQRCode(string, fg[r,g,b], bg[r,g,b], offset)" },
    {"setMagMinFilter", (PyCFunction)VRPyMaterial::setMagMinFilter, METH_VARARGS, "Set the mag && min filtering mode - setMagMinFilter( mag, min)\n possible values for mag are GL_X && min can be GL_X || GL_X_MIPMAP_Y, where X && Y can be NEAREST || LINEAR" },
    {"setVertexProgram", (PyCFunction)VRPyMaterial::setVertexProgram, METH_VARARGS, "Set vertex program - setVertexProgram( myScript )" },
    {"setFragmentProgram", (PyCFunction)VRPyMaterial::setFragmentProgram, METH_VARARGS, "Set fragment program - setFragmentProgram( myScript )" },
    {"setGeometryProgram", (PyCFunction)VRPyMaterial::setGeometryProgram, METH_VARARGS, "Set geometry program - setGeometryProgram( myScript )" },
    {"setWireFrame", (PyCFunction)VRPyMaterial::setWireFrame, METH_VARARGS, "Set wireframe mode - setWireFrame(bool)" },
    {"setLit", (PyCFunction)VRPyMaterial::setLit, METH_VARARGS, "Set if geometry is lit - setLit(bool)" },
    {"addPass", (PyCFunction)VRPyMaterial::addPass, METH_NOARGS, "Add a new pass - i addPass()" },
    {"remPass", (PyCFunction)VRPyMaterial::remPass, METH_VARARGS, "Remove a pass - remPass(i)" },
    {"setActivePass", (PyCFunction)VRPyMaterial::setActivePass, METH_VARARGS, "Activate a pass - setActivePass(i)" },
    {"setZOffset", (PyCFunction)VRPyMaterial::setZOffset, METH_VARARGS, "Set the z offset factor and bias - setZOffset(factor, bias)" },
    {"setTexture", (PyCFunction)VRPyMaterial::setTexture, METH_VARARGS, "Set the texture - setTexture(str path)\n - setTexture([[r,g,b]], [xN, yN, zN], bool isFloat)\n - setTexture([[r,g,b,a]], [xN, yN, zN], bool isFloat)" },
    {"setTextureType", (PyCFunction)VRPyMaterial::setTextureType, METH_VARARGS, "Set the texture type - setTexture(str type)\n types are: 'Normal, 'SphereEnv'" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMaterial::setTextureType(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setTextureType, C obj is invalid"); return NULL; }
	self->obj->setTextureType(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTexture(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setTexture, C obj is invalid"); return NULL; }

	int aN = pySize(args);
	if (aN == 1) self->obj->setTexture( parseString(args) );

	if (aN > 1) {
        PyObject *data, *dims; int doFl;
        if (! PyArg_ParseTuple(args, "OOi", &data, &dims, &doFl)) return NULL;

        if (pySize(data) == 0) Py_RETURN_TRUE;
        vector<PyObject*> _data = pyListToVector(data);
        int dN = _data.size();

        int vN = pySize(_data[0]);
        if (doFl) {
            vector<float> buf(vN*dN, 0);
            for (int i=0; i<dN; i++) {
                PyObject* dObj = _data[i];
                vector<PyObject*> vec = pyListToVector(dObj);
                for (int j=0; j<vN; j++) buf[i*vN+j] = PyFloat_AsDouble(vec[j]);
            }
            self->obj->setTexture( (char*)&buf[0], vN, parseVec3iList(dims), true );
        } else {
            vector<char> buf(vN*dN, 0);
            for (int i=0; i<dN; i++) {
                vector<PyObject*> vec = pyListToVector(_data[i]);
                for (int j=0; j<vN; j++) buf[i*vN+j] = PyInt_AsLong(vec[j]);
            }
            self->obj->setTexture( &buf[0], vN, parseVec3iList(dims), false );
        }
	}

	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setZOffset(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setZOffset, C obj is invalid"); return NULL; }
	float f,b;
    if (! PyArg_ParseTuple(args, "ff", &f, &b)) return NULL;
	self->obj->setZOffset(f,b);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setLit(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setLit, C obj is invalid"); return NULL; }
	self->obj->setLit(parseBool(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::addPass(VRPyMaterial* self) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::addPass, C obj is invalid"); return NULL; }
	return PyInt_FromLong( self->obj->addPass() );
}

PyObject* VRPyMaterial::remPass(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::remPass, C obj is invalid"); return NULL; }
	self->obj->remPass(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setActivePass(VRPyMaterial* self, PyObject* args) {
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setActivePass, C obj is invalid"); return NULL; }
	self->obj->setActivePass(parseInt(args));
	Py_RETURN_TRUE;
}

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
	if (self->obj == 0) { PyErr_SetString(err, "VRPyMaterial::setPerlin, C obj is invalid"); return NULL; }
	PyObject *c1, *c2; int seed; float amount;
    if (! PyArg_ParseTuple(args, "OOif", &c1, &c2, &seed, &amount)) return NULL;
	OSG::VRTextureGenerator tgen;
	tgen.add(OSG::PERLIN, amount, parseVec3fList(c1), parseVec3fList(c2));
	self->obj->setTexture(tgen.compose(seed));
	Py_RETURN_TRUE;
}
