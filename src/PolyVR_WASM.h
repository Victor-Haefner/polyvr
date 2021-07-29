#ifndef POLYVR_WASM_H_INCLUDED
#define POLYVR_WASM_H_INCLUDED


#include <emscripten.h>

#include <OpenSG/OSGPNGImageFileType.h>
#include <OpenSG/OSGJPGImageFileType.h>
#include <OpenSG/OSGNFIOSceneFileType.h>
#include <OpenSG/OSGTypedGeoVectorProperty.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>

// init OSB singletons for wasm
#include <OpenSG/OSGOSBChunkBlockElement.h>
#include <OpenSG/OSGOSBChunkMaterialElement.h>
#include <OpenSG/OSGOSBCubeTextureChunkElement.h>
#include <OpenSG/OSGOSBGenericElement.h>
#include <OpenSG/OSGOSBGenericAttElement.h>
#include <OpenSG/OSGOSBGeometryElement.h>
#include <OpenSG/OSGOSBImageElement.h>
#include <OpenSG/OSGOSBMaterialPoolElement.h>
#include <OpenSG/OSGOSBNameElement.h>
#include <OpenSG/OSGOSBNodeElement.h>
#include <OpenSG/OSGOSBRootElement.h>
#include <OpenSG/OSGOSBShaderParameterBoolElement.h>
#include <OpenSG/OSGOSBShaderParameterIntElement.h>
#include <OpenSG/OSGOSBShaderParameterMatrixElement.h>
#include <OpenSG/OSGOSBShaderParameterMIntElement.h>
#include <OpenSG/OSGOSBShaderParameterMRealElement.h>
#include <OpenSG/OSGOSBShaderParameterMVec2fElement.h>
#include <OpenSG/OSGOSBShaderParameterMVec3fElement.h>
#include <OpenSG/OSGOSBShaderParameterMVec4fElement.h>
#include <OpenSG/OSGOSBShaderParameterRealElement.h>
#include <OpenSG/OSGOSBShaderParameterVec2fElement.h>
#include <OpenSG/OSGOSBShaderParameterVec3fElement.h>
#include <OpenSG/OSGOSBShaderParameterVec4fElement.h>
#include <OpenSG/OSGOSBSHLChunkElement.h>
#include <OpenSG/OSGOSBTextureChunkElement.h>
#include <OpenSG/OSGOSBVoidPAttachmentElement.h>

#include <OpenSG/OSGOSBGeoPropertyConversionElement.h>
#include <OpenSG/OSGOSBTypedGeoIntegralPropertyElement.h>
#include <OpenSG/OSGOSBTypedGeoVectorPropertyElement.h>

OSG_BEGIN_NAMESPACE;

typedef const char* CSTR;
EMSCRIPTEN_KEEPALIVE void PolyVR_shutdown() { PolyVR::shutdown(); }
EMSCRIPTEN_KEEPALIVE void PolyVR_reloadScene() { VRSceneManager::get()->reloadScene(); }
EMSCRIPTEN_KEEPALIVE void PolyVR_showStats() { VRSetup::getCurrent()->toggleViewStats(0); }
EMSCRIPTEN_KEEPALIVE int PolyVR_getNScripts() { auto s = VRScene::getCurrent(); return s ? s->getNScripts() : 0; }

VRScriptPtr getScript(CSTR name) {
    string Name = string(name);
    auto s = VRScene::getCurrent();
    return s ? s->getScript(Name) : 0;
}

EMSCRIPTEN_KEEPALIVE void PolyVR_triggerScript(CSTR name, CSTR* params, int N) {
    string Name = string(name);
    vector<string> sparams;
    for (int i=0; i<N; i++) sparams.push_back(string(params[i]));
    auto s = VRScene::getCurrent();
    if (s) s->triggerScript(Name, sparams);
}

EMSCRIPTEN_KEEPALIVE CSTR PolyVR_getIthScriptName(int i) {
    auto s = VRScene::getCurrent();
    static string name = "";
    name = s ? s->getIthScriptName(i) : "";
    return name.c_str();
}

