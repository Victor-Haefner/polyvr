#ifdef __EMSCRIPTEN__
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include <OpenSG/OSGGL.h>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGPrimeMaterial.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGSceneFileHandler.h>

#ifdef __EMSCRIPTEN__
#include <OpenSG/OSGPNGImageFileType.h>
#include <OpenSG/OSGJPGImageFileType.h>

#include <OpenSG/OSGNFIOSceneFileType.h>
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
#include <OpenSG/OSGTypedGeoVectorProperty.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>
#endif

#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/coreDumpHandler.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiSignals.h"
#endif
#include "core/networking/VRMainInterface.h"
#ifndef WITHOUT_SHARED_MEMORY
#include "core/networking/VRSharedMemory.h"
#endif
#include "core/utils/VROptions.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRSceneLoader.h"
#ifndef WITHOUT_AV
#include "core/scene/sound/VRSoundManager.h"
#endif
#include "core/objects/object/VRObject.h"
#include "core/objects/VRTransform.h"
#include "core/setup/VRSetup.h"

#ifdef WASM
#include <emscripten.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <termios.h>
#endif

#ifdef _WIN32
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // tells an optimus system to run PolyVR in high performance mode
}
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

PolyVR* pvr = 0;

void printFieldContainer(int maxID = -1) {
    int N = FieldContainerFactory::the()->getNumTotalContainers();
    for (int i=0;i<N;++i) {
        FieldContainer* fc = FieldContainerFactory::the()->getContainer(i);
        if(fc == 0) continue;
        int fcID = fc->getId();
        if(fcID <= 358) continue; // stuff created in osgInit()
        if(fcID > maxID && maxID > 0) break; // stop

        // skip prototypes
        if(fc->getType().getPrototype() == 0 || fc->getType().getPrototype() == fc  ) continue;

        //cout << "\nFC id: " << fcID << flush;

        AttachmentContainer* ac = dynamic_cast<AttachmentContainer*>(fc);
        if (ac == 0) {
            Attachment* a = dynamic_cast<Attachment*>(fc);
            if (a != 0) {
                FieldContainer* dad = 0;
                if (a->getMFParents()->size() > 0) dad = a->getParents(0);
                ac = dynamic_cast<AttachmentContainer*>(dad);
            }
        }

        const Char8* name = getName(ac);
        if (name != 0) printf("Detected living FC %s (%s) %p refcount %d ID %d\n", fc->getTypeName(), name, fc, fc->getRefCount(), fcID);
        else printf( "Detected living FC %s (no name) %p refcount %d ID %d\n", fc->getTypeName(), fc, fc->getRefCount(), fcID );
    }
}

PolyVR::PolyVR() {}

PolyVR::~PolyVR() {
    cout << "PolyVR::~PolyVR" << endl;
#ifndef WITHOUT_SHARED_MEMORY
    try {
        VRSharedMemory sm("PolyVR_System");
        int* i = sm.addObject<int>("identifier");
        *i = 0;
    }
    catch (...) {}
#endif

    pvr = 0;

#ifndef WITHOUT_GTK
    VRGuiSignals::get()->clear();
#endif
    scene_mgr->closeScene();
    VRSetup::getCurrent()->stopWindows();
    scene_mgr->stopAllThreads();
    setup_mgr->closeSetup();


    monitor.reset();
    gui_mgr.reset();
    main_interface.reset();
    loader.reset();
    setup_mgr.reset();
    scene_mgr.reset();
    sound_mgr.reset();
    options.reset();

    cout << " terminated all polyvr modules" << endl;
#ifndef WASM
    printFieldContainer();
#endif

    cout << "call osgExit" << endl;
    osgExit();

    /*#ifdef WASM
        cout << "call emscripten_force_exit" << endl;
        emscripten_force_exit(0);
    #else
        cout << "call exit" << endl;
        std::exit(0);
    #endif*/
}

shared_ptr<PolyVR> PolyVR::create() {
    if (!pvr) {
        pvr = new PolyVR();
        return shared_ptr<PolyVR>(pvr);
    } else return 0;
}

PolyVR* PolyVR::get() { return pvr; }

#ifdef WASM
typedef const char* CSTR;
EMSCRIPTEN_KEEPALIVE void PolyVR_shutdown() { PolyVR::shutdown(); }
EMSCRIPTEN_KEEPALIVE void PolyVR_reloadScene() { VRSceneManager::get()->reloadScene(); }
EMSCRIPTEN_KEEPALIVE void PolyVR_showStats() { VRSetup::getCurrent()->toggleViewStats(0); }
EMSCRIPTEN_KEEPALIVE void PolyVR_triggerScript(const char* name, CSTR* params, int N) {
    vector<string> sparams;
    for (int i=0; i<N; i++) sparams.push_back(string(params[i]));
    VRScene::getCurrent()->triggerScript(string(name), sparams);
}
#endif

void PolyVR::shutdown() {
    if (!pvr) return;
    cout << "PolyVR::shutdown" << endl;
    pvr->doLoop = false;
}

void printNextOSGID(int i) {
    NodeRefPtr n = Node::create();
    cout << "next OSG ID: " << n->getId() << " at maker " << i << endl;
}

#ifdef WASM
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
#endif

void printOSGImportCapabilities() {
    auto h = SceneFileHandler::the();

    std::list<const Char8*> suffixList;
    h->getSuffixList(suffixList, SceneFileType::OSG_READ_SUPPORTED);
    cout << "OSG read formats:" << endl;
    for (auto s : suffixList) cout << " " << s;
    cout << endl;

    h->getSuffixList(suffixList, SceneFileType::OSG_WRITE_SUPPORTED);
    cout << "OSG write formats:" << endl;
    for (auto s : suffixList) cout << " " << s;
    cout << endl;
}

