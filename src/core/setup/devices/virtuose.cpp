#include "virtuose.h"
#include <virtuose/virtuoseAPI.h>
#include <OpenSG/OSGQuaternion.h>
#include "core/networking/VRPing.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/system/VRSystem.h"

#define CHECK(x) { \
    int result = (x); \
    if (result) { \
        cout << "\nRuntime error: " << #x << " returned " << result << " at " << __FILE__ ":" << __LINE__ << endl; \
        if (vc) { \
            cout << " err code: " << virtGetErrorCode(vc) << endl; \
            /*char* m = virtGetErrorMessage(virtGetErrorCode(vc)); \
            if (m) cout << "  err msg: " << m[0] << endl; \
            else cout << "  err msg invalid " << endl;*/ \
        } \
        else cout << " vc is 0\n"; \
    } \
}

#define CHECK_INIT(x) {\
    if (!x) { \
        cout << "\nRuntime error: virtOpen returned 0 at " << __FILE__ ":" << __LINE__ << endl; \
        if (false) cout << "\n err msg: " << virtGetErrorMessage(virtGetErrorCode(x)) << endl; \
    } \
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
Vec3d(y,z,x);
virtVec(z,x,y);

*/

template <typename T>
void print(const T& t, int N) {
    cout << " trans: ";
    for (int i=0; i<N; i++) cout << t.data[i] << " ";
    cout << endl;
}

virtuose::virtuose() : interface("virtuose") {
    interface.addBarrier("barrier1", 2);
    interface.addBarrier("barrier2", 2);
    //interface.addBarrier("barrier3", 2);
    interface.addObject<Vec6>("targetForces");
    string projectPath = VRSceneManager::get()->getOriginalWorkdir() + "/src/core/setup/devices/virtuose/";
    deamonPath = projectPath+"bin/Debug/virtuose";
    if (!exists(deamonPath)) deamonPath = projectPath+"bin/Release/virtuose";

    if (!exists(deamonPath)) {
        compileCodeblocksProject(projectPath + "virtuose.cbp"); // not working :(
    }

    if (!exists(deamonPath)) return;

    deamon = popen(deamonPath.c_str(), "w");
    if (!deamon) { cout << " failed to open virtuose deamon" << endl; return; }
    cout << "\nstarted haptic device deamon" << endl;
}

virtuose::~virtuose() {
    cout << "virtuose::~virtuose\n";
    disconnect();
}

bool virtuose::connected() { return interface.hasObject<Vec7>("position"); }

string virtuose::getDeamonState() {
    if (connected() && deamon) return "running";
    if (!exists(deamonPath)) return "not compiled";
    return "not running";
}

string virtuose::getDeviceState() {
    return "not connected";
}

void virtuose::enableForceFeedback(bool enable) {
    if(!connected()) return;
    int i = (enable ? 1 : 0);
    //CHECK(virtEnableForceFeedback(vc,i)); // TODO
}

string GetEnv( const string & var ) {
     const char * val = ::getenv( var.c_str() );
     return val ? val : "";
}

void virtuose::connect(string IP,float pTimeStep) { // TODO
    commandType = COMMAND_TYPE_VIRTMECH;
    if (!connected()) return;
    //disconnect();
    //VRPing ping;
    //ping.start(IP, port); // TODO: test it with right port
    cout << "\nOpen virtuose " << IP << ", timestep delta: " << pTimeStep << endl;
    //cout << " LD_LIBRARY_PATH: " << GetEnv("LD_LIBRARY_PATH") << endl;
    /*vc = virtOpen(IP.c_str());
    CHECK_INIT(vc);
    if (vc == 0) {
        cout << "Open virtuose failed!" << endl;
        return;
    }
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

    auto a = virtEnableForceFeedback(vc, 1);
    auto b = virtSetPowerOn(vc, 1);
    cout << " virtuose::connect, enable forcefeedback, a b " << a << " " << b << endl;*/

    //enableForceFeedback(true);
    //float baseFrame[7] = { 0.0f, 0.0f, 0.0f, 0.70710678f, 0.0f, 0.70710678f, 0.0f };
    //virtActiveSpeedControl(vc, 0.04f, 10.0f);

    //CHECK( virtSetPowerOn(vc, 1) );
    //virtSetPeriodicFunction(vc, callback, &timestep, this);
}

