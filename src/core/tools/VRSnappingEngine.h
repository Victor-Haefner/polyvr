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
#include "core/objects/material/VRMaterialFwd.h"
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
            bool active = true;
        };

        struct EventSnap : public std::enable_shared_from_this<EventSnap> {
            int snap = 0;
            int snapID = 0;
            VRTransformPtr o1 = 0;
            VRTransformPtr o2 = 0;
            VRTransformPtr a1 = 0;
            VRTransformPtr a2 = 0;

            Vec3d po1;
            Vec3d po2;
            Vec3d pa1;
            Vec3d pa2;

            Matrix4d m;
            VRDevicePtr dev = 0;

            void set(VRTransformPtr O1, VRTransformPtr O2, Matrix4d M, VRDevicePtr DEV, int Snap, int SnapID, VRTransformPtr A1, VRTransformPtr A2) {
                o1 = O1; o2 = O2; a1 = A1; a2 = A2; m = M; dev = DEV; snap = Snap; snapID = SnapID;
            }

            shared_ptr<EventSnap> ptr() { return shared_from_this(); }
        };

        typedef shared_ptr<EventSnap> EventSnapPtr;
        typedef weak_ptr<EventSnap> EventSnapWeakPtr;
        typedef VRFunction<EventSnapWeakPtr, bool> VRSnapCb;
        typedef shared_ptr<VRSnapCb> VRSnapCbPtr;

    private:
        map<int, Rule*> rules; // snapping rules, translation and orientation
        map<VRTransformPtr, Matrix4d> objects; // map objects to reference matrix
        map<VRTransformPtr, vector<Anchor> > anchors; // object anchors
        vector<VRSnapCbPtr> callbacks; // object anchors
        shared_ptr<Octree<VRTransform*>> positions = 0; // objects by positions
        VRUpdateCbPtr updatePtr;

        VRTransformPtr ghostHost;
        VRTransformPtr ghostHook;
        VRMaterialPtr ghostMat;
        VRObjectPtr ghostParent;
        VRDevicePtr ghostDevice;
        VRGeometryPtr snapVisual;

        float influence_radius = 1000;
        float distance_snap = 0.05;
        bool lastEvent = 0;
        int lastEventID = 0;

        EventSnapPtr event;
        VRSignalPtr snapSignal;

        bool doGhosts = false;
        bool active = true;
        bool showSnaps = false;

        // update sub functions
        void terminateGhost();
        void updateGhost(VRDevicePtr dev, VRTransformPtr obj);
        void updateSnapVisual();
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
        void pauseObjectAnchors(VRTransformPtr obj, bool b);
        void pauseObjectAnchor(VRTransformPtr obj, int i, bool b);
        void remLocalRules(VRTransformPtr obj);

        void addObject(VRTransformPtr obj, int group = 0);
        void addTree(VRObjectPtr obj, int group = 0);
        void remObject(VRTransformPtr obj);

        void setPreset(PRESET preset);
        void showSnapping(bool b);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSNAPPINGENGINE_H_INCLUDED
