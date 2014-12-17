#include "virtuose.h"

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
    isAttached = false;
    gripperPosition = 0.0f;
    gripperSpeed = 0.0f;

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
    float identity[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};

	CHECK( virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION) );
    setSimulationScales(1.0f,0.1f);
    timestep = 0.003f;
	CHECK( virtSetTimeStep(vc, timestep) );
	CHECK( virtSetBaseFrame(vc, identity) );
    CHECK( virtSetObservationFrame(vc, identity) );
	//CHECK( virtSetCommandType(vc, COMMAND_TYPE_IMPEDANCE) );
	CHECK( virtSetCommandType(vc, COMMAND_TYPE_VIRTMECH) );
	commandType = COMMAND_TYPE_VIRTMECH;
	CHECK( virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP) );

    //float baseFrame[7] = { 0.0f, 0.0f, 0.0f, 0.70710678f, 0.0f, 0.70710678f, 0.0f };
	//virtActiveSpeedControl(vc, 0.04f, 10.0f);

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
        isAttached = false;
        CHECK( virtSetCommandType(vc, COMMAND_TYPE_IMPEDANCE) );
        commandType = COMMAND_TYPE_IMPEDANCE;

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

    if(commandType == COMMAND_TYPE_IMPEDANCE) {
        CHECK( virtGetAvatarPosition(vc, f) );
    }
    //CHECK( virtGetPhysicalPosition(vc, f) );

    Matrix m;

    Vec3f pos(f[1], f[2], f[0]);
    Quaternion q;
    q.setValue(f[4], f[5], f[3]);
    m.setRotate(q);
    m.setTranslate(pos);
    return m;
}




void virtuose::attachTransform(VRTransform* trans) {
    isAttached = true;
    attached = trans;
    VRPhysics* o = trans->getPhysics();
    //add a damping to attached obj
    o->setDamping(0.5f,0.5f);
    btMatrix3x3 t = o->getInertiaTensor();
    //"roundabout"-inertia-tensor
    float s = 0.1;
    float k = s*s*o->getMass()/6.0;
    float inertia[9] = {k,0.0,0.0,0.0,k,0.0,0.0,0.0,k};
    CHECK(virtAttachVO(vc, o->getMass(), inertia));

}

void virtuose::detachTransform() {
    isAttached = false;
    CHECK(virtDetachVO(vc));
    attached = 0;

}


/**
* takes positiob, speed of attached object and puts it on the virtuose
**/
void virtuose::updateVirtMech() {

    // calc time delta in seconds
    //float timeNow = glutGet(GLUT_ELAPSED_TIME);
    //float dt = (timeNow - timeLastFrame) * 0.001f;
    //timeLastFrame = timeNow;

    float position[7] = {0.0,0.0,0.0,0.0,0.0,0.0,1.0};
    float speed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
    float force[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
    int power = 0;

    CHECK(virtIsInShiftPosition(vc,&power));
    if(commandType == COMMAND_TYPE_VIRTMECH)
	{


            if (!isAttached)
            {
                virtGetPosition(vc, position);
                virtSetPosition(vc, position);
                virtGetSpeed(vc, speed);
                virtSetSpeed(vc, speed);

                virtGetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                virtGetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
                virtSetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                virtSetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
            }
            else
            {
                 //apply position&speed to the haptic
                 btTransform pos = this->attached->getPhysics()->getTransform();
                 position[0] = (float) pos.getOrigin().getZ();
                 position[1] = (float) pos.getOrigin().getX();
                 position[2] = (float) pos.getOrigin().getY();
                 pos.setRotation(pos.getRotation().normalized());
                 position[3] = (float) pos.getRotation().getZ();
                 position[4] = (float) pos.getRotation().getX();
                 position[5] = (float) pos.getRotation().getY();
                 position[6] = (float) pos.getRotation().getW();
                 CHECK(virtSetPosition(vc, position));
                 Vec3f vel = this->attached->getPhysics()->getLinearVelocity();
                 speed[0] =(float) vel.z();
                 speed[1] =(float) vel.x();
                 speed[2] =(float) vel.y();
                 vel = this->attached->getPhysics()->getAngularVelocity();
                 speed[3] =(float) vel.z();
                 speed[4] = (float)vel.x();
                 speed[5] =(float) vel.y();
                 CHECK(virtSetSpeed(vc, speed));

                 //get force applied by human on the haptic
                 CHECK(virtGetForce(vc, force));
                 //multiply with hardcoded "bullshit"- factor
                 float f_lin = 0.1f;
                 float f_ang = 0.1f;

                 Vec3f frc = Vec3f(force[1], force[2], force[0]) * f_lin;
                 Vec3f trqu = Vec3f((float)force[4],(float)force[5],(float)force[3]);
                 trqu *= f_ang;
                 //cout << globalforce[0] << " " <<globalforce[1] <<" " << globalforce[2] <<" " << "\n ";

                 //apply force on the object
                 if(power == 0) {
                     attached->getPhysics()->addForce(frc);
                     attached->getPhysics()->addTorque(trqu);
                 }

            }
	    }


}


OSG_END_NAMESPACE;
