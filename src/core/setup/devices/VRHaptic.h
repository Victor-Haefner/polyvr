#ifndef VRHAPTIC_H_INCLUDED
#define VRHAPTIC_H_INCLUDED

#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class virtuose;
class VRTransform;

class VRHaptic : public VRDevice {
    private:
        virtuose* v;
        string IP;
        string type;
        VRFunction<int>* updateFkt;

        void applyTransformation(VRTransform* t);

    public:
        VRHaptic();
        ~VRHaptic();

        void setForce(Vec3f force, Vec3f torque);
        void setSimulationScales(float scale, float forces);

        void setIP(string IP);
        string getIP();

        void setType(string IP);
        string getType();
};

OSG_END_NAMESPACE;

#endif // VRHAPTIC_H_INCLUDED