void virtuose::disconnect() { // TODO
    /*interface.setObject<bool>("run", false);
    pclose(deamon);
    isAttached = false;
    cout << "virtuose::disconnect!" << endl;*/
}

void virtuose::setSimulationScales(float translation, float forces) { // TODO
    if(!connected()) return;
    //CHECK( virtSetSpeedFactor(vc, translation) );
    //CHECK( virtSetForceFactor(vc, forces) );
    //cout << " virtuose::setSimulationScales " << translation << " " << forces << endl;
}


void virtuose::applyForce(Vec3d force, Vec3d torque) {
    if(!connected()) return;
    //forces.data = { float(force[2]), float(force[0]), float(force[1]), float(torque[2]), float(torque[0]), float(torque[1]) };
    targetForces.data[0] = float(force[2]);
    targetForces.data[1] = float(force[0]);
    targetForces.data[2] = float(force[1]);
    targetForces.data[3] = float(torque[2]);
    targetForces.data[4] = float(torque[0]);
    targetForces.data[5] = float(torque[1]);
    interface.setObject<Vec6>("targetForces", targetForces);
    //CHECK( virtAddForce(vc, f) );
}

Vec3d virtuose::getForce() { return totalForce; }

Matrix4d virtuose::getPose(float f[7]) {
    Matrix4d m;
    Vec3d pos(f[1], f[2], f[0]);
    Quaterniond q;
    q.setValue(f[4], f[5], f[3]);
    m.setRotate(q);
    m.setTranslate(pos);
    return m;
}

Matrix4d virtuose::getPose() {
    auto data = interface.getObject<Vec7>("position").data;
    return getPose(data);
}

void virtuose::setBase(VRTransformPtr tBase) { base = tBase; }

void virtuose::attachTransform(VRTransformPtr trans) {
    //if(!connected()) return;
    isAttached = true;
    attached = trans;
    VRPhysics* o = trans->getPhysics();
    btMatrix3x3 t = o->getInertiaTensor();
    Vec9 inertia;
    Matrix3ToArray(t,inertia.data);
    print(inertia, 9);
    //cout<<"\n virtuose::attachTransform:\n " << inertia[0] << "    " <<inertia[1] <<  "    " <<inertia[2] << "\n "<<inertia[3] <<  "    " <<inertia[4] <<  "    " <<inertia[5] << "\n "<<inertia[6] << "    " <<inertia[7] <<"    " << inertia[8]<<"\n ";
    cout<<"\n virtuose::attachTransform:\n ";
    interface.setObject<Vec9>("inertia", inertia);
    interface.setObject<float>("mass", o->getMass());
    interface.setObject<bool>("doAttach", true);
    //CHECK(virtAttachVO(vc, o->getMass(), inertia));
}

void virtuose::fillPosition(VRPhysics* p, float *to, VRPhysics* origin) {
    //no origin->take zero as origin
    btTransform pos = p->getTransform();
    if (origin != 0) {
        pos.setOrigin(( p->getTransform().getOrigin() - origin->getTransform().getOrigin()));
        pos.setRotation(p->getTransform().getRotation() * origin->getTransform().getRotation());
    }

    to[0] = pos.getOrigin().getZ();
    to[1] = pos.getOrigin().getX();
    to[2] = pos.getOrigin().getY();
    to[3] = pos.getRotation().getZ();
    to[4] = pos.getRotation().getX();
    to[5] = pos.getRotation().getY();
    to[6] = pos.getRotation().getW();

    //cout << " virtuose::fillPosition " << to[0] << " " << to[1] << " " << to[2] << endl;
}

