#ifndef VRDEVICE_H_INCLUDED
#define VRDEVICE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRIntersect.h"
#include "VRAvatar.h"
#include "core/utils/VRName.h"
#include "core/utils/VRStorage.h"
OSG_BEGIN_NAMESPACE;

class VRSignal;

class VRDevice : public VRIntersect, public VRAvatar, public VRName, public VRStorage {
    protected:
        int sig_key = -1;
        int sig_state = -1;
        string message;
        string type;
        VRTransformWeakPtr target;
        Vec2f speed;

        map< string, VRSignalPtr > callbacks; //all callbacks
        map<VRSignal*, VRSignalPtr> activatedSignals;
        map<VRSignal*, VRDeviceCb> deactivationCallbacks;
        map<int, int> BStates;//states of buttons
        map<int, float> SStates;//states of slider

        VRSignalPtr signalExist(int key, int state);
        VRSignalPtr createSignal(int key, int state);
        void triggerSignal(int key, int state);

    public:
        VRDevice(string _type);
        virtual ~VRDevice();

        VRSignalPtr addSignal(int key, int state);
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
        void remUpdateSignal(VRSignalPtr sig, VRDevice* dev);
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

        void setSpeed(Vec2f s);
        Vec2f getSpeed();

        //virtual void save(xmlpp::Element* node);
        //virtual void load(xmlpp::Element* node);
};


OSG_END_NAMESPACE;

#endif // VRDEVICE_H_INCLUDED
