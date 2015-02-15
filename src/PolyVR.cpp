#include "PolyVR.h"
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

#include <signal.h>
extern "C" void my_function_to_handle_aborts(int signal_number) {
    cout << "\nARG - ABORT!\n";
    return;
}

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

    signal(SIGABRT, &my_function_to_handle_aborts);

    //Options
    VROptions::get()->parse(argc,argv);

    //GLUT
    glutInit(&argc, argv);
    glEnable(GL_DEPTH_TEST);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO);
    else glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

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
    //VRFunction<VRThread*>* fkt = new VRFunction<VRThread*>( "VRSceneManager::update", boost::bind(&VRSceneManager::update, VRSceneManager::get()) );
    //VRSceneManager::get()->initThread(fkt, "", true, 0);
    while(true) {
        VRSceneManager::get()->update();
    }
}


OSG_END_NAMESPACE;
