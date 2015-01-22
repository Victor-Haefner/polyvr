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
    setSimulationScales(1.0f,1.0f);
    timestep = 0.017f;
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




void virtuose::attachTransform(VRTransform* trans) {
    isAttached = true;
    attached = trans;
    VRPhysics* o = trans->getPhysics();
    btMatrix3x3 t = o->getInertiaTensor();
    t = t.inverse();
    float inertia[9] {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    Matrix3ToArray(t,inertia);
    cout<<"\n "<<"\n "<<inertia[0] << "    " <<inertia[1] <<  "    " <<inertia[2] << "\n "<<inertia[3] <<  "    " <<inertia[4] <<  "    " <<inertia[5] << "\n "<<inertia[6] << "    " <<inertia[7] <<"    " << inertia[8]<<"\n ";
    //"roundabout"-inertia-tensor
    //float s = 0.1;
    //float k = s*s*o->getMass()/6.0;
    //float inertia[9] = {k,0.0,0.0,0.0,k,0.0,0.0,0.0,k};
    //float tmp = inertia[0];
    //inertia[0] = inertia[8];
    //inertia[8] = inertia[4];
    //inertia[4] = tmp;

    cout<<"\n "<<"\n "<<inertia[0] << "    " <<inertia[1] <<  "    " <<inertia[2] << "\n "<<inertia[3] <<  "    " <<inertia[4] <<  "    " <<inertia[5] << "\n "<<inertia[6] << "    " <<inertia[7] <<"    " << inertia[8]<<"\n ";

    CHECK(virtAttachVO(vc, o->getMass(), inertia));

}

void virtuose::fillPosition(VRPhysics* p, float *to) {
     btTransform pos = p->getTransform();
     to[0] =  pos.getOrigin().getZ();
     to[1] = pos.getOrigin().getX();
     to[2] =  pos.getOrigin().getY();
     //pos.setRotation(pos.getRotation().normalized());
     to[3] =  pos.getRotation().getZ();
     to[4] =  pos.getRotation().getX();
     to[5] =  pos.getRotation().getY();
     to[6] =  pos.getRotation().getW();
}
void virtuose::fillSpeed(VRPhysics* p, float *to) {
     Vec3f vel = p->getLinearVelocity();
     to[0] = vel.z();
     to[1] = vel.x();
     to[2] = vel.y();
     vel = p->getAngularVelocity();
     to[3] = vel.z();
     to[4] = vel.x();
     to[5] = vel.y();
}
void virtuose::Matrix3ToArray(btMatrix3x3 m, float *to) {
    int j = 0;
    for(int i = 0; i < 3; i++) {
        to[j] = m.getRow(i).getX();
        to[j + 1] = m.getRow(i).getY();
        to[j + 2] = m.getRow(i).getZ();
        j = j + 3;
    }

}



void virtuose::detachTransform() {
    isAttached = false;
    CHECK(virtDetachVO(vc));
    attached = 0;

}

OSG::Vec3i virtuose::getButtonStates() {
    int i = 0;
    int j = 0;
    int k = 0;
    CHECK(virtGetButton(vc,0,&i));
    CHECK(virtGetButton(vc,1,&j));
    CHECK(virtGetButton(vc,2,&k));
    return Vec3i(i,j,k);
}

void virtuose::updateVirtMech() {

    // calc time delta in seconds
    float timeNow = glutGet(GLUT_ELAPSED_TIME);
    float dt = (timeNow - timeLastFrame) * 0.001f;
    timeLastFrame = timeNow;

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

            }
            else
            {
                 float ts = 0.0;
                 CHECK(virtGetTimeStep(vc, &ts));

                 //apply position&speed to the haptic
                 fillPosition(this->attached->getPhysics(),position);
                 CHECK(virtSetPosition(vc, position));
                 fillSpeed(this->attached->getPhysics(),speed);
                 for(int i = 0; i < 6; i++) {
                     speed[i] *= dt;
                     speed[i] *= (1/ts);
                 }


                 CHECK(virtSetSpeed(vc, speed));
                 //get force applied by human on the haptic
                 CHECK(virtGetForce(vc, force));
                for(int i = 0; i < 6; i++) {
                     force[i] *= ts;
                     force[i] *= ts;
                     force[i] *= (1/dt);
                     force[i] *= (1/dt);
                 }

                 Vec3f frc = Vec3f(force[1], force[2], force[0]);
                 Vec3f trqu = Vec3f(force[4],force[5],force[3]);
                 //cout << globalforce[0] << " " <<globalforce[1] <<" " << globalforce[2] <<" " << "\n ";
                 //apply force on the object
                 if(power == 0) {
                    //optimization against "bumby" surfaces (for interaction with static rigidbodies only!)
/*
                    vector<VRCollision> colls = attached->getPhysics()->getCollisions();
                    VRCollision tmp;
                    Vec3f v = Vec3f(0.0,0.0,0.0);
                    for(int i = 0; i < colls.size(); i++) {
                        tmp = colls[i];
                        v += tmp.norm;
                    }
                    v.normalize();
                    float bumpBackf = -((frc[0] * v[0]) + (frc[1] * v[1]) + (frc[2] * v[2]));
                    frc += (v * bumpBackf);
*/

                     attached->getPhysics()->addForce(frc);
                     attached->getPhysics()->addTorque(trqu);
                 }

            }
	    }


}


OSG_END_NAMESPACE;
