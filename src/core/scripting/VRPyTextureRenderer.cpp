#include "VRPyTextureRenderer.h"
#include "VRPyCamera.h"
#include "VRPyMaterial.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRTextureRenderer::CHANNEL& e) {
    string str = PyString_AsString(obj);
    return toValue( str , e);
}

simpleVRPyType(TextureRenderer, New_VRObjects_ptr);

PyMethodDef VRPyTextureRenderer::methods[] = {
    {"setup", PyWrapOpt(TextureRenderer, setup, "Setup texture renderer, cam, width, height", "0", void, VRCameraPtr, int, int, bool) },
    {"getMaterial", PyWrap(TextureRenderer, getMaterial, "Get the material with the rendering", VRMaterialPtr) },
    {"setActive", PyWrap(TextureRenderer, setActive, "Activate and deactivate the texture rendering", void, bool) },
    {"renderOnce", PyWrapOpt(TextureRenderer, renderOnce, "Render once", "RENDER", VRTexturePtr, VRTextureRenderer::CHANNEL) },
    {"getCamera", PyWrap(TextureRenderer, getCamera, "Get camera", VRCameraPtr) },
    {"createCubeMaps", PyWrap(TextureRenderer, createCubeMaps, "Create cube maps, front, back, left, right, up, down", vector<VRTexturePtr>, VRTransformPtr) },
    {"test", PyWrap(TextureRenderer, test, "test", void) },
    {NULL}  /* Sentinel */
};



