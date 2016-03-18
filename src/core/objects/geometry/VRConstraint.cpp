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
    store("do_cT", &doTConstraint);
    store("do_cR", &doRConstraint);
    store("cT_mode", &tConMode);
    store("cT_local", &localTC);
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
void VRConstraint::setReferenceB(Matrix m) { refMatrixB = m; };
Matrix VRConstraint::getReferenceA() { return refMatrixA; };
Matrix VRConstraint::getReferenceB() { return refMatrixB; };

void VRConstraint::apply(VRTransformPtr obj) {
    if (!doTConstraint && !doRConstraint) return;
    auto now = VRGlobals::get()->CURRENT_FRAME;
    if (apply_time_stamp == now) return;
    apply_time_stamp = now;

    VRTransformPtr ref = constraints_referential.lock();

    Matrix t, tr, ti, ti1, ti2;
    Matrix t0 = constraints_reference;
    if (localTC) t = obj->getMatrix();
    else t = obj->getWorldMatrix();
    if (ref) {
        tr = ref->getWorldMatrix();
        ti = tr; ti.invert();
        ti1 = ti; ti2 = ti;
        //ti1.mult(t0); t0 = ti1; // W = O*L => L = O⁻*W
        ti2.mult(t); t = ti2; // W = O*L => L = O⁻*W
    }

    //rotation
    if (doRConstraint) {
        if (rConMode == POINT) { for (int i=0;i<3;i++) t[i] = t0[i]; } // TODO: add RConstraint as offset to referential and reference

        if (rConMode == LINE) {
            Vec3f a = rConstraint;
            Vec3f y = Vec3f(0,1,0);

            Quaternion q(Vec3f(-a[2], 0, a[0]), -acos(a[1]));
            Matrix m; m.setRotate(q);
            Matrix mi;
            m.inverse(mi);

            auto rebase = [&](Matrix a) {
                Matrix k = m;
                k.mult(a);
                return k;
            };

            auto restore = [&](Matrix a) {
                Matrix k = mi;
                k.mult(a);
                return k;
            };

            t0 = rebase(t0);
            t = rebase(t);

            int u,v,w;
            u = 1; v = 0; w = 2; // rotate around up axis

            for (int i=0;i<3;i++) t[i][u] = t0[i][u]; //copy old transformation

            //normiere so das die b komponennte konstant bleibt
            for (int i=0;i<3;i++) {
                float a = 1-t[i][u]*t[i][u];
                if (a < 1e-6) {
                    t[i][v] = t0[i][v];
                    t[i][w] = t0[i][w];
                } else {
                    a /= (t0[i][v]*t0[i][v] + t0[i][w]*t0[i][w]);
                    a = sqrt(a);
                    t[i][v] *= a;
                    t[i][w] *= a;
                }
            }

            t0 = restore(t0);
            t = restore(t);
        }

        if (rConMode == PLANE); // TODO
    }

    //translation
    if (doTConstraint) {
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

    if (ref) { tr.mult(t); t = tr; }
    if (localTC) obj->setMatrix(t);
    else obj->setWorldMatrix(t);
}


void VRConstraint::setReference(Matrix m) { constraints_reference = m; }
void VRConstraint::setReferential(VRTransformPtr t) { constraints_referential = t; }
void VRConstraint::toggleTConstraint(bool b, VRTransformPtr obj) { doTConstraint = b; if (b) obj->getWorldMatrix(constraints_reference); if(!doRConstraint) obj->setFixed(!b); }
void VRConstraint::toggleRConstraint(bool b, VRTransformPtr obj) { doRConstraint = b; if (b) obj->getWorldMatrix(constraints_reference); if(!doTConstraint) obj->setFixed(!b); }

void VRConstraint::setTConstraint(Vec3f trans, int mode, bool local) {
    tConstraint = trans;
    if (tConstraint.length() > 1e-4) tConstraint.normalize();
    tConMode = mode;
    localTC = local;
}

void VRConstraint::setRConstraint(Vec3f rot, int mode, bool local) {
    rConstraint = rot;
    if (rConstraint.length() > 1e-4) rConstraint.normalize();
    rConMode = mode;
    localRC = local;
}

bool VRConstraint::getTMode() { return tConMode; }
bool VRConstraint::getRMode() { return rConMode; }
Vec3f VRConstraint::getTConstraint() { return tConstraint; }
Vec3f VRConstraint::getRConstraint() { return rConstraint; }

bool VRConstraint::hasTConstraint() { return doTConstraint; }
bool VRConstraint::hasRConstraint() { return doRConstraint; }

OSG_END_NAMESPACE;
