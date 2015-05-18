#include "virtuose.h"
#include <virtuose/virtuoseAPI.h>
#include <OpenSG/OSGQuaternion.h>
#include "core/networking/VRPing.h"

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

virtuose::~virtuose() { disconnect(); }
bool virtuose::connected() { return (vc != 0); }
void virtuose::enableForceFeedback(bool enable) {if(vc==0)return; int i = (enable==true ? 1 : 0);CHECK(virtEnableForceFeedback(vc,i));}

void virtuose::connect(string IP,float pTimeStep) {
    disconnect();
    VRPing ping;
    //ping.start(IP, port); // TODO: test it with right port
    cout << "Open virtuose " << IP << ", timestep delta: " << pTimeStep << endl;
    vc = virtOpen(IP.c_str());
    CHECK_INIT(vc);
    if (vc == 0) return;
    float identity[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};

    CHECK( virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION) );
    setSimulationScales(1.0f,1.0f);
    timestep = pTimeStep;
    CHECK( virtSetTimeStep(vc, timestep) );
    CHECK( virtSetBaseFrame(vc, identity) );
    CHECK( virtSetObservationFrame(vc, identity) );
    //CHECK( virtSetCommandType(vc, COMMAND_TYPE_IMPEDANCE) );
    CHECK( virtSetCommandType(vc, COMMAND_TYPE_VIRTMECH) );
    commandType = COMMAND_TYPE_VIRTMECH;
    CHECK( virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP) );

    enableForceFeedback(true);
    //float baseFrame[7] = { 0.0f, 0.0f, 0.0f, 0.70710678f, 0.0f, 0.70710678f, 0.0f };
    //virtActiveSpeedControl(vc, 0.04f, 10.0f);

    CHECK( virtSetPowerOn(vc, 1) );
    //virtSetPeriodicFunction(vc, callback, &timestep, this);
}

void virtuose::disconnect()
{
    if(vc == 0) return;

        CHECK( virtSetPowerOn(vc, 0) );
        CHECK( virtDetachVO(vc) );
        //CHECK( virtStopLoop(vc) );
        CHECK( virtClose(vc) );

        vc = 0;
        isAttached = false;
}

void virtuose::setSimulationScales(float translation, float forces)
{
    if(vc == 0) return;
    CHECK( virtSetSpeedFactor(vc, translation) );
    CHECK( virtSetForceFactor(vc, forces) );
}


void virtuose::applyForce(Vec3f force, Vec3f torque)
{
    if(vc == 0) return;
    float f[6] = { force[2], force[0], force[1], torque[2], torque[0], torque[1] };
    CHECK( virtAddForce(vc, f) );
}

Vec3f virtuose::getForce() {

    return totalForce;
}

