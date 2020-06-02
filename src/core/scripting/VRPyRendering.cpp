#include "VRPyRendering.h"
#include "VRPyImage.h"
#include "VRPyBaseT.h"

#include "core/scene/VRScene.h"

using namespace OSG;

simpleVRPyType(Rendering, 0);
simpleVRPyType(RenderStudio, 0);

PyMethodDef VRPyRendering::methods[] = {
    {"addStage", PyWrap(Rendering, addStage, "Add a stage to the rendering pipeline - addStage( str name, str insert_point )\n\ta common insertion point is the 'shading' stage", void, string, string ) },
    {"setStageShader", PyWrap(Rendering, setStageShader, "Set the stage shader by name - setStageShader( str stage, str VPpath, str FPpath, bool deferred )", void, string, string, string, bool ) },
    {"setStageActive", PyWrap(Rendering, setStageActive, "Activate/deactivate the stage - setStageActive( str stage, bool deferred, bool layer )", void, string, bool, bool ) },
    {"addStageBuffer", PyWrap(Rendering, addStageBuffer, "Add additional render buffer - id addStageBuffer( str stage, pixel_format, pixel_type )\r\tpixel_format can be 'RGB' or 'RGBA', pixel_type can be 'UINT8', 'UINT16', 'UINT32', 'FLOAT32', ... ", int, string, int, int ) },
    {"setStageParameter", PyWrap(Rendering, setStageParameter, "Set shader parameter of stage - setStageParameter( str stage, str var, int val )", void, string, string, int ) },
    {"setStageTexture", PyWrap(Rendering, setStageTexture, "Set stage material texture - setStageTexture( str stage, texture, int unit, str mag, str min )", void, string, VRTexturePtr, int, int, int ) },
    {"setDeferredShading", PyWrap(Rendering, setDeferredShading, "Toggle deferred shading", void, bool ) },
    {"reloadStageShaders", PyWrap(Rendering, reloadStageShaders, "Reload shaders of rendering stages", void ) },
    {"setFogParams", PyWrap(Rendering, setFogParams, "Set fog", void, Color4f, Color4f ) },
    {"getMultisampling", PyWrap(Rendering, getMultisampling, "Get MSAA flag", bool ) },
    {"setMultisampling", PyWrap(Rendering, setMultisampling, "Set MSAA", void, bool) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRenderStudio::methods[] = {
    {"getRoot", PyWrap(RenderStudio, getRoot, "Get root object", VRObjectPtr ) },
    {NULL}  /* Sentinel */
};
