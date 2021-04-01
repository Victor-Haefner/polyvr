#ifndef VRSNAPPINGENGINE_H_INCLUDED
#define VRSNAPPINGENGINE_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>
#include <map>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"


using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSignal;
class VRDevice;

class VRSnappingEngine {
    private:
        struct Rule;

    public:
        enum PRESET {
            SIMPLE_ALIGNMENT,
            SNAP_BACK,
        };

        enum Type {
            NONE,
            POINT,
            LINE,
            PLANE,
            POINT_LOCAL,
            LINE_LOCAL,
            PLANE_LOCAL
        };

        struct Anchor {
            VRTransformPtr a;
            int grp = 0;
            int snpgrp = 0;
        };

        struct EventSnap {
            int snap = 0;
            int snapID = 0;
            VRTransformPtr o1 = 0;
            VRTransformPtr o2 = 0;
            Matrix4d m;
            VRDevicePtr dev = 0;
            void set(VRTransformPtr O1, VRTransformPtr O2, Matrix4d M, VRDevicePtr DEV, int Snap, int SnapID) {
                o1 = O1; o2 = O2; m = M; dev = DEV; snap = Snap; snapID = SnapID;
            }
        };

        typedef VRFunction<EventSnap> VRSnapCb;
        typedef shared_ptr<VRSnapCb> VRSnapCbPtr;

    private:
        map<int, Rule*> rules; // snapping rules, translation and orientation
        map<VRTransformPtr, Matrix4d> objects; // map objects to reference matrix
        map<VRTransformPtr, vector<Anchor> > anchors; // object anchors
        vector<VRSnapCbPtr> callbacks; // object anchors
        OctreePtr positions = 0; // objects by positions
        VRUpdateCbPtr updatePtr;

        VRTransformPtr ghostHost;
        VRTransformPtr ghostHook;
        VRMaterialPtr ghostMat;
        VRObjectPtr ghostParent;
        VRDevicePtr ghostDevice;

        float influence_radius = 1000;
        float distance_snap = 0.05;
        bool lastEvent = 0;
        int lastEventID = 0;

        EventSnap* event = 0;
        VRSignalPtr snapSignal = 0;

        bool doGhosts = false;
        bool active = true;

        // update sub functions
        void terminateGhost();
        void updateGhost(VRDevicePtr dev, VRTransformPtr obj);
        void handleDraggedObject(VRDevicePtr dev, VRTransformPtr obj, VRTransformPtr gobj);
        void postProcessEvent(VRDevicePtr dev, VRTransformPtr obj, VRTransformPtr gobj);

    public:
        VRSnappingEngine();
        ~VRSnappingEngine();
        static shared_ptr<VRSnappingEngine> create();

        VRSignalPtr getSignalSnap();

        void clear();

        Type typeFromStr(string t);

        void setActive(bool b);
        bool isActive();

        void enableGhosts(bool b);

        void addCallback(VRSnapCbPtr cb);

        int addRule(Type t, Type o, PosePtr pt, PosePtr po, float d, int g = 0, VRTransformPtr l = 0);
        void remRule(int i);

        void addObjectAnchor(VRTransformPtr obj, VRTransformPtr a, int grp = 0, int snpgrp = 0);
        void clearObjectAnchors(VRTransformPtr obj);
        void remLocalRules(VRTransformPtr obj);

        void addObject(VRTransformPtr obj, int group = 0);
        void addTree(VRObjectPtr obj, int group = 0);
        void remObject(VRTransformPtr obj);

        void setPreset(PRESET preset);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSNAPPINGENGINE_H_INCLUDED