void virtuose::fillSpeed(VRPhysics* p, float* to, VRPhysics* origin) {
    Vec3d vel = p->getLinearVelocity();
    if (origin!=0) vel -= origin->getLinearVelocity();
    to[0] = vel.z();
    to[1] = vel.x();
    to[2] = vel.y();
    Vec3d ang = p->getAngularVelocity();
    if (origin!=0) ang -= origin->getAngularVelocity();
    to[3] = ang.z();
    to[4] = ang.x();
    to[5] = ang.y();
}

void virtuose::Matrix3ToArray(btMatrix3x3 m, float *to) {
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

VRTransformPtr virtuose::detachTransform() { // TODO
    VRTransformPtr ret = 0;
    /*if(!connected() || !isAttached) return ret;
    isAttached = false;
    CHECK(virtDetachVO(vc));
    ret = attached;
    attached = 0;*/
    return ret;
}

OSG::Vec3i virtuose::getButtonStates() { // TODO
    Vec3i buttons;
    if (!connected()) return buttons;
    /*CHECK(virtGetButton(vc,0,&buttons[0]));
    CHECK(virtGetButton(vc,1,&buttons[1]));
    CHECK(virtGetButton(vc,2,&buttons[2]));*/
    return buttons;
}

bool sync = false;

void virtuose::updateVirtMechPre() {
	if (!connected()) return;

	float position[7] = {0.0,0.0,0.0,0.0,0.0,0.0,1.0};
	float speed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};

	bool shifting = interface.getObject<bool>("shifting", false);
    if (shifting) return;

	if (commandType == COMMAND_TYPE_VIRTMECH) {
        if (!isAttached) { // TODO
            //virtGetPosition(vc, position);
			//auto a = virtSetPosition(vc, position);
			//virtGetSpeed(vc, speed);
			//auto b = virtSetSpeed(vc, speed);
            //cout << "virtuose::updateVirtMechPre set zero speed and position! " << a << " " << b << endl;
		} else {
            // apply position&speed to the haptic
            VRPhysics* phBase = (base == 0) ? 0 : base->getPhysics();
            fillPosition(this->attached->getPhysics(), targetPosition.data, phBase);
            fillSpeed(this->attached->getPhysics(), targetSpeed.data, phBase);

            interface.waitAt("barrier1");
            interface.setObject<Vec7>("targetPosition", targetPosition);
            interface.setObject<Vec6>("targetSpeed", targetSpeed);
            interface.waitAt("barrier1");

            sync = true;

            /*if (shifting) { // TODO
                CHECK(virtGetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                CHECK(virtGetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));
                CHECK(virtSetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                CHECK(virtSetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));
            } else {
                CHECK(virtSetArticularPositionOfAdditionalAxe(vc,&gripperPosition));
                gripperSpeed = 0.0f;
                CHECK(virtSetArticularSpeedOfAdditionalAxe(vc,&gripperSpeed));
            }*/
		}
	}
}

void virtuose::updateVirtMechPost() {
	if (!connected()) return;

	float force[6] = {0.0,0.0,0.0,0.0,0.0,0.0};

	bool shifting = interface.getObject<bool>("shifting", false);
    bool doPhysUpdate = interface.getObject<bool>("doPhysUpdate");

    if (isAttached) attached->getPhysics()->pause(shifting);
    if (shifting) return;

    interface.waitAt("barrier2");
    interface.waitAt("barrier2");

    if (!doPhysUpdate) return;

	if (commandType == COMMAND_TYPE_VIRTMECH) {
		if (isAttached && interface.hasObject<Vec6>("forces")) {

            Vec6 forces = interface.getObject<Vec6>("forces"); //get force applied by human on the haptic
            auto& f = forces.data;
			Vec3d frc = Vec3d( f[1], f[2], f[0] ); // position +1 +2 +0
			Vec3d trqu = Vec3d( f[4], f[5], f[3] ); // rotation +4 +5 +3    (x-Achse am haptik: force[4])(y-Achse am haptik: force[5])(z-Achse am haptik: force[3])
			totalForce = frc;
            attached->getPhysics()->addForce(frc);
            attached->getPhysics()->addTorque(trqu);
        }
    }
}

OSG_END_NAMESPACE;



