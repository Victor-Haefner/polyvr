#ifndef VRINTERSECT_H_INCLUDED
#define VRINTERSECT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGIntersectAction.h>
#include <vector>
#include <map>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRIntersectAction : public IntersectAction {
    private:
        bool doSkipVolumes = false;

    public:
        VRIntersectAction() {}

        void setSkipVolumes(bool b) { doSkipVolumes = b; }
        bool skipVolume() { return doSkipVolumes; }
};

struct VRIntersection {
    Line ray;
    bool hit = false;
    unsigned int time = 0;
    string name;
    VRObjectWeakPtr object; // hit object
    VRObjectWeakPtr tree; // intersection tree
    Pnt3d point;
    Vec3d normal;
    Vec2d texel;
    int triangle;
    int customID;
    Vec3i triangleVertices;

    VRObjectPtr getIntersected();
    Pnt3d getIntersection();
};

class VRIntersect {
    private:
        map<VRObject*, VRIntersection> intersections;
        VRIntersection lastIntersection;

        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies

        VRSignalPtr dragSignal = 0;
        VRSignalPtr dropSignal = 0;
        map<VRTransform*, VRTransformWeakPtr> dragged;
        VRTransformPtr dragged_ghost;
        unsigned int drop_time = 0;
        VRGeometryPtr cross = 0;
        map<VRObject*, VRDeviceCbPtr > int_fkt_map;
        map<VRObject*, VRDeviceCbPtr > dra_fkt_map;
        VRDeviceCbPtr drop_fkt;

        void initCross();

    protected:
        map< int, map<VRObject*, VRObjectWeakPtr> > dynTrees;

        void initIntersect(VRDevicePtr dev);
        VRIntersection intersectRay(VRObjectWeakPtr tree, Line ray, bool skipVols = false);

        virtual void dragCB(VRTransformWeakPtr caster, VRObjectWeakPtr tree, VRDeviceWeakPtr dev = VRDevicePtr(0));

    public:
        VRIntersect();
        ~VRIntersect();

        VRIntersection intersect(VRObjectWeakPtr wtree, bool force = false, VRTransformPtr caster = 0, Vec3d dir = Vec3d(0,0,-1), bool skipVols = false);
        void drag(VRIntersection i, VRTransformWeakPtr caster);
        void drop(VRDeviceWeakPtr dev = VRDevicePtr(0), VRTransformWeakPtr beacon = VRTransformPtr(0));
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
        VRTransformPtr getDraggedObject(VRTransformPtr beacon = 0);
        VRTransformPtr getDraggedGhost();
        VRIntersection getLastIntersection();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