void testGLCapabilities() {
    cout << "Check OpenGL capabilities:" << endl;
    cout << " OpenGL vendor: " << VRRenderManager::getGLVendor() << endl;
    cout << " OpenGL version: " << VRRenderManager::getGLVersion() << endl;
    cout << " GLSL version: " << VRRenderManager::getGLSLVersion() << endl;
    cout << " has geometry shader: " << VRRenderManager::hasGeomShader() << endl;
    cout << " has tesselation shader: " << VRRenderManager::hasTessShader() << endl;
}

void PolyVR::init(int argc, char **argv) {
    cout << "Init PolyVR" << endl << endl;
    initTime();

#ifdef WASM
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

    //OSG
    cout << " init OSG" << endl;
    ChangeList::setReadWriteDefault();
    osgInit(argc,argv);
    initOSGImporter();
    cout << "  ..done" << endl;

    PrimeMaterialRecPtr pMat = OSG::getDefaultMaterial();
    OSG::setName(pMat, "default_material");
#else
    enableCoreDump(true);
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);
    checkProcessesAndSockets();

    //OSG
    cout << " init OSG" << endl;
    ChangeList::setReadWriteDefault();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
    osgInit(argc,argv);
    cout << "  ..done" << endl << endl;

    printOSGImportCapabilities();

#ifndef WITHOUT_SHARED_MEMORY
    try {
        VRSharedMemory sm("PolyVR_System");
        int* i = sm.addObject<int>("identifier");
        *i = 1;
    } catch(...) {}
    cout << endl;
#endif

    PrimeMaterialRecPtr pMat = OSG::getDefaultMaterial();
    OSG::setName(pMat, "default_material");
#endif

#ifndef WITHOUT_AV
    sound_mgr = VRSoundManager::get();
#endif

    setup_mgr = VRSetupManager::create();
#ifdef WASM
    VRSetupManager::get()->load("Browser", "Browser.xml");
#endif

    scene_mgr = VRSceneManager::create();
	main_interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());

#ifndef WITHOUT_GTK
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
#endif

    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());

    //string app = options->getOption<string>("application");
    //if (app != "") VRSceneManager::get()->loadScene(app);
    removeFile("setup/.startup"); // remove startup failsafe

    testGLCapabilities();
}

void PolyVR::update() {
    VRSceneManager::get()->update();

    if (VRGlobals::CURRENT_FRAME == 300) {
        string app = options->getOption<string>("application");
        string dcy = options->getOption<string>("decryption");
        string key;
        if (startsWith(dcy, "key:")) key = subString(dcy, 4, dcy.size()-4);
        if (startsWith(dcy, "serial:")) key = "123"; // TODO: access serial connection to retreive key
        if (app != "") VRSceneManager::get()->loadScene(app, false, key);
    }
}

#ifdef WASM
void glutUpdate() {
    PolyVR::get()->update();
}
#endif

void PolyVR::run() {
    cout << endl << "Start main loop" << endl << endl;
#ifndef WASM
    doLoop = true;
    while(doLoop) update(); // default
#else
    // WASM needs to control the main loop
    glutIdleFunc(glutPostRedisplay);
    glutDisplayFunc(glutUpdate);
    glutMainLoop();
#endif
}

void PolyVR::startTestScene(OSGObjectPtr n, const Vec3d& camPos) {
    cout << "start test scene " << n << endl;
    VRSceneManager::get()->newScene("test");
    VRScene::getCurrent()->getRoot()->find("light")->addChild(n);

    VRTransformPtr cam = dynamic_pointer_cast<VRTransform>( VRScene::getCurrent()->get("Default") );
    cam->setFrom(Vec3d(camPos));

#ifndef WITHOUT_GTK
    VRGuiManager::get()->wakeWindow();
#endif
    run();
}

string createTimeStamp() {
    time_t _tm =time(NULL );
    struct tm * curtime = localtime ( &_tm );
    return asctime(curtime);
}

#ifndef _WIN32
char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0) perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0) perror ("tcsetattr ~ICANON");
        return (buf);
}
#else
TCHAR getch() {
	return getchar();
}
#endif

void PolyVR::checkProcessesAndSockets() {
    // check for failed startup
    bool dofailcheck = VROptions::get()->getOption<bool>("dofailcheck");
    string timestamp;
    ifstream f1("setup/.startup"); getline(f1,timestamp); f1.close();
    if (timestamp != "" && dofailcheck) {
        bool handling_bad_startup = true;
        cout << "Warning! a previously failed startup has been detected that occurred at " << timestamp << endl;
        cout << "Hints: " << endl;
        cout << " - In some instances a corruption of the x server can prevent the creation of the GL context.. " << endl;
        cout << "     -> restart your system" << endl;
        do {
            cout << "\n\tChoose on of the following options to proceed:" << endl;
            cout << "\t1) resume startup" << endl;
            cout << "\t2) reset to default hardware setup" << endl;
            char c = getch();
            //char c = cin.get();
            if (c == '1') { cout << "\t\tresuming startup now..\n" << endl; break; }
            if (c == '2') { cout << "\t\tremoving local setup config 'setup/.local'" << endl; removeFile("setup/.local"); }
        } while (handling_bad_startup);
    }

    // store startup tmp file, removed after successful startup
    timestamp = createTimeStamp();
    ofstream f("setup/.startup"); f.write(timestamp.c_str(), timestamp.size()); f.close();

#ifndef WITHOUT_SHARED_MEMORY
    // TODO!!
    try {
        VRSharedMemory sm("PolyVR_System");// check for running PolyVR process
        int i = sm.getObject<int>("identifier");
        if (i) cout << "Error: A PolyVR instance is already running!\n";
    } catch(...) {}
#endif
    cout << endl;
}


OSG_END_NAMESPACE;
