#include "virtuose.h"
#include <virtuose/virtuoseAPI.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <fcntl.h>

#include <OpenSG/OSGQuaternion.h>

#define CHECK(x) { \
  int result = (x); \
  if (result != 0) { \
    fprintf(stderr, "\nRuntime error: %s returned %d at %s:%d", #x, result, __FILE__, __LINE__); \
    fprintf(stderr, "\n err msg: %s", virtGetErrorMessage(virtGetErrorCode(vc))); \
  } \
}

#define CHECK_INIT(vc) \
if (vc == NULL) { \
  fprintf(stderr, "\nRuntime error: virtOpen returned 0 at %s:%d", __FILE__, __LINE__); \
  fprintf(stderr, "\n err msg: %s", virtGetErrorMessage(virtGetErrorCode(vc))); \
}

OSG_BEGIN_NAMESPACE;
using namespace std;


/*

haptic coordinate system

     z
     |
     x--y

to OSG
y -> x
z -> y
x -> z
Vec3f(y,z,x);
virtVec(z,x,y);

*/

virtuose::virtuose() {
}

virtuose::~virtuose() {
    disconnect();
}

bool virtuose::connected() { return (vc != 0); }

bool checkVirtuoseIP(string IP) { // TODO: try if haptic is found -> right port?
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // TODO: try blocking but with timeout
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(3131);  // Could be anything
    inet_pton(AF_INET, IP.c_str(), &sin.sin_addr);

    if (connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
        printf("No haptic device at IP %s: %d (%s)\n", IP.c_str(), errno, strerror(errno));
        return false;
    }
    return true;
}

void virtuose::connect(string IP) {
    disconnect();
    //if (!checkVirtuoseIP(IP)) return; // TODO: does not work
    cout << "Open virtuose " << IP << endl;
    vc = virtOpen(IP.c_str());
    CHECK_INIT(vc);
    if (vc == 0) return;

    float timestep = 0.003;
	CHECK( virtSetCommandType(vc, COMMAND_TYPE_IMPEDANCE) );
	CHECK( virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP) );
	CHECK( virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION) );
	CHECK( virtSetTimeStep(vc, timestep) );
    setSimulationScales(1,1);

    float identity[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
    //float baseFrame[7] = { 0.0f, 0.0f, 0.0f, 0.70710678f, 0.0f, 0.70710678f, 0.0f };
	//virtActiveSpeedControl(vc, 0.04f, 10.0f);
	CHECK( virtSetBaseFrame(vc, identity) );
    CHECK( virtSetObservationFrame(vc, identity) );
	CHECK( virtSetPowerOn(vc, 1) );
	//virtSetPeriodicFunction(vc, callback, &timestep, this);
}

void virtuose::disconnect() {
    if (vc) {
        CHECK( virtSetPowerOn(vc, 0) );
		CHECK( virtDetachVO(vc) );
		//CHECK( virtStopLoop(vc) );
		CHECK( virtClose(vc) );
        vc = 0;
    }
}

void virtuose::setSimulationScales(float translation, float forces) {
    CHECK( virtSetSpeedFactor(vc, translation) );
    CHECK( virtSetForceFactor(vc, forces) );
}

void virtuose::applyForce(Vec3f force, Vec3f torque) {
    float f[6] = { force[2], force[0], force[1], torque[2], torque[0], torque[1] };
    CHECK( virtSetForce(vc, f) );
}

Matrix virtuose::getPose() {
    float f[6];
    CHECK( virtGetAvatarPosition(vc, f) );
    //CHECK( virtGetPhysicalPosition(vc, f) );

    Matrix m;

    Vec3f pos(f[1], f[2], f[0]);
    Quaternion q;
    q.setValue(f[4], f[5], f[3]);
    m.setRotate(q);
    m.setTranslate(pos);
    return m;
}


void virtuose::connectPhysicalized(VRPhysics* ph) {
    connObjects.push_back(ph);
}
void virtuose::disconnectPhysicalized() {
    connObjects.pop_back();
}

/**
* updates the forces affecting the connected objects
**/
void virtuose::updateConnectedObjects() {

    //virtuose
    float f[6];
    CHECK( virtGetForce(vc, f) );
    Vec3f virtForce(f[1], f[2], f[0]);

    //for all connected objects
    for (connObjIt = connObjects.begin(); connObjIt != connObjects.end(); connObjIt++) {
        if((*connObjIt)->isPhysicalized()) {
            (*connObjIt)->applyForce(virtForce);
        }
    }
}

/**
* takes the resulting force of the last connected object and puts it on the virtuose
**/
void virtuose::updateFeedbackForces() {

    connObjIt = connObjects.end();
    connObjIt--;
    if(connObjects.empty())
        return;
    float f[6];
    btVector3 diff;

     //virtuose
    CHECK( virtGetForce(vc, f) );
    btVector3 virtForce(f[1], f[2], f[0]);

    //connected object
    //last object (affects the haptic)
    diff = ((*connObjIt)->getForce() * virtForce.length());
    applyForce(Vec3f((float) diff.getX(),(float) diff.getY(),(float) diff.getZ()),
               Vec3f(0.0f,0.0f,0.0f));


}


OSG_END_NAMESPACE;
