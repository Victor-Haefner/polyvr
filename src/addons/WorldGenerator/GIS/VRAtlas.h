#ifndef VRATLAS_H
#define VRATLAS_H

#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include "../terrain/VRTerrain.h"
#include "GISFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAtlas : public std::enable_shared_from_this<VRAtlas>  {
    private:
        enum TYPE {
            INNERQUAD = 0,
            INNERRING = 1,
            OUTERRING = 2
        };
        struct Patch {
            string id;
            int LODlvl;
            VRTerrainPtr terrain;

            Patch(string sid, int lvl, VRTerrainPtr ter);
            Patch();
            ~Patch();
        };
        struct Layout {
            int currentLODlvl = 0;
            int currentMaxLODlvl = 0;
            Vec3d currentOrigin = Vec3d(0,0,0);
            list<vector<Patch>> rings;
            vector<Patch> innerQuad;
            vector<Patch> innerRing;
            vector<Patch> outerRing;
            list<Patch> toDestroy;
            list<Patch> toGenerate;
        };
        float size = 64.0;
        int LODMax = 3;
        int patchcount = 0;

        string filepath;
        VRTransformPtr atlas;
        VRUpdateCbPtr updatePtr;
        Layout layout;
        void update();
        void shiftLayout();
        void downSize();
        void upSize();
        void addInnerQuad(int lvl);
        void addInnerRing(int lvl);
        void addOuterRing(int lvl);
        VRGeometryPtr generatePatch(string id);
        VRTerrainPtr generateTerrain(string id, int lvl, float edgeLength);

    public:
        VRAtlas();
        ~VRAtlas();
        static VRAtlasPtr create();
		//VRAtlasPtr ptr();

        VRTransformPtr setup();
        void test();
};

OSG_END_NAMESPACE;

#endif // VRATLAS_H
