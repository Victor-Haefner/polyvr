#ifndef VRHAPTIC_H_INCLUDED
#define VRHAPTIC_H_INCLUDED

#include "VRDevice.h"
#include "core/objects/geometry/VRPhysics.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class virtuose;
class VRTransform;

class VRHaptic : public VRDevice {
    private:
        virtuose* v;
        string IP;
        string type;
        VRFunction<int>* timestepWatchdog;
        VRFunction<int>* updateFktPre;
        VRFunction<int>* updateFktPost;
        Vec3i button_states;

        /**gets positive when fps changes, negative w**/
        int fps_change = 0;
        /** fps stable flag. 1=stable, 0=not stable**/
        int fps_stable = 1;


        void on_scene_changed(VRDevice* dev);

        void applyTransformation(VRTransform* t);
        void updateHapticPre(VRTransform* t);
        void updateHapticPost(VRTransform* t);
        /** restarts Haptic, if necessary (fps drop/gain) **/
        void updateHapticTimestep(VRTransform* t);

    public:
        VRHaptic();
        ~VRHaptic();

        void setForce(Vec3f force, Vec3f torque);
        Vec3f getForce();
        void setSimulationScales(float scale, float forces);
        void attachTransform(VRTransform* trans);
        void setBase(VRTransform* trans);
        void detachTransform();
        void updateVirtMechPre();
        void updateVirtMechPost();
        Vec3i getButtonStates();
        void setIP(string IP);
        string getIP();

        void setType(string IP);
        string getType();
};

OSG_END_NAMESPACE;

#endif // VRHAPTIC_H_INCLUDED
