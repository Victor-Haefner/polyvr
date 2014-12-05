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

    float timestep = 0.003f;

	CHECK( virtSetCommandType(vc, COMMAND_TYPE_IMPEDANCE) );
	CHECK( virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP) );
	CHECK( virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION) );
	CHECK( virtSetTimeStep(vc, timestep) );
    setSimulationScales(1.0f,1.0f);

    float identity[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
    //float baseFrame[7] = { 0.0f, 0.0f, 0.0f, 0.70710678f, 0.0f, 0.70710678f, 0.0f };
	//virtActiveSpeedControl(vc, 0.04f, 10.0f);
	CHECK( virtSetBaseFrame(vc, identity) );
    CHECK( virtSetObservationFrame(vc, identity) );
	CHECK( virtSetPowerOn(vc, 1) );
	//virtSetPeriodicFunction(vc, callback, &timestep, this);



    isAttached = true;


}

void virtuose::disconnect() {
    if (vc) {
        CHECK( virtSetPowerOn(vc, 0) );
		CHECK( virtDetachVO(vc) );
		//CHECK( virtStopLoop(vc) );
		CHECK( virtClose(vc) );
        vc = 0;
        isAttached = false;
    }
}

void virtuose::setSimulationScales(float translation, float forces) {
    CHECK( virtSetSpeedFactor(vc, translation) );
    CHECK( virtSetForceFactor(vc, forces) );
}

void virtuose::applyForce(Vec3f force, Vec3f torque) {
    float f[6] = { force[2], force[0], force[1], torque[2], torque[0], torque[1] };
    CHECK( virtAddForce(vc, f) );
}

Matrix virtuose::getPose() {
    float f[7]= {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
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

Vec3f virtuose::getForce() {
    float force[6] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    CHECK(virtGetForce(vc, force));
    Vec3f imp(force[1],force[2],force[0]);
    return imp;
}

/**
* takes the resulting force of given object and puts it on the virtuose
**/
void virtuose::updateHapticToObject(VRTransform* trans) {
    Matrix m = getPose();
    trans->setMatrix(m);
    vector<VRCollision> colls = trans->getPhysics()->getCollisions();
    Vec3f frc =   trans->getPhysics()->getForce();

    float force[6] = {frc.z(),frc.x(),frc.y(),0.0f,0.0f,0.0f};

    CHECK(virtSetForce(vc,force));


}


OSG_END_NAMESPACE;
