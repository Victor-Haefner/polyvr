#include <iostream>

#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGClusterServer.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGViewport.h>

using namespace std;
using namespace OSG;

GLUTWindowRefPtr    window;
RenderActionRefPtr  ract;
ClusterServer      *server;

bool doPrint() {
    static int t0 = 0;
    int t = time(0);
    if ( t-t0 >= 1 ) {
        t0 = t;
        return true;
    }
    return false;
}

void printBVolume() {
    ViewportRecPtr v = window->getPort(0);
    if (v == 0) return;

    NodeRecPtr root = v->getRoot();
    if (root == 0) return;

    Vec3f min,max;
    //root->updateVolume();
    BoxVolume vol = root->getVolume();
    vol.getBounds( min, max );

    cout << "\nRACT " << max-min << endl;
}

void display() {

    try {
        server->render(ract);
        Thread::getCurrentChangeList()->clear();

    } catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << endLog;

        window->clearPorts();
        server->stop();
        server->start();
    }

    //if (doPrint()) cout << "\nRACT " << ract->getFrustumCulling() << endl;
    //if (doPrint()) printBVolume();
}

void update(void) { glutPostRedisplay(); }
void reshape( int width, int height ) { window->resize( width, height ); }

const char     *name           = "ClusterServer";
const char     *connectionType = "StreamSock";
bool            fullscreen     = true;
std::string     address        = "";

void initServer(int argc, char **argv) {
	OSG::osgInit(argc, argv);

	int winid = glutCreateWindow(name);
	if (argc>1) glutPositionWindow(atoi(argv[1]),0);
	if(fullscreen) glutFullScreen();
	glutDisplayFunc(display);
	glutIdleFunc(update);
	glutReshapeFunc(reshape);
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	glEnable( GL_NORMALIZE );
	glutSetCursor(GLUT_CURSOR_NONE);

	ract = OSG::RenderAction::create();

	window = OSG::GLUTWindow::create();
	window->setGlutId(winid);
	window->init();

	server = new OSG::ClusterServer(window,name,connectionType,address);
	server->start();

	glutMainLoop();
}

// Initialize GLUT & OpenSG and start the cluster server
int main(int argc, char **argv) {
    if (argc == 1) { // debug vars
        fullscreen = false;
        name = "localhost";
    }

    // initialize Glut
    glutInit(&argc, argv);
    glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    /*OSG::preloadSharedObject("OSGBase");
    OSG::preloadSharedObject("OSGContribCSMSimplePlugin");
    OSG::preloadSharedObject("OSGContribWebInterface");
    OSG::preloadSharedObject("OSGGroup");
    OSG::preloadSharedObject("OSGUtil");
    OSG::preloadSharedObject("OSGCluster");
    OSG::preloadSharedObject("OSGContribCSM");
    OSG::preloadSharedObject("OSGDrawable");
    OSG::preloadSharedObject("OSGImageFileIO");
    OSG::preloadSharedObject("OSGWindowGLUT");
    OSG::preloadSharedObject("OSGContribBackgroundLoader");
    OSG::preloadSharedObject("OSGContribGUI");
    OSG::preloadSharedObject("OSGDynamics");
    OSG::preloadSharedObject("OSGState");
    OSG::preloadSharedObject("OSGWindow");
    OSG::preloadSharedObject("OSGContribCgFX");
    OSG::preloadSharedObject("OSGContribPLY");
    OSG::preloadSharedObject("OSGEffectGroups");
    OSG::preloadSharedObject("OSGSystem");
    OSG::preloadSharedObject("OSGWindowX");
    OSG::preloadSharedObject("OSGContribComputeBase");
    OSG::preloadSharedObject("OSGContribTrapezoidalShadowMaps");
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGText");*/

    // evaluate params
    for(int a=1 ; a<argc ; ++a) {
        if(argv[a][0] != '-') name=argv[a];
        else {
            char o = argv[a][1];
            if (o == 'm') connectionType="Multicast";
            else if (o == 'p') connectionType="SockPipeline";
            else if (o == 'w') fullscreen=false;
            else if (o == 'a') {
                address = argv[a][2] ? argv[a]+2 : argv[++a];
                if(address.empty()) {
                    SLOG << "address missing" << OSG::endLog;
                    return 0;
                }
                std::cout << address << OSG::endLog;
            } else {
                std::cout << argv[0] << "-m -p -w -a address " << OSG::endLog;
                return 0;
            }
        }
    }

    try { initServer(argc, argv); }
    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << OSG::endLog;

        delete server;
        ract   = NULL;
        window = NULL;
        OSG::osgExit();
    }

    return 0;
}
