#ifndef VRNAVIGATOR_H_INCLUDED
#define VRNAVIGATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>
#include <string>
#include <vector>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRStorage.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRNavBinding {
    int key = 0;
    int state = 0;
    VRSignalWeakPtr sig;
    VRDeviceCb cb;
    string sig_name;
    string cb_name;
    bool doRepeat = false;

    VRNavBinding(VRDeviceCb c, int k, int s, bool repeat);
    ~VRNavBinding();

    void clearSignal();
};

class VRNavPreset : public VRName {
    private:
        vector<VRNavBinding> bindings;
        VRDevicePtr dev = 0;
        VRTransformWeakPtr target;
        bool active = false;
        Vec2f speed;

    public:
        VRNavPreset();
        ~VRNavPreset();

        void updateBinding(VRNavBinding& b);

        void setDevice(VRDevicePtr _dev);
        void setTarget(VRTransformPtr _target);
        void setSpeed(float vt, float vr);

        void activate();
        void deactivate();

        void addKeyBinding(VRNavBinding b);
        vector<VRNavBinding>& getBindings();

        vector<VRNavBinding>::iterator begin();
        vector<VRNavBinding>::iterator end();
};

class VRNavigator_base : public VRStorage {
    private:
        map<string, VRDeviceCb> library;

        VRNavPreset* current;
        string current_name;
        map<string, VRNavPreset*> presets;

    public:
        VRNavigator_base();
        ~VRNavigator_base();

        void addNavigation(VRNavPreset* ps);
        void remNavigation(string name);

        void setActiveNavigation(string s);
        string getActiveNavigation();
        VRNavPreset* getNavigation(string s);
        map<string, VRNavPreset*> getNavigations();
        vector<string> getNavigationNames();
        string getNavigationTip(string name);

        void storeNavigationCallback(VRDeviceCb cb);
        map<string, VRDeviceCb>& getNavigationCallbacks();
        VRDeviceCb getNavigationCallback(string s);
};

class VRNavigator : public VRNavigator_base {
    private:
        VRTransformWeakPtr target;
        VRDeviceWeakPtr device;

        vector<VRTransformWeakPtr> walk_surfaces;
        static float clip_dist_down;
        std::shared_ptr<VRFunction<float> > focus_fkt;

        // callbacks
        void zoom(VRDeviceWeakPtr dev, int dir);
        void walk(VRDeviceWeakPtr dev);
        void fly_walk(VRDeviceWeakPtr dev);
        void hyd_walk(VRDeviceWeakPtr dev);
        void orbit(VRDeviceWeakPtr dev);
        void orbit2D(VRDeviceWeakPtr dev);
        void focus(VRDeviceWeakPtr dev);

    protected:
        VRNavigator();
        ~VRNavigator();

        // init presets
        void initWalk(VRTransformPtr target, VRDevicePtr dev);
        void initOrbit(VRTransformPtr target, VRDevicePtr dev);
        void initOrbit2D(VRTransformPtr target, VRDevicePtr dev);
        void initFlyOrbit(VRTransformPtr target, VRDevicePtr dev);
        void initFlyWalk(VRTransformPtr target, VRDevicePtr dev);
        void initHydraFly(VRTransformPtr target, VRDevicePtr dev);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRNAVIGATOR_H_INCLUDED
