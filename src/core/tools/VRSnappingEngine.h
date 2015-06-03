#ifndef VRSNAPPINGENGINE_H_INCLUDED
#define VRSNAPPINGENGINE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>
#include <map>


using namespace std;
OSG_BEGIN_NAMESPACE;

class Octree;
class VRObject;
class VRTransform;
class VRGeometry;
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
            VRTransform* o1 = 0;
            VRTransform* o2 = 0;
            Matrix m;
            VRDevice* dev = 0;
            void set(VRTransform* O1, VRTransform* O2, Matrix M, VRDevice* DEV) {
                o1 = O1; o2 = O2; m = M; dev = DEV;
            }
        };

    private:
        map<int, Rule*> rules; // snapping rules, translation and orientation
        map<VRTransform*, Matrix> objects; // map objects to reference matrix
        map<VRTransform*, vector<VRTransform*> > anchors; // object anchors
        Octree* positions = 0; // objects by positions
        VRGeometry* hintGeo = 0;

        float influence_radius = 1000;
        float distance_snap = 0.05;
        bool showHints = false;

        EventSnap* event = 0;
        VRSignal* snapSignal = 0;

    public:
        VRSnappingEngine();

        VRSignal* getSignalSnap();

        void clear();

        Type typeFromStr(string t);

        int addRule(Type t, Type o, Line pt, Line po, float d, float w = 1, VRTransform* l = 0);
        void remRule(int i);

        void addObjectAnchor(VRTransform* obj, VRTransform* a);
        void clearObjectAnchors(VRTransform* obj);
        void remLocalRules(VRTransform* obj);

        void addObject(VRTransform* obj, float weight = 1);
        void addTree(VRObject* obj, float weight = 1);
        void remObject(VRTransform* obj);

        void setVisualHints(bool b = true);
        void setPreset(PRESET preset);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSNAPPINGENGINE_H_INCLUDED
