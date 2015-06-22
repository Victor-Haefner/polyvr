#include "PolyVR.h"

#include "core/scene/VRScene.h"
#include "core/objects/object/VRObject.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRSceneLoader.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSoundManager.h"
#include "core/objects/material/VRMaterial.h"
#include "core/networking/VRMainInterface.h"
#include <GL/glut.h>

#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGTypedGeoIntegralProperty.h>
#include <OpenSG/OSGNameAttachment.h>

#include <signal.h>
#ifndef _WIN32
extern "C" void coreDump(int sig) {
    auto mgr = OSG::VRSceneManager::get();
    string path = mgr->getOriginalWorkdir();
    cout << "\n dump core to " << path << "/core" << endl;
    mgr->setWorkdir(path);

    //kill(getpid(), sig);
    //abort();
    //raise(SIGABRT);
    kill(getpid(), SIGABRT);
}
#endif

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

void initPolyVR(int argc, char **argv) {
    cout << "Init PolyVR\n\n";
    setlocale(LC_ALL, "C");

#ifndef _WIN32
    signal(SIGSEGV, &coreDump);
    signal(SIGFPE, &coreDump);
#endif

    //Options
    VROptions::get()->parse(argc,argv);

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

    cout << "Init SceneManager\n";
    VRSceneManager::get();
    VRSetupManager::get();
    VRInternalMonitor::get();

    VRGuiManager::get();
    VRMainInterface::get();

    string app = VROptions::get()->getOption<string>("application");
    if (app != "") VRSceneManager::get()->loadScene(app);
}

void exitPolyVR() {
    delete VRGuiManager::get();
    delete VRSetupManager::get();
    delete VRSceneManager::get();
    delete VRSceneLoader::get();
    delete VROptions::get();
    delete VRInternalMonitor::get();
    delete VRMainInterface::get();
    delete &VRSoundManager::get();
    VRMaterial::clearAll();

    //printFieldContainer();

    osgExit();
    exit(0);
}


void startPolyVR() {
    while(true) VRSceneManager::get()->update();
}

void startPolyVR_testScene(NodeRecPtr n) {
    VRSceneManager::get()->newScene("test");
    VRSceneManager::getCurrent()->getRoot()->find("Headlight")->addChild(n);
    VRGuiManager::get()->wakeWindow();

    startPolyVR();
}


OSG_END_NAMESPACE;
