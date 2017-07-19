#ifndef VRINTERSECT_H_INCLUDED
#define VRINTERSECT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include <vector>
#include <map>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

struct VRIntersection {
    bool hit = false;
    uint time = 0;
    string name;
    VRObjectWeakPtr object; // hit object
    VRObjectWeakPtr tree; // intersection tree
    Pnt3d point;
    Vec3d normal;
    Vec2d texel;
    int triangle;
    Vec3i triangleVertices;
};

class VRIntersect {
    private:
        map<VRObject*, VRIntersection> intersections;
        VRIntersection lastIntersection;

        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies

        VRSignalPtr dragSignal = 0;
        VRSignalPtr dropSignal = 0;
        VRTransformWeakPtr dragged;
        VRTransformPtr dragged_ghost;
        uint drop_time = 0;
        VRGeometryPtr cross = 0;
        map< int, map<VRObject*, VRObjectWeakPtr> > dynTrees;
        map<VRObject*, VRDeviceCbPtr > int_fkt_map;
        map<VRObject*, VRDeviceCbPtr > dra_fkt_map;
        VRDeviceCbPtr drop_fkt;

        void dragCB(VRTransformWeakPtr caster, VRObjectWeakPtr tree, VRDeviceWeakPtr dev = VRDevicePtr(0));

        void initCross();

    protected:
        void initIntersect(VRDevicePtr dev);

    public:
        VRIntersect();
        ~VRIntersect();

        VRIntersection intersect(VRObjectWeakPtr tree, Line ray);
        VRIntersection intersect(VRObjectWeakPtr tree, bool force = false);
        VRIntersection intersect();
        void drag(VRObjectWeakPtr obj, VRTransformWeakPtr caster);
        void drop(VRDeviceWeakPtr dev = VRDevicePtr(0));
        VRDeviceCbPtr addDrag(VRTransformWeakPtr caster, VRObjectWeakPtr tree);
        VRDeviceCbPtr addDrag(VRTransformWeakPtr caster);
        VRDeviceCbPtr getDrop();

        void toggleDragnDrop(bool b);
        void showHitPoint(bool b);

        VRObjectPtr getCross();

        void addDynTree(VRObjectPtr o, int prio = 0);
        void remDynTree(VRObjectPtr o);
        void clearDynTrees();

        VRSignalPtr getDragSignal();
        VRSignalPtr getDropSignal();
        VRTransformPtr getDraggedObject();
        VRTransformPtr getDraggedGhost();
        VRIntersection getLastIntersection();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
