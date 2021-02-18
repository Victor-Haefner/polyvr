#include "VRPyMaterial.h"
#include "VRPyTextureGenerator.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"
#include "VRPyImage.h"
#include "VRPyBaseFactory.h"

using namespace OSG;

simpleVRPyType(Material, New_VRObjects_ptr);

#ifndef WITHOUT_AV
simpleVRPyType(Video, 0);
#endif

#ifndef WITHOUT_AV
PyMethodDef VRPyVideo::methods[] = {
    {"play", PyWrapOpt(Video, play, "Play video", "0|0|-1|1", void, int, float, float, float ) },
    {"pause", PyWrap(Video, pause, "Pause video", void ) },
    {"resume", PyWrap(Video, resume, "Resume video after pause", void ) },
    {"isPaused", PyWrap(Video, isPaused, "Check if paused", bool ) },
    {"setVolume", PyWrap(Video, setVolume, "Set video volume, master volume can be set with soundmanager", void, float ) },
    {"getNFrames", PyWrapOpt(Video, getNFrames, "Get frame count of stream", "0", size_t, int ) },
    {"showFrame", PyWrap(Video, showFrame, "Show a specific frame of a stream (stream, frame)", void, int, int ) },
    {NULL}  /* Sentinel */
};
#endif

PyMethodDef VRPyMaterial::methods[] = {
    {"getAmbient", PyWrap(Material, getAmbient, "Returns the ambient color", Color3f ) },
    {"setAmbient", PyWrapPack(Material, setAmbient, "Sets the ambient color", void, Color3f ) },
    {"getDiffuse", PyWrap(Material, getDiffuse, "Returns the diffuse color", Color3f ) },
    {"setDiffuse", PyWrapPack(Material, setDiffuse, "Sets the diffuse color", void, Color3f ) },
    {"getSpecular", PyWrap(Material, getSpecular, "Returns the specular color", Color3f ) },
    {"setSpecular", PyWrapPack(Material, setSpecular, "Sets the specular color", void, Color3f ) },
    {"getTransparency", (PyCFunction)VRPyMaterial::getTransparency, METH_NOARGS, "Returns the transparency - f getTransparency()" },
    {"setTransparency", (PyCFunction)VRPyMaterial::setTransparency, METH_VARARGS, "Sets the transparency - setTransparency(f)" },
    {"setDepthTest", PyWrap(Material, setDepthTest, "Sets the depth test function\t\n'GL_ALWAYS'", void, int ) },
    {"getDepthTest", PyWrap(Material, getDepthTest, "Get depth test function", int ) },
    {"clearTransparency", (PyCFunction)VRPyMaterial::clearTransparency, METH_NOARGS, "Clears the transparency channel - clearTransparency()" },
    {"enableTransparency", (PyCFunction)VRPyMaterial::enableTransparency, METH_NOARGS, "Enables the transparency channel - enableTransparency()" },
    {"getShininess", (PyCFunction)VRPyMaterial::getShininess, METH_NOARGS, "Returns the shininess - f getShininess()" },
    {"setShininess", (PyCFunction)VRPyMaterial::setShininess, METH_VARARGS, "Sets the shininess - setShininess(f)" },
    {"setPointSize", (PyCFunction)VRPyMaterial::setPointSize, METH_VARARGS, "Sets the GL point size - setPointSize(i)" },
    {"setLineWidth", (PyCFunction)VRPyMaterial::setLineWidth, METH_VARARGS, "Sets the GL line width - setLineWidth(i)" },
    {"getTexture", (PyCFunction)VRPyMaterial::getTexture, METH_VARARGS, "Get the texture - texture getTexture( int unit = 0 )" },
    {"setQRCode", (PyCFunction)VRPyMaterial::setQRCode, METH_VARARGS, "Encode a string as QR code texture - setQRCode(string, fg[r,g,b], bg[r,g,b], offset)" },
    {"setMagMinFilter", (PyCFunction)VRPyMaterial::setMagMinFilter, METH_VARARGS, "Set the mag and min filtering mode - setMagMinFilter( mag, min | int unit )\n possible values for mag are GL_X && min can be GL_X || GL_X_MIPMAP_Y, where X && Y can be NEAREST || LINEAR" },
    {"setTextureWrapping", (PyCFunction)VRPyMaterial::setTextureWrapping, METH_VARARGS, "Set the texture wrap in u and v - setTextureWrapping( wrapU, wrapV | int unit )\n possible values for wrap are 'GL_CLAMP_TO_EDGE', 'GL_CLAMP_TO_BORDER', 'GL_CLAMP', 'GL_REPEAT'" },
    {"setVertexProgram", (PyCFunction)VRPyMaterial::setVertexProgram, METH_VARARGS, "Set vertex program - setVertexProgram( myScript )" },
    {"setFragmentProgram", (PyCFunction)VRPyMaterial::setFragmentProgram, METH_VARARGS, "Set fragment program - setFragmentProgram( myScript | bool deferred )" },
    {"setGeometryProgram", (PyCFunction)VRPyMaterial::setGeometryProgram, METH_VARARGS, "Set geometry program - setGeometryProgram( myScript )" },
    {"setTessControlProgram", (PyCFunction)VRPyMaterial::setTessControlProgram, METH_VARARGS, "Set tess control program - setTessControlProgram( myScript )" },
    {"setTessEvaluationProgram", (PyCFunction)VRPyMaterial::setTessEvaluationProgram, METH_VARARGS, "Set tess evaluation program - setTessEvaluationProgram( myScript )" },
    {"ignoreMeshColors", PyWrapOpt( Material, ignoreMeshColors, "Allows to ignore vertex colors, only works if lit", "1", void, bool ) },
    {"doesIgnoreMeshColors", PyWrap( Material, doesIgnoreMeshColors, "Check if ignoring vertex colors", bool ) },
    {"setWireFrame", PyWrap( Material, setWireFrame, "Set front and back to wireframe", void, bool ) },
    {"isWireFrame", PyWrap( Material, isWireFrame, "Check if wireframe", bool ) },
    {"setLit", (PyCFunction)VRPyMaterial::setLit, METH_VARARGS, "Set if geometry is lit - setLit(bool)" },
    {"isLit", PyWrap( Material, isLit, "Get if geometry is lit", bool ) },
    {"addPass", PyWrap( Material, addPass, "Add a new pass", int ) },
    {"setActivePass", PyWrap( Material, setActivePass, "Set active pass", void, int ) },
    {"remPass", PyWrap( Material, remPass, "Remove a pass", void, int ) },
    {"getNPasses", PyWrap( Material, getNPasses, "Get number of passes", int ) },
    {"setFrontBackModes", PyWrap(Material, setFrontBackModes, "Set the draw mode of front and back faces - setFrontBackModes(front, back)\n\tmode can be: GL_NONE, GL_FILL, GL_LINE, GL_BACK", void, int, int) },
    {"setZOffset", PyWrap(Material, setZOffset, "Set Z buffer offset and bias, set to 1/1 to push behind or -1/-1 to pull to front", void, float, float) },
    {"setSortKey", (PyCFunction)VRPyMaterial::setSortKey, METH_VARARGS, "Set the sort key, higher key means rendered later, for example 0 is rendered first, 1 second.." },
    {"setTexture", (PyCFunction)VRPyMaterial::setTexture, METH_VARARGS, "Set the texture - setTexture(str path, int pos = 0)"
                                                                        "\n\t setTexture( Texture, int pos = 0)"
                                                                        "\n\t setTexture([[r,g,b]], [xN, yN, zN], bool isFloat)"
                                                                        "\n\t setTexture([[r,g,b,a]], [xN, yN, zN], bool isFloat)" },
    {"setTextureType", (PyCFunction)VRPyMaterial::setTextureType, METH_VARARGS, "Set the texture type - setTexture(str type)\n types are: 'Normal, 'SphereEnv'" },
    {"setStencilBuffer", (PyCFunction)VRPyMaterial::setStencilBuffer, METH_VARARGS, "Set the setStencilBuffer" },
    {"setShaderParameter", (PyCFunction)VRPyMaterial::setShaderParameter, METH_VARARGS, "Set shader variable - setShaderParameter(str var, value)" },
    {"enableShaderParameter", PyWrap(Material, enableShaderParameter, "Enable OSG shader variable which can be one of"
        "\n\t { OSGWorldMatrix OSGInvWorldMatrix OSGTransInvWorldMatrix OSGCameraOrientation OSGCameraPosition OSGViewMatrix OSGInvViewMatrix"
        "\n\t OSGProjectionMatrix OSGModelViewMatrix OSGViewportSize OSGNormalMatrix OSGModelViewProjectionMatrix OSGStereoLeftEye OSGDrawerId"
        "\n\t OSGDrawableId OSGNodeId OSGNodeBoxMin OSGNodeBoxMax OSGNodeBoxCenter OSGNodeWorldBoxMin"
        "\n\t OSGNodeWorldBoxMax OSGNodeWorldBoxCenter OSGActiveLightsMask OSGLight0Active OSGLight1Active OSGLight2Active"
        "\n\t OSGLight3Active OSGLight4Active OSGLight5Active OSGLight6Active OSGLight7Active }",
        void, string) },
    {"setDefaultVertexShader", (PyCFunction)VRPyMaterial::setDefaultVertexShader, METH_NOARGS, "Set a default vertex shader - setDefaultVertexShader()" },
    {"testFix", PyWrap( Material, testFix, "Debug helper", void ) },
    {"setMappingBeacon", PyWrapOpt( Material, setMappingBeacon, "Set beacon for cube mapping", "0", void, VRObjectPtr, int ) },
    {"setMappingPlanes", PyWrapOpt( Material, setMappingPlanes, "Set planes for cube mapping", "0", void, Vec4d, Vec4d, Vec4d, Vec4d, int ) },
    {"setCubeTexture", PyWrapOpt( Material, setCubeTexture, "Set cube texture, (texture, side, unit)\nside can be (front, back, left, right, top, bottom)", "0", void, VRTexturePtr, string, int ) },
    {"updateDeferredShader", PyWrap( Material, updateDeferredShader, "Reload deferred system parameters and shaders", void ) },

    {"readVertexShader", PyWrap( Material, readVertexShader, "Read vertex shader from file", void, string ) },
    {"readFragmentShader", PyWrap( Material, readFragmentShader, "Read fragment shader from file", void, string, bool ) },
    {"readGeometryShader", PyWrap( Material, readGeometryShader, "Read geometry shader from file", void, string ) },
    {"readTessControlShader", PyWrap( Material, readTessControlShader, "Read tesselation control shader from file", void, string ) },
    {"readTessEvaluationShader", PyWrap( Material, readTessEvaluationShader, "Read tesselation evaluation shader from file", void, string ) },

    {"getVertexShader", PyWrap( Material, getVertexShader, "Get vertex shader", string ) },
    {"getFragmentShader", PyWrapOpt( Material, getFragmentShader, "Get fragment shader", "0", string, bool ) },
    {"getGeometryShader", PyWrap( Material, getGeometryShader, "Get geometry shader", string ) },
    {"getTessControlShader", PyWrap( Material, getTessControlShader, "Get tesselation control shader", string ) },
    {"getTessEvaluationShader", PyWrap( Material, getTessEvaluationShader, "Get tesselation evaluation shader", string ) },

#ifndef WITHOUT_AV
    {"setVideo", PyWrap( Material, setVideo, "Set video as texture", VRVideoPtr, string ) },
    {"getVideo", PyWrap( Material, getVideo, "Get video", VRVideoPtr ) },
#endif

    {"diff", PyWrap( Material, diff, "Returns a diff report with another material", string, VRMaterialPtr ) },
    {NULL}  /* Sentinel */
};

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
	return VRPyTexture::fromSharedPtr( self->objPtr->getTexture(i) );
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
    const char* t = 0;
    int u = 0;
    if (! PyArg_ParseTuple(args, "s|i", &t, &u)) return NULL;
	self->objPtr->setTextureType(t?string(t):"", u);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTexture(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTexture, C obj is invalid"); return NULL; }

	int aN = pySize(args);
	if (aN <= 2) {
        PyObject* o = 0; int pos = 0;
        if (! PyArg_ParseTuple(args, "O|i", &o, &pos)) return NULL;
        if (PyString_Check(o)) self->objPtr->setTexture( PyString_AsString(o), true, pos ); // load a file
        else if (VRPyTexture::check(o)) {
            VRPyTexture* img = (VRPyTexture*)o;
            self->objPtr->setTexture( img->objPtr, false, pos );
        }
	}

	if (aN > 2) {
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
	int unit = 0;
    if (! PyArg_ParseTuple(args, "OO|i", &mag, &min, &unit)) return NULL;
	self->objPtr->setMagMinFilter(toGLConst(mag), toGLConst(min), unit);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTextureWrapping(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTextureWrapping, C obj is invalid"); return NULL; }
	PyObject *ws, *wt;
	int unit = 0;
    if (! PyArg_ParseTuple(args, "OO|i", &ws, &wt, &unit)) return NULL;
	self->objPtr->setTextureWrapping(toGLConst(ws), toGLConst(wt), unit);
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setVertexProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setVertexProgram, C obj is invalid"); return NULL; }
	self->objPtr->setVertexScript(parseString(args));
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setFragmentProgram(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setFragmentProgram, C obj is invalid"); return NULL; }
	const char* code = 0;
	int def = 0;
    if (! PyArg_ParseTuple(args, "s|i", (char*)&code, &def)) return NULL;
	self->objPtr->setFragmentScript(code, def);
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
	self->objPtr->setQRCode(PyString_AsString(data), parseVec3dList(fg), parseVec3dList(bg), i);
	Py_RETURN_TRUE;
}
