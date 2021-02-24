#include "VRConstraint.h"
#include "core/objects/VRTransform.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
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

void VRConstraint::lock(vector<int> dofs, float v) { for (int dof : dofs) setMinMax(dof,v,v); setActive(true); }
void VRConstraint::free(vector<int> dofs) { for (int dof : dofs) setMinMax(dof,1,-1); setActive(true); }
bool VRConstraint::isLocked(int i) { return bool(min[i] > max[i]); }

void VRConstraint::setReferenceA(PosePtr p) { refMatrixA = p->asMatrix(); refMatrixA.inverse(refMatrixAI); };
void VRConstraint::setReferenceB(PosePtr p) { refMatrixB = p->asMatrix(); refMatrixB.inverse(refMatrixBI); };
void VRConstraint::setReference(PosePtr p) { setReferenceA(p); }
PosePtr VRConstraint::getReferenceA() { return Pose::create(refMatrixA); };
PosePtr VRConstraint::getReferenceB() { return Pose::create(refMatrixB); };

void VRConstraint::lockRotation() { lock({3,4,5}); }

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
        auto p = Vec3d(refMatrixA[3]);
        lock({0,1});
        free({2});
        auto po = Pose::create(p, params);
        po->makeUpOrthogonal();
        setReferenceA( po );
    }

    if (mode == PLANE) {
        auto p = Vec3d(refMatrixA[3]);
        lock({1});
        free({0,2});
        auto po = Pose::create(p, Vec3d(refMatrixA[2]), params);
        po->makeDirOrthogonal();
        setReferenceA( po );
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
        auto p = Vec3d(refMatrixA[3]);
        lock({3,4});
        free({5});
        auto po = Pose::create(p, params);
        po->makeUpOrthogonal();
        setReferenceA( po );
    }

    if (mode == PLANE) {
        auto p = Vec3d(refMatrixA[3]);
        lock({4});
        free({3,5});
        auto po = Pose::create(p, Vec3d(refMatrixA[2]), params);
        po->makeDirOrthogonal();
        setReferenceA( po );
    }
}

// called from VRTransform::apply_constraints
void VRConstraint::apply(VRTransformPtr obj, VRObjectPtr parent, bool force) {
    if (!active) return;
#ifndef WITHOUT_BULLET
    if (obj->getPhysics()->isPhysicalized()) return;
#endif

    auto now = VRGlobals::CURRENT_FRAME;
    if (apply_time_stamp == now && !force) return;
    apply_time_stamp = now;

    if (local) parent = obj->getParent(true);
    if (auto r = Referential.lock()) parent = r;

    Matrix4d J;
    if (parent) J = parent->getMatrixTo(obj);
    else J = obj->getWorldMatrix();
    J.mult(refMatrixB);
    J.multLeft(refMatrixAI);

    for (int i=0; i<3; i++) { // translation
        if (min[i] > max[i]) continue; // free
        if (min[i] > J[3][i]) J[3][i] = min[i]; // lower bound
        if (max[i] < J[3][i]) J[3][i] = max[i]; // upper bound
    }

    Vec3d angles = VRTransform::computeEulerAngles(J);

    auto sign = [](float a) {
        return a<0?-1:1;
    };

    // TODO: this is not correct, for example [180, 20, 180], corresponds to [0, 160, 0], and not [0, 20, 0] !!
    //  this tries to fix it somewhat, but its not clean!
    if ( abs(angles[0]) > Pi*0.5 && abs(angles[2]) > Pi*0.5) {
        angles[0] -= sign(angles[0])*Pi;
        angles[2] -= sign(angles[2])*Pi;
        angles[1] = Pi - angles[1];
    }

    Vec3d angleDiff;
    for (int i=3; i<6; i++) { // rotation
        if (min[i] > max[i]) continue; // free
        float a = angles[i-3];
        float d1 = min[i]-a; while(d1 > Pi) d1 -= 2*Pi; while(d1 < -Pi) d1 += 2*Pi;
        float d2 = max[i]-a; while(d2 > Pi) d2 -= 2*Pi; while(d2 < -Pi) d2 += 2*Pi;
        if (d1 > 0 && abs(d1) <= abs(d2)) angleDiff[i-3] = d1; // lower bound
        if (d2 < 0 && abs(d2) <= abs(d1)) angleDiff[i-3] = d2; // upper bound
    }
    VRTransform::applyEulerAngles(J, angles + angleDiff);

    J.multLeft(refMatrixA);
    J.mult(refMatrixBI);
    obj->setMatrixTo(J, parent);
}






