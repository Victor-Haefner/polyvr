#include "VRConstraint.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"

#include <OpenSG/OSGQuaternion.h>

using namespace OSG;

template<> string typeName(const VRConstraint::TCMode& o) { return "Constraint mode"; }
template<> int toValue(stringstream& s, VRConstraint::TCMode& t) { return 0; } // TODO

VRConstraint::VRConstraint() {
    local = false;
    for (int i=0; i<6; i++) { min[i] = 0.0; max[i] = 0.0; }

    //store("min", &min);
    //store("max", &max);
    //store("refMatrixA", &refMatrixA);
    //store("refMatrixB", &refMatrixB);
    //store("active", &active);
}

VRConstraint::~VRConstraint() {;}

VRConstraintPtr VRConstraint::create() { return VRConstraintPtr( new VRConstraint() ); }
VRConstraintPtr VRConstraint::duplicate() { auto c = create(); *c = *this; return c; }

void VRConstraint::setLocal(bool b) { local = b; }
void VRConstraint::setActive(bool b) { active = b; }
bool VRConstraint::isLocal() { return local; }
bool VRConstraint::isActive() { return active; }

void VRConstraint::setMinMax(int i, float f1, float f2) { min[i] = f1; max[i] = f2; }
void VRConstraint::setMin(int i, float f) { min[i] = f; }
void VRConstraint::setMax(int i, float f) { max[i] = f; }
float VRConstraint::getMin(int i) { return min[i]; }
float VRConstraint::getMax(int i) { return max[i]; }

void VRConstraint::lock(vector<int> dofs) { for (int dof : dofs) setMinMax(dof,0,0); setActive(true); }
void VRConstraint::free(vector<int> dofs) { for (int dof : dofs) setMinMax(dof,1,-1); }

void VRConstraint::setReferenceA(PosePtr p) { refMatrixA = p->asMatrix(); refMatrixA.inverse(refMatrixAI); };
void VRConstraint::setReferenceB(PosePtr p) { refMatrixB = p->asMatrix(); refMatrixB.inverse(refMatrixBI); };
void VRConstraint::setReference(PosePtr p) { setReferenceA(p); }
PosePtr VRConstraint::getReferenceA() { return Pose::create(refMatrixA); };
PosePtr VRConstraint::getReferenceB() { return Pose::create(refMatrixB); };

void VRConstraint::lockRotation() { setRConstraint(Vec3d(0,0,0), VRConstraint::POINT); }

void VRConstraint::setReferential(VRTransformPtr t) { Referential = t; }

void VRConstraint::setTConstraint(Vec3d params, TCMode mode, bool local) {
    if (params.length() > 1e-4 && mode != POINT) params.normalize();
    this->local = local;
    active = true;

    if (mode == POINT) {
        setMinMax(0, params[0], params[0]);
        setMinMax(1, params[1], params[1]);
        setMinMax(2, params[2], params[2]);
    }

    if (mode == LINE) {
        auto p = Vec3d(refMatrixB[3]);
        lock({0,1});
        free({2});
        setReferenceB( Pose::create(p, params) ); // TODO: will not work for vertical line!
    }

    if (mode == PLANE) {
        auto p = Vec3d(refMatrixB[3]);
        lock({1});
        free({0,2});
        setReferenceB( Pose::create(p, Vec3d(1,0,0), params) ); // TODO: will not work for plane with x as normal!
    }
}

void VRConstraint::setRConstraint(Vec3d params, TCMode mode, bool local) {
    if (params.length() > 1e-4 && mode != POINT) params.normalize();
    this->local = local;
    active = true;

    if (mode == POINT) {
        setMinMax(3, params[0], params[0]);
        setMinMax(4, params[1], params[1]);
        setMinMax(5, params[2], params[2]);
    }

    if (mode == LINE) {
        auto p = Vec3d(refMatrixB[3]);
        lock({3,4});
        free({5});
        setReferenceB( Pose::create(p, params) ); // TODO: will not work for vertical line!
    }

    if (mode == PLANE) {
        auto p = Vec3d(refMatrixB[3]);
        lock({4});
        free({3,5});
        setReferenceB( Pose::create(p, Vec3d(1,0,0), params) ); // TODO: will not work for plane with x as normal!
    }
}

void VRConstraint::apply(VRTransformPtr obj, VRObjectPtr parent) {
    if (!active) return;
    auto now = VRGlobals::CURRENT_FRAME;
    if (apply_time_stamp == now) return;
    apply_time_stamp = now;

    if (local) parent = obj->getParent(true);
    Matrix4d t = obj->getMatrixTo(parent);
    t.mult(refMatrixB);
    t.mult(refMatrixAI);

    cout << "VRConstraint::apply " << obj->getName();
    if (parent) cout << " p: " << parent->getName();
    cout << " Mt: " << refMatrixB[3] << " l: " << local << endl;
    //cout << "VRConstraint::apply " << obj->getName() << " t " << min[0] << "/" << max[0] << " , " << min[1] << "/" << max[1] << " , " << min[2] << "/" << max[2] << endl;

    for (int i=0; i<3; i++) { // translation
        if (min[i] > max[i]) continue; // free
        if (min[i] > t[3][i]) t[3][i] = min[i]; // lower bound
        if (max[i] < t[3][i]) t[3][i] = max[i]; // upper bound
    }

    Vec3d angles = VRTransform::computeEulerAngles(t);
    Vec3d angleDiff;
    for (int i=3; i<6; i++) { // rotation
        if (min[i] > max[i]) continue; // free
        if (min[i] > angles[i-3]) angleDiff[i-3] = min[i] - angles[i-3]; // lower bound
        if (max[i] < angles[i-3]) angleDiff[i-3] = max[i] - angles[i-3]; // upper bound
    }
    //VRTransform::applyEulerAngles(t, angles + angleDiff);
    Matrix4d R1, R2;
    VRTransform::applyEulerAngles(R1, angles);
    VRTransform::applyEulerAngles(R2, angles+angleDiff);
    R1.invert();

    cout << " T:    " << Vec3d(t[3]) << endl;
    t.mult(refMatrixA);
    cout << " A  -> " << Vec3d(t[3]) << " ( " << VRTransform::computeEulerAngles(refMatrixA) << " ) " << endl;
    t.mult(R1);
    cout << " RI -> " << Vec3d(t[3]) << " ( " << VRTransform::computeEulerAngles(R1) << " ) " << endl;
    t.mult(refMatrixBI);
    cout << " BI -> " << Vec3d(t[3]) << " ( " << VRTransform::computeEulerAngles(refMatrixBI) << " ) " << endl;
    t.mult(R2);
    cout << " R  -> " << Vec3d(t[3]) << " ( " << VRTransform::computeEulerAngles(R2) << " ) " << endl;
    obj->setMatrixTo(t, parent);
}






