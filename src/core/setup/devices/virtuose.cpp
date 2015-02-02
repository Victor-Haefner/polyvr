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

virtuose::virtuose()
{
    isAttached = false;
    gripperPosition = 0.0f;
    gripperSpeed = 0.0f;

}

virtuose::~virtuose()
{
    disconnect();
}

bool virtuose::connected()
{
    return (vc != 0);
}

bool checkVirtuoseIP(string IP)   // TODO: try if haptic is found -> right port?
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK); // TODO: try blocking but with timeout
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(3131);  // Could be anything
    inet_pton(AF_INET, IP.c_str(), &sin.sin_addr);

    if (connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
    {
        printf("No haptic device at IP %s: %d (%s)\n", IP.c_str(), errno, strerror(errno));
        return false;
    }
    return true;
}

void virtuose::connect(string IP)
{
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

void virtuose::disconnect()
{
    if (vc)
    {
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

void virtuose::setSimulationScales(float translation, float forces)
{
    CHECK( virtSetSpeedFactor(vc, translation) );
    CHECK( virtSetForceFactor(vc, forces) );
}


void virtuose::applyForce(Vec3f force, Vec3f torque)
{
    float f[6] = { force[2], force[0], force[1], torque[2], torque[0], torque[1] };
    CHECK( virtAddForce(vc, f) );
}

Matrix virtuose::getPose()
{
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




void virtuose::attachTransform(VRTransform* trans)
{
    isAttached = true;
    attached = trans;
    VRPhysics* o = trans->getPhysics();
    btMatrix3x3 t = o->getInertiaTensor();
    float inertia[9] {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    Matrix3ToArray(t,inertia);
    cout<<"\n "<<"\n "<<inertia[0] << "    " <<inertia[1] <<  "    " <<inertia[2] << "\n "<<inertia[3] <<  "    " <<inertia[4] <<  "    " <<inertia[5] << "\n "<<inertia[6] << "    " <<inertia[7] <<"    " << inertia[8]<<"\n ";
    CHECK(virtAttachVO(vc, o->getMass(), inertia));

}

void virtuose::fillPosition(VRPhysics* p, float *to)
{
    btTransform pos = p->getTransform();
    to[0] =  pos.getOrigin().getZ();
    to[1] = pos.getOrigin().getX();
    to[2] =  pos.getOrigin().getY();
    /** not supported **/
    //to[3] =  -pos.getRotation().getY();
    //to[4] =  -pos.getRotation().getW();
    //to[5] =  -pos.getRotation().getZ();
    //to[6] =  pos.getRotation().getX();
}
void virtuose::fillSpeed(VRPhysics* p, float *to)
{
    Vec3f vel = p->getLinearVelocity();
    to[0] = vel.z();
    to[1] = vel.x();
    to[2] = vel.y();
    /**not supported **/
    //Vec3f ang = p->getAngularVelocity();
    //to[3] = -ang.x();
    //to[4] = -ang.z();
    //to[5] = -ang.y();
}
void virtuose::Matrix3ToArray(btMatrix3x3 m, float *to)
{
    to[0] = m.getRow(0).getZ();
    to[1] = m.getRow(0).getX();
    to[2] = m.getRow(0).getY();
    to[3] = m.getRow(2).getZ();
    to[4] = m.getRow(2).getX();
    to[5] = m.getRow(2).getY();
    to[6] = m.getRow(1).getZ();
    to[7] = m.getRow(1).getX();
    to[8] = m.getRow(1).getY();



}



void virtuose::detachTransform()
{
    isAttached = false;
    CHECK(virtDetachVO(vc));
    attached = 0;

}

OSG::Vec3i virtuose::getButtonStates()
{
    int i = 0;
    int j = 0;
    int k = 0;
    CHECK(virtGetButton(vc,0,&i));
    CHECK(virtGetButton(vc,1,&j));
    CHECK(virtGetButton(vc,2,&k));
    return Vec3i(i,j,k);
}

void virtuose::updateVirtMech()
{

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

                virtGetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                virtGetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
                virtSetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                virtSetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
        }
        else
        {
        /** rotation <-> torque with haptic <-> attachedObject is not supported yet. haptic's rotation is set free instead**/
                float ts = 0.0;
                CHECK(virtGetTimeStep(vc, &ts));

            //apply position&speed to the haptic

                fillPosition(this->attached->getPhysics(),position);

            //"diff"
                float tmpPos[7];
                CHECK(virtGetPosition(vc, tmpPos));
                for(int i = 0; i < 7 ; i++) {
                    tmpPos[i] = (position[i] - tmpPos[i]);
                    //cout << tmpPos[i];
                }
                //cout << "\n";
                Vec3f pPos = Vec3f(tmpPos[0],tmpPos[1],tmpPos[2]);
            /* bugfixing and logging
                Quaternion pRot;
                pRot.setValue(tmpPos[4], tmpPos[5], tmpPos[3]);
                cout << setprecision(4);
                cout << fixed;
                cout << "posdiff: " << pPos.length() << "  "  << " rotdiff: " << pRot.length();
               */

            /**free rotation **/
                CHECK(virtGetPosition(vc, tmpPos));
                for(int i = 3; i < 7;i++ ) {
                   position[i] = tmpPos[i];
                }


                CHECK(virtSetPosition(vc, position));

            //speed
                fillSpeed(this->attached->getPhysics(),speed);
                //timestep bullet -> haptic
                for(int i = 0; i < 6; i++)
                {
                    speed[i] *= dt;
                    speed[i] *= (1/ts);
                }
            //"diff"
                float tmpSp[6];
                CHECK(virtGetSpeed(vc, tmpSp));
                for(int i = 0; i < 6 ; i++) {
                    tmpSp[i] = (speed[i] - tmpSp[i]);
                    //cout << tmpSp[i];
                }
                Vec3f sPos = Vec3f(tmpSp[0],tmpSp[1],tmpSp[2]);
                Vec3f sRot = Vec3f(tmpSp[0],tmpSp[1],tmpSp[2]);

            /* bugfixing and logging
                cout << setprecision(4);
                cout << fixed;
                cout <<"   " <<  "speeddiff: "<<  sPos.length() << "  "  << " rotspeeddiff: " << sRot.length();

                cout << "\n";
                */

            /**free rotation **/
                CHECK(virtGetSpeed(vc, tmpSp));
                for(int i = 3; i < 6;i++ ) {
                   speed[i] = tmpSp[i];
                }

                CHECK(virtSetSpeed(vc, speed));



            //get force applied by human on the haptic
                CHECK(virtGetForce(vc, force));

                //timestep haptic -> haptic
                for(int i = 0; i < 6; i++)
                {
                    force[i] *= ts;
                    force[i] *= ts;
                    force[i] *= (1/dt);
                    force[i] *= (1/dt);
                }

                Vec3f frc = Vec3f(force[1], force[2], force[0]);

            /** not supported**/
                //Vec3f trqu = Vec3f(-force[3],-force[5],-force[4]);


            //apply force on the object
                if(power == 0)
                {
                    if( (pPos.length() < 0.1f) && (sPos.length() < 0.5f) &&  (sRot.length() < 0.5f)) {
                        attached->getPhysics()->addForce(frc);
                        /** not supported**/
                        //attached->getPhysics()->addTorque(trqu);
                    } else {
                        attached->getPhysics()->resetForces();
                        float newSpeed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
                        CHECK(virtSetSpeed(vc, newSpeed));
                    }
                }

            //copypaste from demosrc
                int m_power = 0;
                virtGetPowerOn(vc, &m_power);
                virtIsInShiftPosition(vc, &power);
                if ((m_power == 0) || (power == 1))
                {
                    virtGetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                    virtGetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
                    virtSetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                    virtSetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
                }
                else
                {
                    /* mode bloqué */
                    virtSetArticularPositionOfAdditionalAxe(vc, &gripperPosition);
                    gripperSpeed = 0.0;
                    virtSetArticularSpeedOfAdditionalAxe(vc, &gripperSpeed);
                }

        }
    }


}


OSG_END_NAMESPACE;
