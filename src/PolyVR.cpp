#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <OpenSG/OSGGL.h>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGPrimeMaterial.h>
#include <OpenSG/OSGNameAttachment.h>

#include "PolyVR.h"

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetupManager.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/coreDumpHandler.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
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

OSG_BEGIN_NAMESPACE;
using namespace std;

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
PolyVR::~PolyVR() {}

PolyVR* PolyVR::get() {
    static PolyVR* pvr = new PolyVR();
    return pvr;
}

#ifdef WASM
EMSCRIPTEN_KEEPALIVE void PolyVR_shutdown() { PolyVR::shutdown(); }
EMSCRIPTEN_KEEPALIVE void PolyVR_reloadScene() { VRSceneManager::get()->reloadScene(); }
#endif

void PolyVR::shutdown() {
    cout << "PolyVR::shutdown" << endl;
#ifndef WITHOUT_SHARED_MEMORY
    try {
        VRSharedMemory sm("PolyVR_System");
        int* i = sm.addObject<int>("identifier");
        *i = 0;
    } catch(...) {}
#endif

    auto pvr = get();
    pvr->scene_mgr->closeScene();
    VRSetup::getCurrent()->stopWindows();
    pvr->scene_mgr->stopAllThreads();
    pvr->setup_mgr->closeSetup();

/*
    pvr->monitor.reset();
    pvr->gui_mgr.reset();
    pvr->interface.reset();
    pvr->loader.reset();
    pvr->setup_mgr.reset();
    pvr->scene_mgr.reset();
    pvr->sound_mgr.reset();
    pvr->options.reset();*/

    delete pvr;
#ifndef WASM
    printFieldContainer();
#endif
    osgExit();
#ifdef WASM
    emscripten_force_exit(0);
#else
    std::exit(0);
#endif
}

void printNextOSGID(int i) {
    NodeRefPtr n = Node::create();
    cout << "next OSG ID: " << n->getId() << " at maker " << i << endl;
}

void PolyVR::setOption(string name, bool val) { options->setOption(name, val); }
void PolyVR::setOption(string name, string val) { options->setOption(name, val); }
void PolyVR::setOption(string name, int val) { options->setOption(name, val); }
void PolyVR::setOption(string name, float val) { options->setOption(name, val); }


void display(void) {
	/*static float f = 0;
	static float d = 1;
	f += d*0.005;
	if (abs(f) > 1) d *= -1;

	//background->setColor(Color3f(1,f,-f));
	Matrix m;
	m.setTransform( Vec3f(), Quaternion( OSG::Vec3f(0,1,0), f ) );
	trans->setMatrix(m);

	if (window) {
		commitChanges();
		window->render(renderAction);
	}*/
}

void idle(void) {
	glutPostRedisplay();
}

void reshape(int w, int h) {
	//cout << "glut reshape " << glutGetWindow() << endl;
	/*if (window) {
		window->resize(w, h);
		glutPostRedisplay();
	}*/
}


void PolyVR::init(int argc, char **argv) {
    cout << "Init PolyVR\n\n";
    initTime();

#ifdef WASM
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);

    //OSG
    cout << "Init OSG\n";
    ChangeList::setReadWriteDefault();
    osgInit(argc,argv);
    cout << " ..done\n";

    //GLUT
    cout << "Init GLUT\n";
    glutInit(&argc, argv);
	glutInitWindowSize(300, 300);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    cout << " ..done\n";

    PrimeMaterialRecPtr pMat = OSG::getDefaultMaterial();
    OSG::setName(pMat, "default_material");
#else
    enableCoreDump(true);
    setlocale(LC_ALL, "C");
    options = shared_ptr<VROptions>(VROptions::get());
    options->parse(argc,argv);
    checkProcessesAndSockets();

    //GLUT
    cout << "Init GLUT\n";
    glutInit(&argc, argv);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    if (VROptions::get()->getOption<bool>("active_stereo"))
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STEREO | GLUT_STENCIL);
    else glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL);
    cout << " ..done\n";

    //OSG
    cout << "Init OSG\n";
    ChangeList::setReadWriteDefault();
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGImageFileIO");
    osgInit(argc,argv);
    cout << " ..done\n";

#ifndef WITHOUT_SHARED_MEMORY
    try {
        VRSharedMemory sm("PolyVR_System");
        int* i = sm.addObject<int>("identifier");
        *i = 1;
    } catch(...) {}
#endif

    PrimeMaterialRecPtr pMat = OSG::getDefaultMaterial();
    OSG::setName(pMat, "default_material");
#endif

    cout << "Init Modules\n";
#ifndef WITHOUT_AV
    sound_mgr = VRSoundManager::get();
#endif
    setup_mgr = VRSetupManager::create();
#ifdef WASM
    VRSetupManager::get()->load("Browser", "Browser.xml");
#endif
    scene_mgr = VRSceneManager::create();
    interface = shared_ptr<VRMainInterface>(VRMainInterface::get());
    monitor = shared_ptr<VRInternalMonitor>(VRInternalMonitor::get());
#ifndef WITHOUT_GTK
    gui_mgr = shared_ptr<VRGuiManager>(VRGuiManager::get());
#endif
    loader = shared_ptr<VRSceneLoader>(VRSceneLoader::get());

    //string app = options->getOption<string>("application");
    //if (app != "") VRSceneManager::get()->loadScene(app);
    removeFile("setup/.startup"); // remove startup failsafe
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
    cout << "Start main loop\n";
#ifndef WASM
    while(true) update(); // default
#else
    // WASM needs to control the main loop
    glutIdleFunc(glutPostRedisplay);
    glutDisplayFunc(glutUpdate);
    glutMainLoop();
#endif
}

void PolyVR::startTestScene(OSGObjectPtr n, Vec3d camPos) {
    cout << "start test scene " << n << endl;
    VRSceneManager::get()->newScene("test");
    VRScene::getCurrent()->getRoot()->find("light")->addChild(n);

    VRTransformPtr cam = dynamic_pointer_cast<VRTransform>( VRScene::getCurrent()->get("Default") );
    cam->setFrom(camPos);

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

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}

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
}


OSG_END_NAMESPACE;
