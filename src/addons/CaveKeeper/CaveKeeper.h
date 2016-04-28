#ifndef CAVEKEEPER_H_INCLUDED
#define CAVEKEEPER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <map>
#include <string>
#include <vector>
#include "CKOctree.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRMaterial;

// -------------- TODO --------
// check if element exists when adding to octree
// when changing the world, only change the minimum data, do not rebuild the full mesh!
// bumpmap

class BlockWorld {
    public:
		CKOctree* tree;

        VRObjectPtr getAnchor();

    private:
        VRObjectPtr anchor;

        map<string, VRMaterialPtr> materials;
        map<int, VRGeometryPtr> chunks;
        VRUpdatePtr updatePtr;

        // octree population algorithm

        void createPlane(int w);
        void createSphere(int r, Vec3i p0);
        // mesh methods

        VRMaterialPtr initMaterial(string texture);
		VRGeometryPtr createChunk(vector<CKOctree::element*>& elements);
        VRGeometryPtr initChunk();

		void appendToVector(vector<CKOctree::element*>* elements, CKOctree::element* e);

        // update methods

        void updateShaderCamPos();

    protected:
        BlockWorld();
        ~BlockWorld();

        void initWorld();

        void redraw(int chunk_id = 0);
};

class CaveKeeper : public BlockWorld {
    private:
        void placeLight(Vec3f p);

    public:
        CaveKeeper();
        ~CaveKeeper();

        void dig(VRDevicePtr dev);

        void place(VRDevicePtr dev, string s, VRTransformPtr geo);
};

OSG_END_NAMESPACE

#endif // CAVEKEEPER_H_INCLUDED
