// OpenSG Tutorial Example: Cluster Server
//
// This is a full functional OpenSG cluster server. In OpenSG
// the terms server and client are used similar to X11. The
// application is the client. Instances that are used for 
// rendering are called server.
//
// See the clusterclient.cpp for an example of how to use them.
//
// Libs: Cluster

#include <iostream>

#ifdef OSG_BUILD_ACTIVE
// GLUT is used for window handling
#include <OSGGLUT.h>
// General OpenSG configuration, needed everywhere
#include <OSGConfig.h>
// The Cluster server definition
#include <OSGClusterServer.h>
// The GLUT-OpenSG connection class
#include <OSGGLUTWindow.h>
// Render action definition. 
#include <OSGRenderAction.h>
#else
// GLUT is used for window handling
#include <OpenSG/OSGGLUT.h>
// General OpenSG configuration, needed everywhere
#include <OpenSG/OSGConfig.h>
// The Cluster server defini2tion
#include <OpenSG/OSGClusterServer.h>
// The GLUT-OpenSG connection class
#include <OpenSG/OSGGLUTWindow.h>
// Render action definition. 
#include <OpenSG/OSGRenderAction.h>
#endif

// local glut window
OSG::GLUTWindowRefPtr    window;
// render action
OSG::RenderActionRefPtr  ract;
// pointer the the cluster server instance
OSG::ClusterServer      *server;

// forward declaration so we can have the interesting stuff upfront
void display(void);
void update (void);
void reshape(int width, int height);

// Initialize GLUT & OpenSG and start the cluster server
int main(int argc, char **argv)
{
    int             winid;
    const char     *name           = "ClusterServer";
    const char     *connectionType = "StreamSock";
    bool            fullscreen     = true;
    std::string     address        = "";

    // initialize Glut
    glutInit(&argc, argv);
    glutInitDisplayMode( GLUT_RGB | 
                         GLUT_DEPTH | 
                         GLUT_DOUBLE);

    // evaluate params
    for(int a=1 ; a<argc ; ++a)
    {
        if(argv[a][0] == '-')
        {
            switch(argv[a][1])
            {
                case 'm': connectionType="Multicast";
                          break;
                case 'p': connectionType="SockPipeline";
                          break;
                case 'w': fullscreen=false;
                          break;
                case 'a': address = argv[a][2] ? argv[a]+2 : argv[++a];
                          if(address.empty())
                          { 
                              SLOG << "address missing" << OSG::endLog;
                              return 0;
                          }
                          std::cout << address << OSG::endLog;
                          break;
                default:  std::cout << argv[0] 
                                    << "-m "
                                    << "-p "
                                    << "-w "
                                    << "-a address "
                                    << OSG::endLog;
                          return 0;
            }
        }
        else
        {
            name=argv[a];
        }
    }
    try
    {
        // init OpenSG
        OSG::osgInit(argc, argv);

        winid = glutCreateWindow(name);
        if(fullscreen)
            glutFullScreen();
        glutDisplayFunc(display);
        glutIdleFunc(update);
        glutReshapeFunc(reshape);

        glEnable( GL_LIGHTING );
        glEnable( GL_LIGHT0 );
        glEnable( GL_NORMALIZE );
        glutSetCursor(GLUT_CURSOR_NONE);

        // create the render action
        ract = OSG::RenderAction::create();

        // setup the OpenSG Glut window
        window     = OSG::GLUTWindow::create();
        window->setGlutId(winid);
        window->init();

        // create the cluster server
        server     = new OSG::ClusterServer(window,name,connectionType,address);
        // start the server
        server->start();

        // enter glut main loop
        glutMainLoop();
    }
    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
    {
        SLOG << e.what() << OSG::endLog;
        
        // clean up global variables
        delete server;

        ract   = NULL;
        window = NULL;
        
        OSG::osgExit(); 
    }
    return 0;
}

/* render loop */
void display()
{
    try
    {
        // receive scenegraph and do rendering
        server->render(ract);
        // clear changelist 
        OSG::Thread::getCurrentChangeList()->clear();
    } 
    catch(OSG_STDEXCEPTION_NAMESPACE::exception &e)
    {
        SLOG << e.what() << OSG::endLog;
        
        window->clearPorts();

        // try to restart server
        server->stop();
        // start server, wait for client to connect
        server->start();
    }
}

void update(void)
{
    glutPostRedisplay();
}

/* window reshape */
void reshape( int width, int height )
{
    // set new window size
	window->resize( width, height );
}
