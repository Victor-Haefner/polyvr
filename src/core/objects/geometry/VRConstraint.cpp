#include "VRConstraint.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRConstraint::VRConstraint() {
    local = false;
    for (int i=0; i<6; i++) { min[i] = 0.0; max[i] = 0.0; }
}

VRConstraint::~VRConstraint() {;}

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

void VRConstraint::updateMatrix(Matrix& m) {;} // TODO
void VRConstraint::updatePose(Vec3f& from, Vec3f& at, Vec3f& up) {;} // TODO

OSG_END_NAMESPACE;