EMSCRIPTEN_KEEPALIVE CSTR PolyVR_getScriptCore(CSTR name) {
    auto script = getScript(name);
    static string core = "";
    core = script ? script->getScript() : "";
    return core.c_str();
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptCore(CSTR name, CSTR core) {
    string Name = string(name);
    string Core = string(core);
    auto s = VRScene::getCurrent();
    if (s) s->updateScript(name, core);
}

EMSCRIPTEN_KEEPALIVE CSTR PolyVR_getScriptType(CSTR name) {
    auto script = getScript(name);
    static string type;
    type = script ? script->getType() : "";
    return type.c_str();
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptType(CSTR name, CSTR type) {
    string Type = string(type);
    auto script = getScript(name);
    if (script) script->setType(Type);
}

EMSCRIPTEN_KEEPALIVE int PolyVR_getNScriptTriggers(CSTR name) {
    auto script = getScript(name);
    return script ? script->getTriggers().size() : 0;
}

VRScript::trigPtr getIthTrig(VRScriptPtr script, int i) {
    auto trigs = script->getTriggers();
    auto it = trigs.begin();
    advance(it, i);
    return *it;
}

VRScript::argPtr getIthArg(VRScriptPtr script, int i) {
    auto args = script->getArguments();
    auto it = args.begin();
    advance(it, i);
    return *it;
}

EMSCRIPTEN_KEEPALIVE CSTR PolyVR_getScriptIthTrigger(CSTR name, int i) {
    auto script = getScript(name);
    static string data = "";
    data = "";
    if (!script) return data.c_str();

    auto t = getIthTrig(script, i);
    string key = toString(t->key);
    if (t->dev == "keyboard" && t->key > 32 && t->key < 127) {
        char kc = t->key;
        key = kc;
    }

    data = t->trigger;
    data += "|" + t->param;
    data += "|" + t->dev;
    data += "|" + key;
    data += "|" + t->state;
    return data.c_str();
}

EMSCRIPTEN_KEEPALIVE int PolyVR_getNScriptArguments(CSTR name) {
    auto script = getScript(name);
    return script ? script->getArguments().size() : 0;
}

EMSCRIPTEN_KEEPALIVE CSTR PolyVR_getScriptIthArgument(CSTR name, int i) {
    auto script = getScript(name);
    static string data = "";
    data = "";
    if (!script) return data.c_str();

    auto a = getIthArg(script, i);
    data = a->getName();
    data += "|" + a->val;
    return data.c_str();
}

EMSCRIPTEN_KEEPALIVE void PolyVR_addScriptArgument(CSTR name) {
    auto script = getScript(name);
    if (script) script->addArgument();
}

EMSCRIPTEN_KEEPALIVE void PolyVR_addScriptTrigger(CSTR name) {
    auto script = getScript(name);
    if (script) script->addTrigger();
}

EMSCRIPTEN_KEEPALIVE void PolyVR_remScriptIthArgument(CSTR name, int i) {
    auto script = getScript(name);
    if (!script) return;
    auto a = getIthArg(script, i);
    script->remArgument(a->getName());
}

EMSCRIPTEN_KEEPALIVE void PolyVR_remScriptIthTrigger(CSTR name, int i) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->remTrigger(t->getName());
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthTriggerType(CSTR name, int i, CSTR type) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->changeTrigger(t->getName(), string(type));
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthTriggerParam(CSTR name, int i, CSTR param) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->changeTrigParams(t->getName(), string(param));
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthTriggerDevice(CSTR name, int i, CSTR device) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->changeTrigDev(t->getName(), string(device));
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthTriggerKey(CSTR name, int i, int key) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->changeTrigKey(t->getName(), key);
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthTriggerState(CSTR name, int i, CSTR state) {
    auto script = getScript(name);
    if (!script) return;
    auto t = getIthTrig(script, i);
    script->changeTrigState(t->getName(), string(state));
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthArgumentVar(CSTR name, int i, CSTR var) {
    auto script = getScript(name);
    if (!script) return;
    auto a = getIthArg(script, i);
    script->changeArgName(a->getName(), string(var));
}

EMSCRIPTEN_KEEPALIVE void PolyVR_setScriptIthArgumentVal(CSTR name, int i, CSTR val) {
    auto script = getScript(name);
    if (!script) return;
    auto a = getIthArg(script, i);
    script->changeArgValue(a->getName(), string(val));
}

OSG_END_NAMESPACE;

void initOSGImporter() {
    // init image formats
    cout << "  init image formats" << endl;
	PNGImageFileType::the();
	JPGImageFileType::the();

    // init data formats
    cout << "  init data formats" << endl;
	NFIOSceneFileType::the();

	OSBElementFactory::the()->registerDefault(new OSBElementCreator<OSBGenericElement>());

	OSBElementFactory::the()->registerElement("ChunkBlock", new OSBElementCreator<OSBChunkBlockElement>);
    OSBElementFactory::the()->registerElement("ChunkMaterial", new OSBElementCreator<OSBChunkMaterialElement>);
    OSBElementFactory::the()->registerElement("SimpleMaterial", new OSBElementCreator<OSBChunkMaterialElement>);
    OSBElementFactory::the()->registerElement("CubeTextureChunk", new OSBElementCreator<OSBCubeTextureChunkElement>);
    OSBElementFactory::the()->registerElement("GenericAtt", new OSBElementCreator<OSBGenericAttElement>);
    OSBElementFactory::the()->registerElement("Geometry", new OSBElementCreator<OSBGeometryElement>);
    OSBElementFactory::the()->registerElement("Image", new OSBElementCreator<OSBImageElement>);
    OSBElementFactory::the()->registerElement("MaterialPool", new OSBElementCreator<OSBMaterialPoolElement>);
    OSBElementFactory::the()->registerElement("Name", new OSBElementCreator<OSBNameElement>);
    OSBElementFactory::the()->registerElement("Node", new OSBElementCreator<OSBNodeElement>);
    OSBElementFactory::the()->registerElement("RootElement", new OSBElementCreator<OSBRootElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterBool", new OSBElementCreator<OSBShaderParameterBoolElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterInt", new OSBElementCreator<OSBShaderParameterIntElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMatrix", new OSBElementCreator<OSBShaderParameterMatrixElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMInt", new OSBElementCreator<OSBShaderParameterMIntElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMReal", new OSBElementCreator<OSBShaderParameterMRealElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMVec2f", new OSBElementCreator<OSBShaderParameterMVec2fElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMVec3f", new OSBElementCreator<OSBShaderParameterMVec3fElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterMVec4f", new OSBElementCreator<OSBShaderParameterMVec4fElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterReal", new OSBElementCreator<OSBShaderParameterRealElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterVec2f", new OSBElementCreator<OSBShaderParameterVec2fElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterVec3f", new OSBElementCreator<OSBShaderParameterVec3fElement>);
    OSBElementFactory::the()->registerElement("ShaderParameterVec4f", new OSBElementCreator<OSBShaderParameterVec4fElement>);
    OSBElementFactory::the()->registerElement("SHLChunk", new OSBElementCreator<OSBSHLChunkElement>);
    OSBElementFactory::the()->registerElement("TextureChunk", new OSBElementCreator<OSBTextureChunkElement>);
    OSBElementFactory::the()->registerElement("VoidPAttachment", new OSBElementCreator<OSBVoidPAttachmentElement>);

    OSBElementFactory::the()->registerElement("GeoPositions2s", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt2sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPositions3s", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt3sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPositions4s", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt4sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPositions2f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt2fProperty>>);
    OSBElementFactory::the()->registerElement("GeoPositions3f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoPositions4f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt4fProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPositions2d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt2dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPositions3d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt3dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPositions4d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoPnt4dProperty>>);
    OSBElementFactory::the()->registerElement("GeoNormals3s", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec3sProperty>>);
    OSBElementFactory::the()->registerElement("GeoNormals3f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoNormals3b", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec3bProperty>>);
    OSBElementFactory::the()->registerElement("GeoColors3f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoColor3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoColors4f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoColor4fProperty>>);
    OSBElementFactory::the()->registerElement("GeoColors3ub", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoColor3ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoColors4ub", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoColor4ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoTexCoords1f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec1fProperty>>);
    OSBElementFactory::the()->registerElement("GeoTexCoords2f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec2fProperty>>);
    OSBElementFactory::the()->registerElement("GeoTexCoords3f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoTexCoords4f", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec4fProperty>>);
    //OSBElementFactory::the()->registerElement("GeoTexCoords1d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec1dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoTexCoords2d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec2dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoTexCoords3d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec3dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoTexCoords4d", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoVec4dProperty>>);
    OSBElementFactory::the()->registerElement("GeoPTypesUI8", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt8Property>>);
    OSBElementFactory::the()->registerElement("GeoPTypesUI16", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt16Property>>);
    OSBElementFactory::the()->registerElement("GeoPTypesUI32", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt32Property>>);
    OSBElementFactory::the()->registerElement("GeoPLengthsUI8", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt8Property>>);
    OSBElementFactory::the()->registerElement("GeoPLengthsUI16", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt16Property>>);
    OSBElementFactory::the()->registerElement("GeoPLengthsUI32", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt32Property>>);
    OSBElementFactory::the()->registerElement("GeoIndicesUI8", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt8Property>>);
    OSBElementFactory::the()->registerElement("GeoIndicesUI16", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt16Property>>);
    OSBElementFactory::the()->registerElement("GeoIndicesUI32", new OSBElementCreator<OSBGeoPropertyConversionElement<GeoUInt32Property>>);

    OSBElementFactory::the()->registerElement("GeoUInt8Property", new OSBElementCreator<OSBTypedGeoIntegralPropertyElement<GeoUInt8Property>>);
    OSBElementFactory::the()->registerElement("GeoUInt16Property", new OSBElementCreator<OSBTypedGeoIntegralPropertyElement<GeoUInt16Property>>);
    OSBElementFactory::the()->registerElement("GeoUInt32Property", new OSBElementCreator<OSBTypedGeoIntegralPropertyElement<GeoUInt32Property>>);

    OSBElementFactory::the()->registerElement("GeoPnt1ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1bProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2bProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3bProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4bProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1usProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2usProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3usProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4usProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4sProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt1fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1fProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt2fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2fProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt3fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoPnt4fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4fProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPnt1dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt1dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPnt2dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt2dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPnt3dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt3dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoPnt4dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoPnt4dProperty>>);

    OSBElementFactory::the()->registerElement("GeoVec1ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1bProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2bProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3bProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4bProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4bProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1usProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2usProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3usProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4usProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4usProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1sProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2sProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3sProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4sProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4sProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4NbProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4NbProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4NusProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4NusProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4NsProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4NsProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec1fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1fProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec2fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2fProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec3fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoVec4fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4fProperty>>);
    //OSBElementFactory::the()->registerElement("GeoVec1dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec1dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoVec2dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec2dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoVec3dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec3dProperty>>);
    //OSBElementFactory::the()->registerElement("GeoVec4dProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoVec4dProperty>>);

    OSBElementFactory::the()->registerElement("GeoColor3ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor3ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoColor4ubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor4ubProperty>>);
    OSBElementFactory::the()->registerElement("GeoColor3NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor3NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoColor4NubProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor4NubProperty>>);
    OSBElementFactory::the()->registerElement("GeoColor3fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor3fProperty>>);
    OSBElementFactory::the()->registerElement("GeoColor4fProperty", new OSBElementCreator<OSBTypedGeoVectorPropertyElement<GeoColor4fProperty>>);
}

OSG_BEGIN_NAMESPACE;
void glutUpdate() {
    PolyVR::get()->update();
}
OSG_END_NAMESPACE;

#endif // POLYVR_WASM_H_INCLUDED
