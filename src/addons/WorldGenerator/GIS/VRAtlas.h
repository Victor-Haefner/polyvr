#ifndef VRATLAS_H
#define VRATLAS_H

#include <string>
#include <deque>
#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include "../terrain/VRTerrain.h"
#include "GISFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAtlas : public std::enable_shared_from_this<VRAtlas>  {
    private:
        struct Boundary {
            double minEast = 0.0;
            double maxEast = 0.0;
            double minNorth = 0.0;
            double maxNorth = 0.0;

            Boundary(double minEast, double maxEast, double minNorth, double maxNorth);
            Boundary();
            ~Boundary();
        };

        struct Patch {
            string id;
            vector<string> children;
            int LODlvl;
            VRTerrainPtr terrain;
            VRMapManagerPtr mapMgr;
            Vec2d coords = Vec2d(0,0);
            Vec3d localPos = Vec3d(0,0,0);
            float edgeLength;
            float localHeightoffset = 0.0;
            string orthoPic;
            string heightPic;
            string east;
            string north;
            string els;
            bool visible = false;
            bool visibleToBe = false;
            bool allowedToDeload = false;
            bool loaded = false;

            void paint();
            Patch(string sid, int lvl);
            Patch();
            ~Patch();
        };

        float size = 100.0;
        float LODviewHeight = 500.0;
        float scaling = 1.0;
        Vec3d origin = Vec3d(0,0,0);
        Vec2d atlasOrigin = Vec2d(0.0,0.0);
        Boundary bounds;
        int LODMax = 0;
        bool stop = false;
        deque<string> patchQueue;
        deque<string> invisQueue;
        VRMapManagerPtr mapMgr;

        bool isValid();

        map<string,Patch> allPatchesByID;
        map<string,int> loadedPatches;
        map<string,int> visiblePatchesByID;
        map<string,int> levelPatchesByID;

        string filepath;
        VRTransformPtr atlas;
        VRUpdateCbPtr updatePtr;
        string localPathOrtho = "";
        string localPathHeight = "";
        int sinceLastMovement = 0;
        Vec3d lastPos = Vec3d(0,0,0);
        VRGeometryPtr debugQuad;
        bool debugMode = false;
        bool toBeReset = false;

        void update();

        void addInnerQuad(int lvl, Vec2d nOrigin);
        void addInnerRing(int lvl, Vec2d nOrigin);
        void addOuterRing(int lvl, Vec2d nOrigin);
        void handleJobQueue();
        void resetJobQueue();

        void setCoords(Patch& pat);
        void debugPaint();

        VRGeometryPtr generatePatch(string id);
        VRTerrainPtr generateTerrain(string id, int lvlh);

    public:
        VRAtlas();
        ~VRAtlas();
        static VRAtlasPtr create();
		//VRAtlasPtr ptr();

        VRTransformPtr setup();
        void setCoordOrigin(double east, double north);
        void setBoundary(double minEast, double maxEast, double minNorth, double maxNorth);
        void setLocalPaths(string ortho, string height);
        void setScale(float s);
        //void setParameters();
        void setMapManager(VRMapManagerPtr mgr);
        void setDebug(bool mode);
        void repaint();
        Vec3d getLocalPos(double east, double north);
        void test();
        void toggleUpdater();
};

OSG_END_NAMESPACE;

#endif // VRATLAS_H
