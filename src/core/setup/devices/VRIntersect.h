#ifndef VRINTERSECT_H_INCLUDED
#define VRINTERSECT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include <vector>
#include <map>

#include "core/objects/VRObjectFwd.h"

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
    VRObjectPtr object = 0; // hit object
    VRObjectPtr tree = 0; // intersection tree
    Pnt3f point;
    Vec3f normal;
    Vec2f texel;
    int triangle;
    Vec3i triangleVertices;
};

class VRIntersect {
    private:
        map<VRObjectPtr, VRIntersection> intersections;
        VRIntersection lastIntersection;

        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies

        VRSignal* dragSignal = 0;
        VRSignal* dropSignal = 0;
        VRTransformPtr dragged = 0;
        VRTransformPtr dragged_ghost = 0;
        uint drop_time = 0;
        VRGeometryPtr cross = 0;
        vector<VRObjectPtr> dynTrees;
        VRObjectPtr dynTree = 0;

        map<VRObjectPtr, VRDevCb* > int_fkt_map;
        map<VRObjectPtr, VRDevCb* > dra_fkt_map;
        VRDevCb* drop_fkt;

        void dragCB(VRTransformPtr caster, VRObjectPtr tree, VRDevice* dev = 0);

        void initCross();

    public:
        VRIntersect();
        ~VRIntersect();

        VRIntersection intersect(VRObjectPtr tree, Line ray);
        VRIntersection intersect(VRObjectPtr tree);
        void drag(VRObjectPtr obj, VRTransformPtr caster);
        void drop(VRDevice* dev = 0);
        VRDevCb* addDrag(VRTransformPtr caster, VRObjectPtr tree);
        VRDevCb* getDrop();

        void toggleDragnDrop(bool b);
        void showHitPoint(bool b);

        VRObjectPtr getCross();

        void addDynTree(VRObjectPtr o);
        void remDynTree(VRObjectPtr o);
        void updateDynTree(VRObjectPtr a);

        VRSignal* getDragSignal();
        VRSignal* getDropSignal();
        VRTransformPtr getDraggedObject();
        VRTransformPtr getDraggedGhost();
        VRIntersection getLastIntersection();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
