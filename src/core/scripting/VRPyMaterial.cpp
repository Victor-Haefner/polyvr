#include "VRPyMaterial.h"
#include "VRPyTextureGenerator.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "VRPyImage.h"

using namespace OSG;

simpleVRPyType(Material, New_VRObjects_ptr);

PyMethodDef VRPyMaterial::methods[] = {
    {"getAmbient", (PyCFunction)VRPyMaterial::getAmbient, METH_NOARGS, "Returns the ambient color - [f,f,f] getAmbient()" },
    {"setAmbient", (PyCFunction)VRPyMaterial::setAmbient, METH_VARARGS, "Sets the ambient color - setAmbient([f,f,f])" },
    {"getDiffuse", (PyCFunction)VRPyMaterial::getDiffuse, METH_NOARGS, "Returns the diffuse color - [f,f,f] getDiffuse()" },
    {"setDiffuse", (PyCFunction)VRPyMaterial::setDiffuse, METH_VARARGS, "Sets the diffuse color - setDiffuse([f,f,f])" },
    {"getSpecular", (PyCFunction)VRPyMaterial::getSpecular, METH_NOARGS, "Returns the specular color - [f,f,f] getSpecular()" },
    {"setSpecular", (PyCFunction)VRPyMaterial::setSpecular, METH_VARARGS, "Sets the specular color - setSpecular([f,f,f])" },
    {"getTransparency", (PyCFunction)VRPyMaterial::getTransparency, METH_NOARGS, "Returns the transparency - f getTransparency()" },
    {"setTransparency", (PyCFunction)VRPyMaterial::setTransparency, METH_VARARGS, "Sets the transparency - setTransparency(f)" },
    {"setDepthTest", (PyCFunction)VRPyMaterial::setDepthTest, METH_VARARGS, "Sets the depth test function - setDepthTest(f)\t\n'GL_ALWAYS'" },
    {"clearTransparency", (PyCFunction)VRPyMaterial::clearTransparency, METH_NOARGS, "Clears the transparency channel - clearTransparency()" },
    {"enableTransparency", (PyCFunction)VRPyMaterial::enableTransparency, METH_NOARGS, "Enables the transparency channel - enableTransparency()" },
    {"getShininess", (PyCFunction)VRPyMaterial::getShininess, METH_NOARGS, "Returns the shininess - f getShininess()" },
    {"setShininess", (PyCFunction)VRPyMaterial::setShininess, METH_VARARGS, "Sets the shininess - setShininess(f)" },
    {"setPointSize", (PyCFunction)VRPyMaterial::setPointSize, METH_VARARGS, "Sets the GL point size - setPointSize(i)" },
    {"setLineWidth", (PyCFunction)VRPyMaterial::setLineWidth, METH_VARARGS, "Sets the GL line width - setLineWidth(i)" },
    {"getTexture", (PyCFunction)VRPyMaterial::getTexture, METH_VARARGS, "Get the texture - texture getTexture( int unit = 0 )" },
    {"setQRCode", (PyCFunction)VRPyMaterial::setQRCode, METH_VARARGS, "Encode a string as QR code texture - setQRCode(string, fg[r,g,b], bg[r,g,b], offset)" },
    {"setMagMinFilter", (PyCFunction)VRPyMaterial::setMagMinFilter, METH_VARARGS, "Set the mag && min filtering mode - setMagMinFilter( mag, min)\n possible values for mag are GL_X && min can be GL_X || GL_X_MIPMAP_Y, where X && Y can be NEAREST || LINEAR" },
    {"setVertexProgram", (PyCFunction)VRPyMaterial::setVertexProgram, METH_VARARGS, "Set vertex program - setVertexProgram( myScript )" },
    {"setFragmentProgram", (PyCFunction)VRPyMaterial::setFragmentProgram, METH_VARARGS, "Set fragment program - setFragmentProgram( myScript )" },
    {"setGeometryProgram", (PyCFunction)VRPyMaterial::setGeometryProgram, METH_VARARGS, "Set geometry program - setGeometryProgram( myScript )" },
    {"setTessControlProgram", (PyCFunction)VRPyMaterial::setTessControlProgram, METH_VARARGS, "Set tess control program - setTessControlProgram( myScript )" },
    {"setTessEvaluationProgram", (PyCFunction)VRPyMaterial::setTessEvaluationProgram, METH_VARARGS, "Set tess evaluation program - setTessEvaluationProgram( myScript )" },
    {"setWireFrame", (PyCFunction)VRPyMaterial::setWireFrame, METH_VARARGS, "Set wireframe mode - setWireFrame(bool)" },
    {"setLit", (PyCFunction)VRPyMaterial::setLit, METH_VARARGS, "Set if geometry is lit - setLit(bool)" },
    {"addPass", (PyCFunction)VRPyMaterial::addPass, METH_NOARGS, "Add a new pass - i addPass()" },
    {"remPass", (PyCFunction)VRPyMaterial::remPass, METH_VARARGS, "Remove a pass - remPass(i)" },
    {"setActivePass", (PyCFunction)VRPyMaterial::setActivePass, METH_VARARGS, "Activate a pass - setActivePass(i)" },
    {"setFrontBackModes", (PyCFunction)VRPyMaterial::setFrontBackModes, METH_VARARGS, "Set the draw mode of front and back faces - setFrontBackModes(front, back)\n\tmode can be: GL_NONE, GL_FILL, GL_BACK" },
    {"setZOffset", (PyCFunction)VRPyMaterial::setZOffset, METH_VARARGS, "Set the z offset factor and bias - setZOffset(factor, bias)" },
    {"setSortKey", (PyCFunction)VRPyMaterial::setSortKey, METH_VARARGS, "Set the sort key" },
    {"setTexture", (PyCFunction)VRPyMaterial::setTexture, METH_VARARGS, "Set the texture - setTexture(str path)\n - setTexture([[r,g,b]], [xN, yN, zN], bool isFloat)\n - setTexture([[r,g,b,a]], [xN, yN, zN], bool isFloat)" },
    {"setTextureType", (PyCFunction)VRPyMaterial::setTextureType, METH_VARARGS, "Set the texture type - setTexture(str type)\n types are: 'Normal, 'SphereEnv'" },
    {"setStencilBuffer", (PyCFunction)VRPyMaterial::setStencilBuffer, METH_VARARGS, "Set the setStencilBuffer" },
    {"setShaderParameter", (PyCFunction)VRPyMaterial::setShaderParameter, METH_VARARGS, "Set shader variable - setShaderParameter(str var, value)" },
    {"setDefaultVertexShader", (PyCFunction)VRPyMaterial::setDefaultVertexShader, METH_NOARGS, "Set a default vertex shader - setDefaultVertexShader()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyMaterial::setFrontBackModes(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::enableTransparency, C obj is invalid"); return NULL; }
	const char* s1 = "GL_FILL";
	const char* s2 = "GL_FILL";
    if (! PyArg_ParseTuple(args, "s|s", &s1, &s2)) return NULL;
    cout << "setFrontBackModes " << s1 << " " << s2 << endl;
    int m1 = toGLConst(string(s1));
    int m2 = toGLConst(string(s2));
    cout << "setFrontBackModes " << m1 << " " << m2 << endl;
	self->objPtr->setFrontBackModes(m1, m2);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::enableTransparency(VRPyMaterial* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::enableTransparency, C obj is invalid"); return NULL; }
	self->objPtr->enableTransparency(true);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::clearTransparency(VRPyMaterial* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::clearTransparency, C obj is invalid"); return NULL; }
	self->objPtr->clearTransparency(true);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setDefaultVertexShader(VRPyMaterial* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setDefaultVertexShader, C obj is invalid"); return NULL; }
	self->objPtr->setDefaultVertexShader();
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setDepthTest(VRPyMaterial* self, PyObject* args) {
    if (!self->valid()) return NULL;
	const char* s=0;
    if (! PyArg_ParseTuple(args, "s", (char*)&s)) return NULL;
	if (s) self->objPtr->setDepthTest( toGLConst(s) );
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getTexture(VRPyMaterial* self, PyObject* args) {
    if (!self->valid()) return NULL;
	int i=0;
    if (! PyArg_ParseTuple(args, "|i", &i)) return NULL;
	return VRPyImage::fromSharedPtr( self->objPtr->getTexture(i) );
}

PyObject* VRPyMaterial::setShaderParameter(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setShaderParameter, C obj is invalid"); return NULL; }
	PyStringObject* var;
	int i=0;
    if (! PyArg_ParseTuple(args, "Oi", &var, &i)) return NULL;
	self->objPtr->setShaderParameter( PyString_AsString((PyObject*)var), i );
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setSortKey(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setSortKey, C obj is invalid"); return NULL; }
	self->objPtr->setSortKey( parseInt(args) );
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setStencilBuffer(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setStencilBuffer, C obj is invalid"); return NULL; }
	int c,v,m;
	PyObject *o,*f1,*f2,*f3;
    if (! PyArg_ParseTuple(args, "iiiOOOO", &c, &v, &m, &o, &f1, &f2, &f3)) return NULL;
	self->objPtr->setStencilBuffer(c,v,m, toGLConst(o), toGLConst(f1), toGLConst(f2), toGLConst(f3));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTextureType(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTextureType, C obj is invalid"); return NULL; }
	self->objPtr->setTextureType(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTexture(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTexture, C obj is invalid"); return NULL; }

	int aN = pySize(args);
	if (aN == 1) {
        PyObject* o = parseObject(args);
        if (PyString_Check(o)) self->objPtr->setTexture( PyString_AsString(o) ); // load a file
        else if (VRPyImage::check(o)) {
            VRPyImage* img = (VRPyImage*)o;
            self->objPtr->setTexture( img->objPtr, 0 );
        }
	}

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
            self->objPtr->setTexture( (char*)&buf[0], vN, parseVec3iList(dims), true );
        } else {
            vector<char> buf(vN*dN, 0);
            for (int i=0; i<dN; i++) {
                vector<PyObject*> vec = pyListToVector(_data[i]);
                for (int j=0; j<vN; j++) buf[i*vN+j] = PyInt_AsLong(vec[j]);
            }
            self->objPtr->setTexture( &buf[0], vN, parseVec3iList(dims), false );
        }
	}

	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setZOffset(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setZOffset, C obj is invalid"); return NULL; }
	float f,b;
    if (! PyArg_ParseTuple(args, "ff", &f, &b)) return NULL;
	self->objPtr->setZOffset(f,b);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setLit(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setLit, C obj is invalid"); return NULL; }
	self->objPtr->setLit(parseBool(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::addPass(VRPyMaterial* self) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::addPass, C obj is invalid"); return NULL; }
	return PyInt_FromLong( self->objPtr->addPass() );
}

PyObject* VRPyMaterial::remPass(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::remPass, C obj is invalid"); return NULL; }
	self->objPtr->remPass(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setActivePass(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setActivePass, C obj is invalid"); return NULL; }
	self->objPtr->setActivePass(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setWireFrame(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setWireFrame, C obj is invalid"); return NULL; }
	self->objPtr->setWireFrame(parseBool(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setMagMinFilter(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setMagMinFilter, C obj is invalid"); return NULL; }
	PyObject *mag, *min;
    if (! PyArg_ParseTuple(args, "OO", &mag, &min)) return NULL;
	self->objPtr->setMagMinFilter(PyString_AsString(mag), PyString_AsString(min));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setVertexProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setVertexProgram, C obj is invalid"); return NULL; }
	self->objPtr->setVertexScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setFragmentProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setFragmentProgram, C obj is invalid"); return NULL; }
	self->objPtr->setFragmentScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setGeometryProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setGeometryProgram, C obj is invalid"); return NULL; }
	self->objPtr->setGeometryScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTessControlProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTessControlProgram, C obj is invalid"); return NULL; }
	self->objPtr->setTessControlScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTessEvaluationProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTessEvaluationProgram, C obj is invalid"); return NULL; }
	self->objPtr->setTessEvaluationScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getAmbient(VRPyMaterial* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::getAmbient, C obj is invalid"); return NULL; }
    return toPyTuple(self->objPtr->getAmbient());
}

PyObject* VRPyMaterial::setAmbient(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setAmbient, C obj is invalid"); return NULL; }
	self->objPtr->setAmbient(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getDiffuse(VRPyMaterial* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::getDiffuse, C obj is invalid"); return NULL; }
    return toPyTuple(self->objPtr->getDiffuse());
}

PyObject* VRPyMaterial::setDiffuse(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setDiffuse, C obj is invalid"); return NULL; }
	self->objPtr->setDiffuse(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getSpecular(VRPyMaterial* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::getSpecular, C obj is invalid"); return NULL; }
    return toPyTuple(self->objPtr->getSpecular());
}

PyObject* VRPyMaterial::setTransparency(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTransparency, C obj is invalid"); return NULL; }
	self->objPtr->setTransparency(parseFloat(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getTransparency(VRPyMaterial* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::getTransparency, C obj is invalid"); return NULL; }
    return PyFloat_FromDouble(self->objPtr->getTransparency());
}

PyObject* VRPyMaterial::setShininess(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setShininess, C obj is invalid"); return NULL; }
	self->objPtr->setShininess(parseFloat(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::getShininess(VRPyMaterial* self) {
    if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::getShininess, C obj is invalid"); return NULL; }
    return PyFloat_FromDouble(self->objPtr->getShininess());
}

PyObject* VRPyMaterial::setSpecular(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setSpecular, C obj is invalid"); return NULL; }
	self->objPtr->setSpecular(parseVec3f(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setPointSize(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setPointSize, C obj is invalid"); return NULL; }
	self->objPtr->setPointSize(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setLineWidth(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setLineWidth, C obj is invalid"); return NULL; }
	self->objPtr->setLineWidth(parseInt(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setQRCode(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setQRCode, C obj is invalid"); return NULL; }
	PyObject *data, *fg, *bg; int i;
    if (! PyArg_ParseTuple(args, "OOOi", &data, &fg, &bg, &i)) return NULL;
	self->objPtr->setQRCode(PyString_AsString(data), parseVec3fList(fg), parseVec3fList(bg), i);
	Py_RETURN_TRUE;
}