Matrix virtuose::getPose()
{
    if(vc == 0) return Matrix().identity();
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


void virtuose::setBase(VRTransform* tBase) {
    base = tBase;
}


void virtuose::attachTransform(VRTransform* trans)
{
    if(vc == 0) return;

    isAttached = true;
    attached = trans;
    VRPhysics* o = trans->getPhysics();
    btMatrix3x3 t = o->getInertiaTensor();
    float inertia[9] {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    Matrix3ToArray(t,inertia);
    cout<<"\n "<<"\n "<<inertia[0] << "    " <<inertia[1] <<  "    " <<inertia[2] << "\n "<<inertia[3] <<  "    " <<inertia[4] <<  "    " <<inertia[5] << "\n "<<inertia[6] << "    " <<inertia[7] <<"    " << inertia[8]<<"\n ";
    CHECK(virtAttachVO(vc, o->getMass(), inertia));

}

void virtuose::fillPosition(VRPhysics* p, float *to, VRPhysics* origin)
{
    //no origin->take zero as origin
    btTransform pos = p->getTransform();
    if (origin != 0) {
        pos.setOrigin(( p->getTransform().getOrigin() - origin->getTransform().getOrigin()));
        pos.setRotation(p->getTransform().getRotation() * origin->getTransform().getRotation());
    }

    to[0] =  pos.getOrigin().getZ();
    to[1] = pos.getOrigin().getX();
    to[2] =  pos.getOrigin().getY();
    to[3] =  pos.getRotation().getZ();
    to[4] =  pos.getRotation().getX();
    to[5] =  pos.getRotation().getY();
    to[6] =  pos.getRotation().getW();
}
void virtuose::fillSpeed(VRPhysics* p, float *to,VRPhysics* origin)
{


    Vec3f vel = p->getLinearVelocity();
    if(origin!=0)   vel -= origin->getLinearVelocity();
    to[0] = vel.z();
    to[1] = vel.x();
    to[2] = vel.y();
    Vec3f ang = p->getAngularVelocity();
    if(origin!=0)   ang -= origin->getAngularVelocity();
    to[3] = ang.z();
    to[4] = ang.x();
    to[5] = ang.y();
}
void virtuose::Matrix3ToArray(btMatrix3x3 m, float *to)
{
    to[0] = m.getRow(0).getX();
    to[1] = m.getRow(0).getZ();
    to[2] = m.getRow(0).getY();
    to[3] = m.getRow(2).getX();
    to[4] = m.getRow(2).getZ();
    to[5] = m.getRow(2).getY();
    to[6] = m.getRow(1).getZ();
    to[7] = m.getRow(1).getX();
    to[8] = m.getRow(1).getY();



}



VRTransform* virtuose::detachTransform()
{
    VRTransform* ret = 0;
    if(vc == 0 || !isAttached) return ret;
    isAttached = false;
    CHECK(virtDetachVO(vc));
    ret = attached;
    attached = 0;
    return ret;

}

OSG::Vec3i virtuose::getButtonStates()
{
    if(vc == 0) return Vec3i(0,0,0);
    int i = 0;
    int j = 0;
    int k = 0;
    CHECK(virtGetButton(vc,0,&i));
    CHECK(virtGetButton(vc,1,&j));
    CHECK(virtGetButton(vc,2,&k));
    return Vec3i(i,j,k);
}

void virtuose::updateVirtMechPre() {

	if(vc == 0) return;

	float position[7] = {0.0,0.0,0.0,0.0,0.0,0.0,1.0};
	float speed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
	int shiftPos = 0;

	CHECK(virtIsInShiftPosition(vc,&shiftPos));
	if(commandType == COMMAND_TYPE_VIRTMECH) {

        if(!isAttached) {
            virtGetPosition(vc, position);
			virtSetPosition(vc, position);
			virtGetSpeed(vc, speed);
			virtSetSpeed(vc, speed);
		} else {
                //apply position&speed to the haptic
                VRPhysics* phBase = (base == 0) ? 0 : base->getPhysics();
                fillPosition(this->attached->getPhysics(),position,phBase);
                //"diff"
                float tmpPos[7];
                CHECK(virtGetPosition(vc, tmpPos));
                for(int i = 0; i < 7 ; i++) {
                    tmpPos[i] = (position[i] - tmpPos[i]);
                }
                pPos = Vec3f(tmpPos[0],tmpPos[1],tmpPos[2]);
                CHECK(virtSetPosition(vc, position));
                //speed
                fillSpeed(this->attached->getPhysics(),speed,phBase);
                //"diff"
                float tmpSp[6];
                CHECK(virtGetSpeed(vc, tmpSp));
                for(int i = 0; i < 6 ; i++) {
                    tmpSp[i] = (speed[i] - tmpSp[i]);
                }
                sPos = Vec3f(tmpSp[0],tmpSp[1],tmpSp[2]);
                sRot = Vec3f(tmpSp[0],tmpSp[1],tmpSp[2]);
                CHECK(virtSetSpeed(vc, speed));

                int power = 0;
                CHECK(virtGetPowerOn(vc,&power));

                if (power==0 || shiftPos == 1) {
                    CHECK(virtGetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                    CHECK(virtGetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));
                    CHECK(virtSetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                    CHECK(virtSetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));

                } else {
                    CHECK(virtSetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                    gripperSpeed = 0.0f;
                    CHECK(virtSetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));
                }
		}
	}
}
void virtuose::updateVirtMechPost() {
	if(vc == 0) return;

	float force[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
	int shiftPos = 0;

	CHECK(virtIsInShiftPosition(vc,&shiftPos));
	if(commandType == COMMAND_TYPE_VIRTMECH) {
		if (isAttached) {
			//get force applied by human on the haptic
			CHECK(virtGetForce(vc, force));
            //position +1, +2, +0
			Vec3f frc = Vec3f(force[1], force[2], force[0]);
			totalForce = frc;
			//rotation +4 +5 +3    (x-Achse am haptik: force[4])(y-Achse am haptik: force[5])(z-Achse am haptik: force[3])
			Vec3f trqu = Vec3f( force[4], force[5], force[3]);
			//apply force on the object
            //avoiding build-ups
                if( (pPos.length() < 0.1f) && (sPos.length() < 0.5f) &&  (sRot.length() < 0.5f)) {
                   attached->getPhysics()->addForce(frc);
                   attached->getPhysics()->addTorque(trqu);
				}
			}
		}



}



OSG_END_NAMESPACE;
