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
    string name;
    VRObjectWeakPtr object; // hit object
    VRObjectWeakPtr tree; // intersection tree
    Pnt3f point;
    Vec3f normal;
    Vec2f texel;
    int triangle;
    Vec3i triangleVertices;
};

class VRIntersect {
    private:
        map<VRObject*, VRIntersection> intersections;
        VRIntersection lastIntersection;

        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies

        VRSignal* dragSignal = 0;
        VRSignal* dropSignal = 0;
        VRTransformWeakPtr dragged;
        VRTransformPtr dragged_ghost;
        uint drop_time = 0;
        VRGeometryPtr cross = 0;
        map<VRObject*, VRObjectWeakPtr> dynTrees;
        VRObjectWeakPtr dynTree;

        map<VRObject*, VRDevCb* > int_fkt_map;
        map<VRObject*, VRDevCb* > dra_fkt_map;
        VRDevCb* drop_fkt;

        void dragCB(VRTransformWeakPtr caster, VRObjectWeakPtr tree, VRDevice* dev = 0);

        void initCross();

    public:
        VRIntersect();
        ~VRIntersect();

        VRIntersection intersect(VRObjectWeakPtr tree, Line ray);
        VRIntersection intersect(VRObjectWeakPtr tree);
        VRIntersection intersect();
        void drag(VRObjectWeakPtr obj, VRTransformWeakPtr caster);
        void drop(VRDevice* dev = 0);
        VRDevCb* addDrag(VRTransformWeakPtr caster, VRObjectWeakPtr tree);
        VRDevCb* addDrag(VRTransformWeakPtr caster);
        VRDevCb* getDrop();

        void toggleDragnDrop(bool b);
        void showHitPoint(bool b);

        VRObjectPtr getCross();

        void addDynTree(VRObjectWeakPtr o);
        void remDynTree(VRObjectWeakPtr o);
        void updateDynTree(VRObjectPtr a);

        VRSignal* getDragSignal();
        VRSignal* getDropSignal();
        VRTransformPtr getDraggedObject();
        VRTransformPtr getDraggedGhost();
        VRIntersection getLastIntersection();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
