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
    {"setReadback", PyWrap(TextureRenderer, setReadback, "Set readback (RGB, depth), necessary for using the texture beyond rendering", void, bool, bool) },
    {"getMaterial", PyWrap(TextureRenderer, getMaterial, "Get the material with the rendering", VRMaterialPtr) },
    {"copyMaterial", PyWrap(TextureRenderer, copyMaterial, "Get a material copy with the rendering textures", VRMaterialPtr) },
    {"setActive", PyWrap(TextureRenderer, setActive, "Activate and deactivate the texture rendering", void, bool) },
    {"renderOnce", PyWrapOpt(TextureRenderer, renderOnce, "Render once", "RENDER", VRTexturePtr, VRTextureRenderer::CHANNEL) },
    {"getCamera", PyWrap(TextureRenderer, getCamera, "Get camera", VRCameraPtr) },
    {"getResolution", PyWrap(TextureRenderer, getResolution, "Get fbo resolution passed in setup call", Vec2i) },
    {"createCubeMaps", PyWrap(TextureRenderer, createCubeMaps, "Create cube maps, front, back, left, right, up, down", vector<VRTexturePtr>, VRTransformPtr) },
    {"test", PyWrap(TextureRenderer, test, "test", void) },
    {"startServer", PyWrap(TextureRenderer, startServer, "Start streaming server", string, int) },
    {"stopServer", PyWrap(TextureRenderer, stopServer, "Stop streaming server", void) },
    {"exportDepthImage", PyWrap(TextureRenderer, exportDepthImage, "Export depth to file", void, string) },
    {NULL}  /* Sentinel */
};
