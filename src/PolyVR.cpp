#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/coreDumpHandler.h"
#include "core/gui/VRGuiManager.h"
#include "core/networking/VRMainInterface.h"
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
        else printf( "Detected living FC %s %p refcount %d ID %d\n", fc->getTypeName(), fc, fc->getRefCount(), fc->getId() );
    }
}

PolyVR::PolyVR() {}
PolyVR::~PolyVR() {}

PolyVR& PolyVR::get() {
    static PolyVR pvr;
    return pvr;
}

void PolyVR::start() { while(true) VRSceneManager::get()->update(); }
void PolyVR::exit() {
    options = 0;
    scene_mgr = 0;
    setup_mgr = 0;
    monitor = 0;
    gui_mgr = 0;
    interface = 0;
    loader = 0;
    sound_mgr = 0;
    //CEF::shutdown();

    //printFieldContainer();

    osgExit();
    std::exit(0);
}

void PolyVR::init(int argc, char **argv) {
    cout << "Init PolyVR\n\n";
    enableCoreDump(true);
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

    //GLUT
    glutInit(&argc, argv);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO | GLUT_STENCIL);
    else glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);

    //OSG
    ChangeList::setReadWriteDefault();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
    cout << "Init OSG\n";
    osgInit(argc,argv);

    cout << "Init Modules\n";

    scene_mgr = shared_ptr<VRSceneManager>(VRSceneManager::get());
    setup_mgr = shared_ptr<VRSetupManager>(VRSetupManager::get());
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
    interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());
    sound_mgr = shared_ptr<VRSoundManager>(&VRSoundManager::get());

    string app = options->getOption<string>("application");
    if (app != "") VRSceneManager::get()->loadScene(app);
}

void PolyVR::startTestScene(Node* n) {
    VRSceneManager::get()->newScene("test");
    VRSceneManager::getCurrent()->getRoot()->find("Headlight")->addChild(n);
    VRGuiManager::get()->wakeWindow();

    start();
}


OSG_END_NAMESPACE;
