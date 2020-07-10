#include <iostream>
#include <boost/filesystem.hpp>

#ifndef _WIN32
#include <sys/resource.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <vector>
#endif

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

void enableCoreDumps() {
    string file = boost::filesystem::current_path().string()+"/core";
    remove(file.c_str()); // remove old coredump in current folder
#ifndef _WIN32
    struct rlimit corelim;
    corelim.rlim_cur = -1;
    corelim.rlim_max = -1;
    if (setrlimit (RLIMIT_CORE, &corelim) != 0) cerr << "Couldn't set core limit\n";
#endif
}

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
    }

    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e) {
        SLOG << e.what() << endLog;
        window->clearPorts();
        server->stop();
        server->start();
    }

    catch ( ... ) {
        SLOG << "ERROR, unknown error thrown in display()" << endLog;
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
bool            active_stereo  = false;
string          address        = "";

void initServer(int argc, char **argv) {
	OSG::osgInit(argc, argv);

	int winid = glutCreateWindow(name);
	//if (argc>1) glutPositionWindow(atoi(argv[1]),0);
	if (fullscreen) glutFullScreen();
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

void evalParams(int argc, char **argv) {
    // evaluate params
    for(int a=1 ; a<argc ; ++a) {
        cout << " arg " << a << " : " << argv[a] << endl;
        if (argv[a][0] != '-') { name=argv[a]; continue; }

        switch(argv[a][1]) {
            case 'm': connectionType="Multicast"; break;
            case 'p': connectionType="SockPipeline"; break;
            case 'w': fullscreen=false; break;
            case 'A': active_stereo = true; break;
            case 'a':
                address = argv[a][2] ? argv[a]+2 : argv[++a];
                if (address.empty()) { SLOG << "address missing" << OSG::endLog; exit(0); }
                break;
            default:
                cout << argv[0] << " -m -p -w -a address -A " << argv[a][1] << OSG::endLog;
                exit(0);
        }
    }

    cout << "config:\n";
    cout << " connection type: " << connectionType << endl;
    cout << " name: " << name << endl;
    cout << " address: " << address << endl;
    cout << " fullscreen: " << fullscreen << endl;
    cout << " active_stereo: " << active_stereo << endl;
}

// Initialize GLUT & OpenSG and start the cluster server
int main(int argc, char **argv) {
    if (argc == 1) { // debug vars
        fullscreen = false;
        name = "localhost";
    }

#ifdef _WIN32
	/*DISPLAY_DEVICE dd;
	DWORD idx = 0;
	dd.cb = sizeof(dd);
	while(EnumDisplayDevices(NULL, idx, &dd, 0) != false) {
		_tprintf(_T("Name: %s\n"), dd.DeviceName);
		_tprintf(_T("String: %s\n"), dd.DeviceString);
		_tprintf(_T("ID: %s\n"), dd.DeviceID);
		_tprintf(_T("Key: %s\n"), dd.DeviceKey);
		_tprintf(_T("\n"));
		idx++;
	}*/

	/*BOOL CALLBACK MyInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
		MONITORINFOEX iMonitor;
		iMonitor.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(hMonitor, &iMonitor);

		if (iMonitor.dwFlags == DISPLAY_DEVICE_MIRRORING_DRIVER) return true;
		else {
			reinterpret_cast< std::vector<ScreenArray>* >(dwData)->push_back(ScreenArray(&iMonitor));
			return true;
		};
	}

	std::vector<ScreenArray> monitorArray;
	EnumDisplayMonitors(NULL, NULL, &MyInfoEnumProc, reinterpret_cast<LPARAM>(&monitorArray));*/

	struct MonitorRects {
		static BOOL CALLBACK MonitorEnum(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData) {
			MONITORINFO info;
			info.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(hMon, &info);
			cout << endl;
			cout << "Top of the selected monitor: " << info.rcMonitor.top << endl;
			cout << "bottom of selected monitor: " << info.rcMonitor.bottom << endl;
			cout << "left extreme of selected monitor: " << info.rcMonitor.left << endl;
			cout << "right extreme of selected monitor: " << info.rcMonitor.right << endl;
			cout << "Top of the selected monitor: " << info.rcWork.top << endl;
			cout << "bottom of selected monitor: " << info.rcWork.bottom << endl;
			cout << "left extreme of selected monitor: " << info.rcWork.left << endl;
			cout << "right extreme of selected monitor: " << info.rcWork.right << endl;
			return TRUE;
		}

		MonitorRects() {
			EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this);
		}
	};

	MonitorRects monitors;
#endif

	#define  GLUT_CAPTIONLESS                   0x0400
	#define  GLUT_BORDERLESS                    0x0800

    // initialize Glut
    enableCoreDumps();
    glutInit(&argc, argv);
    evalParams(argc, argv);
    //glutSetOption(GLUT_MULTISAMPLE, 4);
    if (active_stereo) glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE | GLUT_STENCIL | GLUT_BORDERLESS | GLUT_CAPTIONLESS | GLUT_STEREO);
    else               glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE | GLUT_STENCIL | GLUT_BORDERLESS | GLUT_CAPTIONLESS);

    //glEnable(GL_MULTISAMPLE);
    //glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

#ifdef _WIN32
    OSG::preloadSharedObject("OSGBase");
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
    //OSG::preloadSharedObject("OSGContribCgFX");
    OSG::preloadSharedObject("OSGContribPLY");
    OSG::preloadSharedObject("OSGEffectGroups");
    OSG::preloadSharedObject("OSGSystem");
    //OSG::preloadSharedObject("OSGWindowX");
    OSG::preloadSharedObject("OSGContribComputeBase");
    OSG::preloadSharedObject("OSGContribTrapezoidalShadowMaps");
    OSG::preloadSharedObject("OSGFileIO");
    OSG::preloadSharedObject("OSGText");
#endif

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



