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

#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/scripting/VRScript.h"
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

#ifndef _WIN32
#include <unistd.h>
#include <termios.h>
#endif

#ifdef _WIN32
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // tells an optimus system to run PolyVR in high performance mode
}
#endif

using namespace OSG;
using namespace std;

#ifdef __EMSCRIPTEN__
#include <PolyVR_WASM.h>
#endif

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
    if (scene_mgr) scene_mgr->closeScene();
    if (auto setup = VRSetup::getCurrent()) setup->stopWindows();
    if (scene_mgr) scene_mgr->stopAllThreads();
    if (setup_mgr) setup_mgr->closeSetup();


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

void PolyVR::shutdown() {
    if (!pvr) return;
    cout << "PolyVR::shutdown" << endl;
    pvr->doLoop = false;
}

void printNextOSGID(int i) {
    NodeRefPtr n = Node::create();
    cout << "next OSG ID: " << n->getId() << " at maker " << i << endl;
}

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

void PolyVR::initEnvironment() {
    initTime();
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

#ifndef WASM
    enableCoreDump(true);
    checkProcessesAndSockets();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
#endif
}

void PolyVR::initOpenSG() {
    cout << " init OSG" << endl;
    ChangeList::setReadWriteDefault();
    osgInit(argc,argv);
    cout << "  ..done" << endl << endl;

#ifdef WASM
    initOSGImporter();
#else
    printOSGImportCapabilities();
#endif

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
}

void PolyVR::initManagers() {
#ifndef WITHOUT_AV
    sound_mgr = VRSoundManager::get();
#endif

    setup_mgr = VRSetupManager::create();
    scene_mgr = VRSceneManager::create();
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());
}

void PolyVR::initUI() {
#ifdef WASM
    VRSetupManager::get()->load("Browser", "Browser.xml");
    cout << " Browser setup loaded!" << endl;
#else
	main_interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
#endif

#ifndef WITHOUT_GTK
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
    gui_mgr->updateSystemInfo();
#endif

    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());
}

void PolyVR::initFinalize() {
    removeFile("setup/.startup"); // remove startup failsafe
    testGLCapabilities();
    initiated = true;
}

void PolyVR::init(int argc, char **argv) {
    this->argc = argc;
    this->argv = argv;

    cout << "Init PolyVR" << endl << endl;
    initQueue.push_back(VRUpdateCb::create( "init environment", bind(&PolyVR::initEnvironment, this)));
    initQueue.push_back(VRUpdateCb::create( "init opensg", bind(&PolyVR::initOpenSG, this)));
    initQueue.push_back(VRUpdateCb::create( "init managers", bind(&PolyVR::initManagers, this)));
    initQueue.push_back(VRUpdateCb::create( "init ui", bind(&PolyVR::initUI, this)));
    initQueue.push_back(VRUpdateCb::create( "init finalize", bind(&PolyVR::initFinalize, this)));
}

void PolyVR::update() {
    if (initQueueItr != initQueue.end()) {
        VRTimer t;
        t.start();
        VRUpdateCbPtr cp = *initQueueItr;
        cout << "> init step: " << cp->name << endl;
        (*cp)();
        cout << "> init step: " << cp->name << " done after " << t.stop() << endl;
        initQueueItr++;
	static int i = 0;
	float d = float(i)/(initQueue.size()-1);
	i++;
        VRSetup::sendToBrowser("setProgress|"+toString(d)+"|"+cp->name);
        return;
    }

    VRSceneManager::get()->update();

    int appInitFrame = 10;
#ifdef _WIN32
    int appInitFrame = 300;
#endif
    if (VRGlobals::CURRENT_FRAME == appInitFrame) {
        string app = options->getOption<string>("application");
        string dcy = options->getOption<string>("decryption");
        string key;
        if (startsWith(dcy, "key:")) key = subString(dcy, 4, dcy.size()-4);
        if (startsWith(dcy, "serial:")) key = "123"; // TODO: access serial connection to retreive key
        if (app != "") VRSceneManager::get()->loadScene(app, false, key);
    }
}

void PolyVR::run() {
    //if (!initiated) return;
    cout << endl << "Start main loop" << endl << endl;
    initQueueItr = initQueue.begin();
#ifndef WASM
    doLoop = true;
    while (doLoop) update();
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
