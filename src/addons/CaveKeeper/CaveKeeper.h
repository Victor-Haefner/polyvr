#ifndef CAVEKEEPER_H_INCLUDED
#define CAVEKEEPER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <map>
#include <string>
#include <vector>
#include "CKOctree.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;
class VRGeometry;
class VRMaterial;
class VRObject;
class VRTransform;

// -------------- TODO --------
// check if element exists when adding to octree
// when changing the world, only change the minimum data, do not rebuild the full mesh!
// bumpmap

class BlockWorld {
    public:
		CKOctree* tree;

        VRObject* getAnchor();

    private:
        VRObject* anchor;

        map<string, VRMaterial*> materials;
        map<int, VRGeometry*> chunks;

        // octree population algorithm

        void createPlane(int w);
        void createSphere(int r, Vec3i p0);
        // mesh methods

        VRMaterial* initMaterial(string texture);

		VRGeometry* createChunk(vector<CKOctree::element*>& elements);

        VRGeometry* initChunk();

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

        void dig(VRDevice* dev);

        void place(VRDevice* dev, string s, VRTransform* geo);
};

OSG_END_NAMESPACE

#endif // CAVEKEEPER_H_INCLUDED
