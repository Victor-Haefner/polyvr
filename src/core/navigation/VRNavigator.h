#ifndef VRNAVIGATOR_H_INCLUDED
#define VRNAVIGATOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <string>
#include <vector>

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
    VRSignal* sig;
    VRDevCb* cb;
    string sig_name;
    string cb_name;
    bool doRepeat;

    VRNavBinding(VRDevCb* c, int k, int s, bool repeat);
};

class VRNavPreset {
    private:
        vector<VRNavBinding> bindings;
        VRDevice* dev;
        VRTransform* target;
        bool active;

    public:
        VRNavPreset();
        ~VRNavPreset();

        void updateBinding(VRNavBinding& b);

        void setDevice(VRDevice* _dev);

        void setTarget(VRTransform* _target);

        void activate();

        void deactivate();

        vector<VRNavBinding>& getBindings();

        void addKeyBinding(VRNavBinding b);
};

class VRNavigator_base {
    private:
        map<string, VRDevCb*> library;

        VRNavPreset* current;
        map<string, VRNavPreset*> presets;

    public:
        VRNavigator_base ();

        void addPreset(VRNavPreset* ps, string& name);

        void remPreset(string name);

        void setActivePreset(string s);

        VRNavPreset* getPreset(string s);

        map<string, VRNavPreset*> getPresets();

        void storeCallback(VRDevCb* cb);

        map<string, VRDevCb*>& getCallbacks();
};

class VRNavigator : public VRNavigator_base {
    private:
        VRTransform* target;
        VRDevice* device;

        vector<VRTransform* > walk_surfaces;
        static float clip_dist_down;

        // callbacks
        void zoom(VRDevice* dev, int dir);
        void walk(VRDevice* dev);
        void fly_walk(VRDevice* dev);
        void orbit(VRDevice* dev);
        void orbit2D(VRDevice* dev);
        void focus(VRDevice* dev);

    protected:
        VRNavigator();

        // init presets
        void initWalk(VRTransform* target, VRDevice* dev);
        void initOrbit(VRTransform* target, VRDevice* dev);
        void initOrbit2D(VRTransform* target, VRDevice* dev);
        void initFlyOrbit(VRTransform* target, VRDevice* dev);
        void initFlyWalk(VRTransform* target, VRDevice* dev);
};

OSG_END_NAMESPACE;

#endif // VRNAVIGATOR_H_INCLUDED
