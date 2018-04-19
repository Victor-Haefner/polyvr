#include "VRPyTextureRenderer.h"
#include "VRPyCamera.h"
#include "VRPyMaterial.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRTextureRenderer::CHANNEL& e) {
    return toValue( PyString_AsString(obj) , e);
}

simpleVRPyType(TextureRenderer, New_VRObjects_ptr);

PyMethodDef VRPyTextureRenderer::methods[] = {
    {"setup", PyWrapOpt(TextureRenderer, setup, "Setup texture renderer, cam, width, height", "0", void, VRCameraPtr, int, int, bool) },
    {"getMaterial", PyWrap(TextureRenderer, getMaterial, "Get the material with the rendering", VRMaterialPtr) },
    {"setActive", PyWrap(TextureRenderer, setActive, "Activate and deactivate the texture rendering", void, bool) },
    {"renderOnce", PyWrap(TextureRenderer, renderOnce, "Render once", VRTexturePtr, VRTextureRenderer::CHANNEL) },
    {"getCamera", PyWrap(TextureRenderer, getCamera, "Get camera", VRCameraPtr) },
    {NULL}  /* Sentinel */
};



