#ifndef VRINTERSECT_H_INCLUDED
#define VRINTERSECT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <vector>
#include <map>

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRGeometry;
class VRObject;
class VRTransform;
typedef VRFunction<VRDevice*> VRDevCb;

class VRIntersect {
    private:
        VRObject* obj = 0;//hit object
        bool dnd = true;//drag n drop
        bool showHit = false;//show where the hitpoint lies
        //VRScene* scene;
        VRTransform* dragged = 0;
        VRGeometry* cross = 0;
        vector<VRObject*> dynTrees;
        VRObject* dynTree = 0;

        Pnt3f hitPoint;
        Vec2f hitTexel;

        map<VRObject*, VRDevCb* > int_fkt_map;
        map<VRObject*, VRDevCb* > dra_fkt_map;
        //map<VRObject*, VRDevCb* > dro_fkt_map;
        VRDevCb* drop_fkt;

        void intersect(VRTransform* caster, VRObject* tree, VRDevice* dev = 0);
        void drag(VRTransform* caster, VRObject* tree, VRDevice* dev = 0);
        void drop(VRDevice* dev = 0);

        void initCross();

    public:
        VRIntersect();
        ~VRIntersect();

        VRDevCb* addIntersect(VRTransform* caster, VRObject* tree);
        VRDevCb* addDrag(VRTransform* caster, VRObject* tree);
        VRDevCb* getDrop();

        void toggleDragnDrop(bool b);
        void showHitPoint(bool b);

        VRObject* getCross();

        void addDynTree(VRObject* o);
        void remDynTree(VRObject* o);
        void updateDynTree(VRObject* a);

        Pnt3f getHitPoint();
        Vec2f getHitTexel();
        VRObject* getHitObject();
        VRTransform* getDraggedObject();
};

OSG_END_NAMESPACE;

#endif // VRINTERSECT_H_INCLUDED
