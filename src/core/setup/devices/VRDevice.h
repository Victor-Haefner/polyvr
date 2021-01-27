#ifndef VRDEVICE_H_INCLUDED
#define VRDEVICE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRSetupFwd.h"
#include "VRIntersect.h"
#include "VRAvatar.h"
#include "core/utils/VRName.h"
#include "core/utils/VRStorage.h"
OSG_BEGIN_NAMESPACE;

class VRDevice : public std::enable_shared_from_this<VRDevice>, public VRName, public VRIntersect, public VRAvatar {
    protected:
        int sig_key = -1;
        int sig_state = -1;
        string message;
        string type;
        VRTransformWeakPtr target;
        Vec2d speed;

        map< string, VRSignalPtr > callbacks; //all callbacks
        map<VRSignal*, VRSignalPtr> activatedSignals;
        map<VRSignal*, VRDeviceCbPtr> deactivationCallbacks;
        map<int, int> BStates;//states of buttons
        map<int, float> SStates;//states of slider

        VRSignalPtr signalExist(int key, int state);
        VRSignalPtr createSignal(int key, int state);
        void triggerSignal(int key, int state);

    public:
        VRDevice(string type);
        virtual ~VRDevice();

        static VRDevicePtr create(string type);
        VRDevicePtr ptr();

        virtual void setCamera(VRCameraPtr cam);

        VRSignalPtr newSignal(int key, int state);
        VRSignalPtr addToggleSignal(int key);
        void addSignal(VRSignalPtr sig, int key, int state);
        VRSignalPtr addSlider(int key);

        map<string, VRSignalPtr> getSignals();
        VRSignalPtr getSignal(string name);

        virtual void clearSignals();

        virtual VRSignalPtr getToEdgeSignal();
        virtual VRSignalPtr getFromEdgeSignal();

        void change_button(int key, int state);
        void change_slider(int key, float state);

        void addUpdateSignal(VRSignalPtr sig);
        void remUpdateSignal(VRSignalPtr sig, VRDeviceWeakPtr dev);
        void updateSignals();

        int key();
        int getState();
        string getMessage();
        void setMessage(string s);

        string getType();

        void b_state(int key, int* b_state);
        int b_state(int key);

        void s_state(int key, float* s_state);
        float s_state(int key);

        void setTarget(VRTransformPtr e);
        VRTransformPtr getTarget();

        void printMap();

        void setSpeed(Vec2d s);
        Vec2d getSpeed();

        void drag(VRObjectPtr obj, int bID = 0);
        void drop(int bID = 0);

        VRTransformPtr getBeacon(int i = 0);
        void setBeacon(VRTransformPtr, int i = 0);
        void setDnD(bool b);

        bool intersect2(VRObjectPtr subtreeRoot = 0, bool force = 0, VRTransformPtr caster = 0, Vec3d dir = Vec3d(0,0,-1), bool skipVols = false);
        Pnt3d getIntersectionPoint();
        Vec3i getIntersectionTriangle();
        Vec3d getIntersectionNormal();
        Vec2d getIntersectionUV();
        Line  getIntersectionRay();
        VRObjectPtr getIntersected();
        int getIntersectionID();

        void addIntersection(VRObjectPtr obj, int priority = 0);
        void remIntersection(VRObjectPtr obj);
        VRTransformPtr getDragged();
        VRTransformPtr getDragGhost();
};


OSG_END_NAMESPACE;

#endif // VRDEVICE_H_INCLUDED
