#include "VRConstraint.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRStorage_template.h"

#include <OpenSG/OSGQuaternion.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRConstraint::VRConstraint() {
    local = false;
    for (int i=0; i<6; i++) { min[i] = 0.0; max[i] = 0.0; }

    store("cT", &tConstraint);
    store("cR", &rConstraint);
    store("cT_mode", &tConMode);
    store("cR_mode", &rConMode);
    store("cT_local", &localTC);
    store("cR_local", &localRC);
    store("active", &active);
}

VRConstraint::~VRConstraint() {;}

VRConstraintPtr VRConstraint::create() { return VRConstraintPtr( new VRConstraint() ); }

void VRConstraint::setLocal(bool b) { local = b; }
bool VRConstraint::isLocal() { return local; }

void VRConstraint::setMinMax(int i, float f1, float f2) { min[i] = f1; max[i] = f2; }
void VRConstraint::setMin(int i, float f) { min[i] = f; }
void VRConstraint::setMax(int i, float f) { max[i] = f; }

float VRConstraint::getMin(int i) { return min[i]; }
float VRConstraint::getMax(int i) { return max[i]; }

void VRConstraint::setReferenceA(Matrix m) { refMatrixA = m; };
void VRConstraint::setReferenceB(Matrix m) { refMatrixB = m; refMatrixB.inverse(refMatrixBI); };
Matrix VRConstraint::getReferenceA() { return refMatrixA; };
Matrix VRConstraint::getReferenceB() { return refMatrixB; };

void VRConstraint::apply(VRTransformPtr obj) {
    if (!hasConstraint() || !active) return;
    auto now = VRGlobals::get()->CURRENT_FRAME;
    if (apply_time_stamp == now) return;
    apply_time_stamp = now;

    VRTransformPtr ref = Referential.lock();

    Matrix t, tr, tri, t0i;
    Matrix t0 = Reference;
    if (localTC) t = obj->getMatrix();
    else t = obj->getWorldMatrix();

    if (ref) {
        tr = ref->getWorldMatrix();
        tr.inverse(tri);
        t.multLeft(tri);
    }

    //rotation
    if (rConMode != NONE) {
        if (rConMode == POINT) { for (int i=0;i<3;i++) t[i] = t0[i]; } // TODO: add RConstraint as offset to referential and reference

        if (rConMode == LINE) {
            t.multLeft(rotRebase);
            t.mult(refRebasedI);
            float a = atan2(-t[2][1], t[2][2]);
            t.setRotate( Quaternion(Vec3f(1,0,0), atan2(-t[2][1], t[2][2])) );
            t.mult(refRebased);
            t.multLeft(rotRebaseI);
        }

        if (rConMode == PLANE); // TODO
    }

    //translation
    if (tConMode != NONE) {
        if (tConMode == PLANE) {
            float d = Vec3f(t[3] - t0[3]).dot(tConstraint);
            for (int i=0; i<3; i++) t[3][i] -= d*tConstraint[i];
        }

        if (tConMode == LINE) {
            Vec3f d = Vec3f(t[3] - t0[3]);
            d = d.dot(tConstraint)*tConstraint;
            //cout << "t0 " << t0[3] << " t " << t[3] << " a " << tConstraint << " d " << d << endl;
            for (int i=0; i<3; i++) t[3][i] = t0[3][i] + d[i];
        }

        if (tConMode == POINT) {
            for (int i=0; i<3; i++) t[3][i] = tConstraint[i];
        }
    }

    t.multLeft(refMatrixB);
    t.mult(refMatrixBI);

    if (ref) t.multLeft(tr);
    if (localTC) obj->setMatrix(t);
    else obj->setWorldMatrix(t);
}


void VRConstraint::setReference(Matrix m) { Reference = m; prepare(); }
void VRConstraint::setReferential(VRTransformPtr t) { Referential = t; }

void VRConstraint::setTConstraint(Vec3f trans, int mode, bool local) {
    tConstraint = trans;
    if (tConstraint.length() > 1e-4 && mode != POINT) tConstraint.normalize();
    tConMode = mode;
    localTC = local;
}

void VRConstraint::setRConstraint(Vec3f rot, int mode, bool local) {
    rConstraint = rot;
    if (rConstraint.length() > 1e-4 && mode != POINT) rConstraint.normalize();
    rConMode = mode;
    localRC = local;

    Quaternion q(Vec3f(0, -rot[2], -rot[1]), -acos(rot[0]));
    rotRebase.setIdentity();
    rotRebase.setRotate(q);
    rotRebase.inverse(rotRebaseI);
    prepare();
}

void VRConstraint::prepare() {
    refRebased = Reference;
    refRebased.multLeft(rotRebase);
    refRebased.inverse(refRebasedI);
}

bool VRConstraint::getTMode() { return tConMode; }
bool VRConstraint::getRMode() { return rConMode; }
Vec3f VRConstraint::getTConstraint() { return tConstraint; }
Vec3f VRConstraint::getRConstraint() { return rConstraint; }

bool VRConstraint::hasTConstraint() { return tConMode != NONE; }
bool VRConstraint::hasRConstraint() { return rConMode != NONE; }
bool VRConstraint::hasConstraint() { return rConMode != NONE || tConMode != NONE; }

void VRConstraint::setActive(bool b, VRTransformPtr obj) { active = b; if (b) obj->getWorldMatrix(Reference); obj->setFixed(!b); prepare(); }
bool VRConstraint::isActive() { return active; }

OSG_END_NAMESPACE;
