#ifndef CAVEKEEPER_H_INCLUDED
#define CAVEKEEPER_H_INCLUDED

#include "core/math/octree2.h"
#include "core/objects/material/VRShader.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGSimpleMaterial.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDevice;

// -------------- TODO --------
// check if element exists when adding to octree
// when changing the world, only change the minimum data, do not rebuild the full mesh!
// bumpmap

class BlockWorld {
    public:
		octree2* tree;

        VRObject* getAnchor();

    private:
        VRObject* anchor;

        map<string, VRShader*>* shader;
        map<string, SimpleMaterialRecPtr>* materials;
        map<int, VRGeometry*>* chunks;

        // octree population algorithm

        void createPlane(int w);
        void createSphere(int r, Vec3i p0);
        // mesh methods

        SimpleMaterialRecPtr initMaterial(string texture);

		VRGeometry* createChunk(vector<octree2::element*>& elements);

        VRGeometry* initChunk();

		void appendToVector(vector<octree2::element*>* elements, octree2::element* e);

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
