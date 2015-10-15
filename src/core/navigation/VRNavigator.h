#ifndef VRNAVIGATOR_H_INCLUDED
#define VRNAVIGATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <map>
#include <string>
#include <vector>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRStorage.h"
#include "core/utils/VRName.h"

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRSignal;
class VRTransform;
typedef VRFunction<VRDevice*> VRDevCb;

struct VRNavBinding {
    int key;
    int state;
    VRSignal* sig = 0;
    VRDevCb* cb = 0;
    string sig_name;
    string cb_name;
    bool doRepeat = false;

    VRNavBinding(VRDevCb* c, int k, int s, bool repeat);
};

class VRNavPreset : public VRName {
    private:
        vector<VRNavBinding> bindings;
        VRDevice* dev;
        VRTransformPtr target;
        bool active;
        Vec2f speed;

    public:
        VRNavPreset();
        ~VRNavPreset();

        void updateBinding(VRNavBinding& b);

        void setDevice(VRDevice* _dev);
        void setTarget(VRTransformPtr _target);
        void setSpeed(float vt, float vr);

        void activate();
        void deactivate();

        void addKeyBinding(VRNavBinding b);
        vector<VRNavBinding>& getBindings();
};

class VRNavigator_base : public VRStorage {
    private:
        map<string, VRDevCb*> library;

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

        void storeNavigationCallback(VRDevCb* cb);
        map<string, VRDevCb*>& getNavigationCallbacks();
        VRDevCb* getNavigationCallback(string s);
};

class VRNavigator : public VRNavigator_base {
    private:
        VRTransformPtr target;
        VRDevice* device;

        vector<VRTransformPtr > walk_surfaces;
        static float clip_dist_down;

        // callbacks
        void zoom(VRDevice* dev, int dir);
        void walk(VRDevice* dev);
        void fly_walk(VRDevice* dev);
        void hyd_walk(VRDevice* dev);
        void orbit(VRDevice* dev);
        void orbit2D(VRDevice* dev);
        void focus(VRDevice* dev);

    protected:
        VRNavigator();
        ~VRNavigator();

        // init presets
        void initWalk(VRTransformPtr target, VRDevice* dev);
        void initOrbit(VRTransformPtr target, VRDevice* dev);
        void initOrbit2D(VRTransformPtr target, VRDevice* dev);
        void initFlyOrbit(VRTransformPtr target, VRDevice* dev);
        void initFlyWalk(VRTransformPtr target, VRDevice* dev);
        void initHydraFly(VRTransformPtr target, VRDevice* dev);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRNAVIGATOR_H_INCLUDED
