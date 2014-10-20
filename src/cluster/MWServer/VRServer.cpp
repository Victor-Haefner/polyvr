
// example usage:
// export OSG_LOAD_LIBS="OSGEffectGroups:OSGContribTrapezoidalShadowMaps" && ./VRServer -n localhost -c Multicast -s 0.0:640x480+0+0

#include <iostream>
#include <string>
#include <vector>

#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGConfig.h>
#include "OSGClusterServer.h"
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGRenderAction.h>

using namespace OSG;
using namespace std;

vector<WindowRefPtr> windows;
vector<WindowRefPtr>::iterator itr;
RenderActionRefPtr ract;
ClusterServer *server;

void displayCB(void);
void update (void);
void reshape(int width, int height);

string name           = "ClusterServer";
string connectionType = "StreamSock";
string address        = "";
string screens        = "0.0:640x480+0+0";
string port           = "8437";

void cleanExit() {
    delete server;
    ract   = NULL;
    windows.clear();
    osgExit();
}

void addWindow(string parameter) {

    // get window configuration --------------
    bool fullscreen = false;
    int display, screen, width, height, h_offset, v_offset;
    char c;

    stringstream ss; ss << parameter;
    ss >> display; ss >> c;
    ss >> screen; ss >> c;

    if (ss.peek() == 'f') fullscreen = true;
    else {
        ss >> width; ss >> c;
        ss >> height;
        ss >> h_offset;
        ss >> v_offset;
    }


    // print window configuration ------------
    cout << "\nAdd window: ";
    cout << "\n Display: " << display;
    cout << "\n Screen: " << screen;
    if (fullscreen) cout << "\n Fullscreen";
    else {
        cout << "\n Size: " << width << " x " << height;
        cout << "\n Offset: " << h_offset << " , " << v_offset;
    }
    cout << endl;
    // ---------------------------------------

    if (!fullscreen) glutInitWindowSize(width, height);
    int winid = glutCreateWindow(name.c_str());
    glutReshapeFunc(reshape);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutDisplayFunc(displayCB);
    glutIdleFunc(update);
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    glEnable( GL_NORMALIZE );

    GLUTWindowRefPtr win = GLUTWindow::create();
    win->setGlutId(winid);
    win->init();

    if (fullscreen) glutFullScreen();
    if (!fullscreen) glutPositionWindow(h_offset, v_offset);

    windows.push_back(win);
}

void initGlut(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

        OSG::preloadSharedObject("OSGBase");
        OSG::preloadSharedObject("OSGContribTrapezoidalShadowMaps");
        OSG::preloadSharedObject("OSGCluster");
        OSG::preloadSharedObject("OSGDrawable");
        OSG::preloadSharedObject("OSGEffectGroups");
    OSG::preloadSharedObject("OSGFileIO");
        OSG::preloadSharedObject("OSGGroup");
    OSG::preloadSharedObject("OSGImageFileIO");
        OSG::preloadSharedObject("OSGState");
        OSG::preloadSharedObject("OSGSystem");
        OSG::preloadSharedObject("OSGUtil");
        OSG::preloadSharedObject("OSGText");
        OSG::preloadSharedObject("OSGWindow");
        OSG::preloadSharedObject("OSGWindowGLUT");
        OSG::preloadSharedObject("OSGWindowX");

    osgInit(argc, argv);
    ract = RenderAction::create();
}

void initServer() {
    server = new ClusterServer(name, connectionType, address);
    for (itr = windows.begin(); itr != windows.end(); itr++) server->addWindow(*itr);

    server->start();
    glutMainLoop();
}

void parseParams(int argc, char **argv) {
    // evaluate params
    for(int i=1 ; i+1<argc ; i+=2) {
        string option = argv[i];
        string parameter = argv[i+1];

        switch (option[1]) {
            case 'n':
                name = parameter;
                break;
            case 'c':
                connectionType = parameter ;
                break;
            case 's':
                addWindow(parameter);
                break;
            case 'a':
                address = parameter;
                break;
            case 'p':
                port = parameter;
            default:
                cout << "\nunknown option " << option << ", usage:";
                cout << "\n -c : Connection type, Multicast, SockPipeline or StreamSock (default)";
                cout << "\n -n : Multicast name";
                cout << "\n -a : Address?";
                cout << "\n -s : Screen configuration, 0.0:640x480+0+0 (default) 0.0:f (fullscreen)";
                cout << endLog;
                cleanExit();
        }
    }
}

int main(int argc, char **argv) {
    initGlut(0, NULL);
    parseParams(argc, argv);

    try { initServer(); }
    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << endLog;
        cleanExit();
    }

    return 0;
}

void displayCB() {
    try {
        server->render(ract);
        Thread::getCurrentChangeList()->clear();

    } catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << endLog;

        for (itr = windows.begin(); itr != windows.end(); itr++) (*itr)->clearPorts();
        server->stop();
        server->start();
    }
}

void update(void) { glutPostRedisplay(); }
void reshape( int width, int height) {
    cout << "\nReshape\n";
    for (itr = windows.begin(); itr != windows.end(); itr++) (*itr)->resize( width, height );
}

