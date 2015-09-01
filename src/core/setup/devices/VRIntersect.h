#ifndef VRINTERSECT_H_INCLUDED
#define VRINTERSECT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include <vector>
#include <map>

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRGeometry;
class VRObject;
class VRTransform;
class VRSignal;
typedef VRFunction<VRDevice*> VRDevCb;

struct VRIntersection {
    bool hit = false;
    uint time = 0;
    VRObject* object = 0; // hit object
    VRObject* tree = 0; // intersection tree
    Pnt3f point;
    Vec3f normal;
    Vec2f texel;
    int triangle;
};

class VRIntersect {
    private:
        map<VRObject*, VRIntersection> intersections;
        VRIntersection lastIntersection;

        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies

        VRSignal* dragSignal = 0;
        VRSignal* dropSignal = 0;
        VRTransform* dragged = 0;
        VRTransform* dragged_ghost = 0;
        uint drop_time = 0;
        VRGeometry* cross = 0;
        vector<VRObject*> dynTrees;
        VRObject* dynTree = 0;

        map<VRObject*, VRDevCb* > int_fkt_map;
        map<VRObject*, VRDevCb* > dra_fkt_map;
        VRDevCb* drop_fkt;

        void dragCB(VRTransform* caster, VRObject* tree, VRDevice* dev = 0);

        void initCross();

    public:
        VRIntersect();
        ~VRIntersect();

        VRIntersection intersect(VRObject* tree, Line ray);
        VRIntersection intersect(VRObject* tree);
        void drag(VRObject* obj, VRTransform* caster);
        void drop(VRDevice* dev = 0);
        VRDevCb* addDrag(VRTransform* caster, VRObject* tree);
        VRDevCb* getDrop();

        void toggleDragnDrop(bool b);
        void showHitPoint(bool b);

        VRObject* getCross();

        void addDynTree(VRObject* o);
        void remDynTree(VRObject* o);
        void updateDynTree(VRObject* a);

        VRSignal* getDragSignal();
        VRSignal* getDropSignal();
        VRTransform* getDraggedObject();
        VRTransform* getDraggedGhost();
        VRIntersection getLastIntersection();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
