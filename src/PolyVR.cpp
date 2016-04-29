#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/coreDumpHandler.h"
#include "core/gui/VRGuiManager.h"
#include "core/networking/VRMainInterface.h"
#include "core/networking/VRSharedMemory.h"
#include "core/utils/VROptions.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSoundManager.h"
#include "core/objects/material/VRMaterial.h"
#include "addons/CEF/CEF.h"

#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGNode.h>
#include <GL/glut.h>


OSG_BEGIN_NAMESPACE;
using namespace std;

void printFieldContainer() {
    int N = FieldContainerFactory::the()->getNumTotalContainers();
    for (int i=0;i<N;++i) {
        FieldContainer* fc = FieldContainerFactory::the()->getContainer(i);
        if(fc == 0) continue;
        if(fc->getId() < 343) continue; // stuff created in osgInit()

        // skip prototypes
        if(fc->getType().getPrototype() == 0 || fc->getType().getPrototype() == fc  ) continue;

        //cout << "\nFC id: " << fc->getId() << flush;

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
        if (name != 0) printf("Detected living FC %s (%s) %p refcount %d ID %d\n", fc->getTypeName(), name, fc, fc->getRefCount(), fc->getId());
        else printf( "Detected living FC %s (no name) %p refcount %d ID %d\n", fc->getTypeName(), fc, fc->getRefCount(), fc->getId() );
    }
}

PolyVR::PolyVR() {}
PolyVR::~PolyVR() {
    /*monitor.reset();
    gui_mgr.reset();
    interface.reset();
    loader.reset();
    setup_mgr.reset();
    scene_mgr.reset();
    sound_mgr.reset();
    options.reset();*/
    //CEF::shutdown();
}

PolyVR* PolyVR::get() {
    static PolyVR* pvr = new PolyVR();
    return pvr;
}

void PolyVR::shutdown() {
    VRSharedMemory sm("PolyVR_System");
    int* i = sm.addObject<int>("identifier");
    *i = 0;

    auto pvr = get();
    pvr->scene_mgr->closeScene();
    pvr->scene_mgr->stopAllThreads();
    //pvr->setup_mgr->closeSetup();
    delete pvr;
    printFieldContainer();
    osgExit();
    std::exit(0);
}

void printNextOSGID(int i) {
    NodeRefPtr n = Node::create();
    cout << "next OSG ID: " << n->getId() << " at maker " << i << endl;
}

void PolyVR::setOption(string name, bool val) { options->setOption(name, val); }
void PolyVR::setOption(string name, string val) { options->setOption(name, val); }
void PolyVR::setOption(string name, int val) { options->setOption(name, val); }
void PolyVR::setOption(string name, float val) { options->setOption(name, val); }

void PolyVR::init(int argc, char **argv) {
    checkProcessesAndSockets();
    cout << "Init PolyVR\n\n";
    enableCoreDump(true);
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

    //GLUT
    glutInit(&argc, argv);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    //OSG
    ChangeList::setReadWriteDefault();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
    cout << "Init OSG\n";
    osgInit(argc,argv);

    VRSharedMemory sm("PolyVR_System");
    int* i = sm.addObject<int>("identifier");
    *i = 1;
}

void PolyVR::run() {
    while(true) {
        VRSceneManager::get()->update();

        if (VRGlobals::get()->CURRENT_FRAME == 300) {
            string app = options->getOption<string>("application");
            if (app != "") VRSceneManager::get()->loadScene(app);
        }
    }
}

void PolyVR::start(bool runit) {
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO | GLUT_STENCIL);
    else glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);

    cout << "Init Modules\n";
    sound_mgr = shared_ptr<VRSoundManager>(&VRSoundManager::get());
    setup_mgr = shared_ptr<VRSetupManager>(VRSetupManager::get());
    interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
    scene_mgr = shared_ptr<VRSceneManager>(VRSceneManager::get());
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());

    //string app = options->getOption<string>("application");
    //if (app != "") VRSceneManager::get()->loadScene(app);

    if (runit) run();
}

void PolyVR::startTestScene(Node* n) {
    start(false);
    cout << "start test scene " << n << endl;
    VRSceneManager::get()->newScene("test");
    VRSceneManager::getCurrent()->getRoot()->find("Headlight")->addChild(n);
    VRGuiManager::get()->wakeWindow();
    run();
}

void PolyVR::checkProcessesAndSockets() { // TODO!!
    VRSharedMemory sm("PolyVR_System");// check for running PolyVR process
    int i = sm.getObject<int>("identifier");
    if (i) {
        cout << "Error: A PolyVR is allready running!\n";
    }
}


OSG_END_NAMESPACE;
