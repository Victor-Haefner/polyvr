#ifndef VRSNAPPINGENGINE_H_INCLUDED
#define VRSNAPPINGENGINE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>
#include <map>
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/objects/VRObjectFwd.h"


using namespace std;
OSG_BEGIN_NAMESPACE;

class Octree;
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

        struct EventSnap {
            int snap = 0;
            VRTransformPtr o1 = 0;
            VRTransformPtr o2 = 0;
            Matrix m;
            VRDevice* dev = 0;
            void set(VRTransformPtr O1, VRTransformPtr O2, Matrix M, VRDevice* DEV, int Snap) {
                o1 = O1; o2 = O2; m = M; dev = DEV; snap = Snap;
            }
        };

    private:
        map<int, Rule*> rules; // snapping rules, translation and orientation
        map<VRTransformPtr, Matrix> objects; // map objects to reference matrix
        map<VRTransformPtr, vector<VRTransformPtr> > anchors; // object anchors
        Octree* positions = 0; // objects by positions
        VRGeometryPtr hintGeo = 0;
        VRUpdatePtr updatePtr;

        float influence_radius = 1000;
        float distance_snap = 0.05;
        bool showHints = false;

        EventSnap* event = 0;
        VRSignalPtr snapSignal = 0;

    public:
        VRSnappingEngine();

        VRSignalPtr getSignalSnap();

        void clear();

        Type typeFromStr(string t);

        int addRule(Type t, Type o, Line pt, Line po, float d, float w = 1, VRTransformPtr l = 0);
        void remRule(int i);

        void addObjectAnchor(VRTransformPtr obj, VRTransformPtr a);
        void clearObjectAnchors(VRTransformPtr obj);
        void remLocalRules(VRTransformPtr obj);

        void addObject(VRTransformPtr obj, float weight = 1);
        void addTree(VRObjectPtr obj, float weight = 1);
        void remObject(VRTransformPtr obj);

        void setVisualHints(bool b = true);
        void setPreset(PRESET preset);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSNAPPINGENGINE_H_INCLUDED
