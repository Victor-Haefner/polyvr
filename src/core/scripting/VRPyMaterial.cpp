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
    {"isRunning", PyWrap(Video, isRunning, "Check if running", bool ) },
    {"setVolume", PyWrap(Video, setVolume, "Set video volume, master volume can be set with soundmanager", void, float ) },
    {"getNFrames", PyWrapOpt(Video, getNFrames, "Get frame count of stream", "0", size_t, int ) },
    {"showFrame", PyWrap(Video, showFrame, "Show a specific frame of a stream (stream, frame)", void, int, int ) },
    {"getDuration", PyWrap(Video, getDuration, "Get total duration", float ) },
    {"goTo", PyWrap(Video, goTo, "Go to position t [0.0, 1.0]", void, float ) },
    {NULL}  /* Sentinel */
};
#endif

PyMethodDef VRPyMaterial::methods[] = {
    {"getAmbient", PyWrap(Material, getAmbient, "Returns the ambient color", Color3f ) },
    {"setAmbient", PyWrapPack(Material, setAmbient, "Sets the ambient color", void, Color3f ) },
    {"getDiffuse", PyWrap(Material, getDiffuse, "Returns the diffuse color", Color3f ) },
    {"setDiffuse", PyWrapPack(Material, setDiffuse, "Sets the diffuse color", void, Color3f ) },
    {"getEmission", PyWrap(Material, getEmission, "Returns the emission color", Color3f ) },
    {"setEmission", PyWrapPack(Material, setEmission, "Sets the emission color", void, Color3f ) },
    {"getSpecular", PyWrap(Material, getSpecular, "Returns the specular color", Color3f ) },
    {"setSpecular", PyWrapPack(Material, setSpecular, "Sets the specular color", void, Color3f ) },
    {"getTransparency", PyWrap(Material, getTransparency, "Returns the transparency - f getTransparency()", float ) },
    {"setTransparency", PyWrap(Material, setTransparency, "Sets the transparency - setTransparency(f)", void, float ) },
    {"setClipPlane", PyWrap(Material, setClipPlane, "Set the clip chunk, (active, plane eq., beacon)", void, bool, Vec4d, VRTransformPtr) },
    {"addClipPlane", PyWrap(Material, addClipPlane, "Add a clip chunk, (plane eq., beacon)", void, Vec4d, VRTransformPtr) },
    {"setDepthTest", PyWrap(Material, setDepthTest, "Sets the depth test function\t\n'GL_ALWAYS'", void, int ) },
    {"getDepthTest", PyWrap(Material, getDepthTest, "Get depth test function", int ) },
    {"clearTransparency", PyWrapOpt(Material, clearTransparency, "Clears the transparency channel - clearTransparency()", "1", void, bool ) },
    {"enableTransparency", PyWrapOpt(Material, enableTransparency, "Enables the transparency channel - enableTransparency()", "1", void, bool ) },
    {"getShininess", PyWrap(Material, getShininess, "Returns the shininess - f getShininess()", float ) },
    {"setShininess", PyWrap(Material, setShininess, "Sets the shininess - setShininess(f)", void, float ) },
    {"setPointSize", PyWrapOpt(Material, setPointSize, "Sets the GL point size - setPointSize(i, smooth = true)", "1", void, int, bool ) },
    {"setLineWidth", PyWrapOpt(Material, setLineWidth, "Sets the GL line width - setLineWidth(i, smooth = true)", "1", void, int, bool ) },
    {"getTexture", PyWrapOpt(Material, getTexture, "Get the texture - texture getTexture( int unit = 0 )", "0", VRTexturePtr, int ) },
    {"setQRCode", PyWrap(Material, setQRCode, "Encode a string as QR code texture - setQRCode(string, fg[r,g,b], bg[r,g,b], offset)", void, string, Color3f, Color3f, int) },
    {"setMagMinFilter", PyWrapOpt(Material, setMagMinFilter, "Set the mag and min filtering mode - setMagMinFilter( mag, min | int unit )\n possible values for mag are GL_X && min can be GL_X || GL_X_MIPMAP_Y, where X && Y can be NEAREST || LINEAR", "0", void, int, int, int ) },
    {"setTextureWrapping", PyWrap(Material, setTextureWrapping, "Set the texture wrap in u and v - setTextureWrapping( wrapU, wrapV | int unit )\n possible values for wrap are 'GL_CLAMP_TO_EDGE', 'GL_CLAMP_TO_BORDER', 'GL_CLAMP', 'GL_REPEAT'", void, int, int, int ) },
    {"setVertexProgram", PyWrap(Material, setVertexScript, "Set vertex program - setVertexProgram( myScript )", void, string ) },
    {"setFragmentProgram", PyWrap(Material, setFragmentScript, "Set fragment program - setFragmentProgram( myScript | bool deferred )", void, string, bool ) },
    {"setGeometryProgram", PyWrap(Material, setGeometryScript, "Set geometry program - setGeometryProgram( myScript )", void, string ) },
    {"setTessControlProgram", PyWrap(Material, setTessControlScript, "Set tess control program - setTessControlProgram( myScript )", void, string ) },
    {"setTessEvaluationProgram", PyWrap(Material, setTessEvaluationScript, "Set tess evaluation program - setTessEvaluationProgram( myScript )", void, string ) },
    {"ignoreMeshColors", PyWrapOpt( Material, ignoreMeshColors, "Allows to ignore vertex colors, only works if lit", "1", void, bool ) },
    {"doesIgnoreMeshColors", PyWrap( Material, doesIgnoreMeshColors, "Check if ignoring vertex colors", bool ) },
    {"setWireFrame", PyWrap( Material, setWireFrame, "Set front and back to wireframe", void, bool ) },
    {"isWireFrame", PyWrap( Material, isWireFrame, "Check if wireframe", bool ) },
    {"setLit", PyWrap(Material, setLit, "Set if geometry is lit - setLit(bool)", void, bool ) },
    {"isLit", PyWrap( Material, isLit, "Get if geometry is lit", bool ) },
    {"addPass", PyWrap( Material, addPass, "Add a new pass", int ) },
    {"setActivePass", PyWrap( Material, setActivePass, "Set active pass", void, int ) },
    {"remPass", PyWrap( Material, remPass, "Remove a pass", void, int ) },
    {"getNPasses", PyWrap( Material, getNPasses, "Get number of passes", int ) },
    {"setFrontBackModes", PyWrap(Material, setFrontBackModes, "Set the draw mode of front and back faces - setFrontBackModes(front, back)\n\tmode can be: GL_NONE, GL_FILL, GL_LINE, GL_BACK", void, int, int) },
    {"setZOffset", PyWrap(Material, setZOffset, "Set Z buffer offset and bias, set to 1/1 to push behind or -1/-1 to pull to front", void, float, float) },
    {"setSortKey", PyWrap(Material, setSortKey, "Set the sort key, higher key means rendered later, for example 0 is rendered first, 1 second..", void, int ) },
    {"setTexture", (PyCFunction)VRPyMaterial::setTexture, METH_VARARGS, "Set the texture - setTexture(str path, int pos = 0)"
                                                                        "\n\t setTexture( Texture, int pos = 0)"
                                                                        "\n\t setTexture([[r,g,b]], [xN, yN, zN], bool isFloat)"
                                                                        "\n\t setTexture([[r,g,b,a]], [xN, yN, zN], bool isFloat)" },
    {"setTextureType", PyWrapOpt(Material, setTextureType, "Set the texture type - setTextureType(str type, unit = 0)\n types are: 'Normal, 'SphereEnv'", "0", void, string, int ) },
    {"setStencilBuffer", PyWrap(Material, setStencilBuffer
        , "Use stencil buffer, (doClear, value, mask, func, opFail, opZFail, opPass)"
        "\n\t doClear clears the buffer to 0"
        "\n\t value and mask used for the test, the mask is ANDed with the value as well as the stored value in buffer, set mask to -1 to disable it"
        "\n\t test function can be GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL, or GL_ALWAYS"
        "\n\t opFail is the action taken when stencil test fails, opZFail when stencil test passes but depth test fails, and opPass if both tests pass"
        "\n\t possible actions are: GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_INCR_WRAP, GL_DECR, GL_DECR_WRAP, and GL_INVERT"
        , void, bool, int, int, int, int, int, int) },
    {"setShaderParameter", (PyCFunction)VRPyMaterial::setShaderParameter, METH_VARARGS, "Set shader variable - setShaderParameter(str var, value)" },
    {"enableShaderParameter", PyWrap(Material, enableShaderParameter, "Enable OSG shader variable which can be one of"
        "\n\t { OSGWorldMatrix OSGInvWorldMatrix OSGTransInvWorldMatrix OSGCameraOrientation OSGCameraPosition OSGViewMatrix OSGInvViewMatrix"
        "\n\t OSGProjectionMatrix OSGModelViewMatrix OSGViewportSize OSGNormalMatrix OSGModelViewProjectionMatrix OSGStereoLeftEye OSGDrawerId"
        "\n\t OSGDrawableId OSGNodeId OSGNodeBoxMin OSGNodeBoxMax OSGNodeBoxCenter OSGNodeWorldBoxMin"
        "\n\t OSGNodeWorldBoxMax OSGNodeWorldBoxCenter OSGActiveLightsMask OSGLight0Active OSGLight1Active OSGLight2Active"
        "\n\t OSGLight3Active OSGLight4Active OSGLight5Active OSGLight6Active OSGLight7Active }",
        void, string) },
    {"setDefaultVertexShader", PyWrap(Material, setDefaultVertexShader, "Set a default vertex shader - setDefaultVertexShader()", void ) },
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

PyObject* VRPyMaterial::setShaderParameter(VRPyMaterial* self, PyObject* args) {
    auto mPtr = self->objPtr;
	if (mPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setShaderParameter, C obj is invalid"); return NULL; }
	const char* name = 0;
	PyObject* var;
    if (! PyArg_ParseTuple(args, "sO", &name, &var)) return NULL;
    if (!name) return NULL;
    string n = name ? name : "";
    if (PyLong_Check(var)) { int v; toValue(var,v); mPtr->setShaderParameter( n, v ); }
    if (PyFloat_Check(var)) { float v; toValue(var,v); mPtr->setShaderParameter( n, v ); }
    if (PyVec_Check(var, 2, 'f')) { Vec2f v; toValue(var,v); mPtr->setShaderParameter( n, v ); }
    if (PyVec_Check(var, 3, 'f')) { Vec3f v; toValue(var,v); mPtr->setShaderParameter( n, v ); }
    if (PyVec_Check(var, 4, 'f')) { Vec4f v; toValue(var,v); mPtr->setShaderParameter( n, v ); }
	Py_RETURN_TRUE;
}

PyObject* VRPyMaterial::setTexture(VRPyMaterial* self, PyObject* args) {
	if (self->objPtr == 0) { PyErr_SetString(err, "VRPyMaterial::setTexture, C obj is invalid"); return NULL; }

	int aN = pySize(args);
	if (aN <= 2) {
        PyObject* o = 0; int pos = 0;
        if (! PyArg_ParseTuple(args, "O|i", &o, &pos)) return NULL;
        if (PyUnicode_Check(o)) self->objPtr->setTexture( PyUnicode_AsUTF8(o), true, pos ); // load a file
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
                for (int j=0; j<vN; j++) buf[i*vN+j] = PyLong_AsLong(vec[j]);
            }
            self->objPtr->setTexture( &buf[0], vN, parseVec3iList(dims), false );
        }
	}

	Py_RETURN_TRUE;
}






