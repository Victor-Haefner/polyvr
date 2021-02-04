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
        struct Level {
            int LODlvl;
            Vec3d currentOrigin = Vec3d(0,0,0);
            int type;
            vector<vector<Patch>> patches;
            Level(int t, int lvl);
            Level();
            ~Level();
        };
        struct Layout {
            int currentLODlvl = 0;
            int currentMaxLODlvl = 0;
            Vec3d origin = Vec3d(0,0,0);
            list<Level> levels;
            Level innerQuad;
            Level innerRing;
            Level outerRing;
            list<Patch> toDestroy;
            list<Patch> toGenerate;
        };
        float size = 64.0;
        int LODMax = 0;
        int patchcount = 0;

        string filepath;
        VRTransformPtr atlas;
        VRUpdateCbPtr updatePtr;
        Layout layout;
        void update();
        void downSize();
        void upSize();
        void shiftLayoutEast(Level& lev);
        void shiftLayoutWest(Level& lev);
        void shiftLayoutNorth(Level& lev);
        void shiftLayoutSouth(Level& lev);
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
