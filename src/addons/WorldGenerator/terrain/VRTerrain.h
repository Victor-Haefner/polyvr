#ifndef VRTERRAIN_H_INCLUDED
#define VRTERRAIN_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/polygon.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/WorldGenerator/VRWorldModule.h"

namespace boost { class recursive_mutex; }

using namespace std;
OSG_BEGIN_NAMESPACE;

class Action;

class VREmbankment : public VRGeometry {
    private:
        PathPtr p1, p2, p3, p4;
        VRPolygon area;

    public:
        VREmbankment(PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4);
        ~VREmbankment();
        static VREmbankmentPtr create(PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4);

        bool isInside(Vec2d p);
        float getHeight(Vec2d p);
        vector<Vec3d> probeHeight( Vec2d p);

        void createGeometry();
};

class VRTerrainGrid {
    public:
        Vec3d p00;
        Vec3d p10;
        Vec3d p01;
        Vec3d p11;

        //Vec2d size = Vec2d(100,100); // deprecated
        double grid = 64;

        VRTerrainGrid();

        void setRectangle(double width, double height);

        Vec2d approxSize();
        Vec2d computeTexel(VRTexturePtr tex, int margin = 0);
        Vec2d computeTexelSize(VRTexturePtr tex);
};

class VRTerrain : public VRGeometry, public VRWorldModule {
    private:
        static string vertexShader;
        static string fragmentShader;
        static string fragmentShaderDeferred;
        static string tessControlShader;
        static string tessEvaluationShader;
        static string vertexShader_es2;
        static string fragmentShader_es2;

        VRTerrainGrid grid;
        Vec2d texelSize = Vec2d(0.01,0.01); // shader parameter
        float resolution = 1; // shader parameter
        float heightScale = 1; // shader parameter
        double LODfac = 1.0;
        bool localMesh = false;
        bool useHeightoffset = false;
        bool doInvertSatY = false;
        bool doInvertTopoY = false;
        float heightoffset = 0.0;
        VRTexturePtr heigthsTex;
        VRTexturePtr satTex;
        Vec4d heightsRect = Vec4d(0,0,1,1);
        Vec4d satRect = Vec4d(0,0,1,1);
        VRMaterialPtr mat;
        shared_ptr<vector<float>> physicsHeightBuffer;

        vector<double> oldMgParams;

        VRPlanetWeakPtr planet;
        Vec2d planetCoords;
        PosePtr pSectorInv;

        map<string, VREmbankmentPtr> embankments;
        vector<Vec3d> edgePoints;
        vector<vector<vector<Vec3d>>> meshTer;

        VRMutex& mtx(); // physics

        void setHeightTexture(VRTexturePtr t);
        void updateTexelSize();
        void setupMat();
        bool createMultiGrid(VRCameraPtr cam, double res);

        void vrPhysicalize();

    public:
        VRTerrain(string name, bool localized = false);
        ~VRTerrain();
        static VRTerrainPtr create(string name = "terrain", bool localized = false);
        VRTerrainPtr ptr();

        void setSimpleNoise();
        Boundingbox getBoundingBox();

        void setParameters( Vec2d size, double resolution, double heightScale, float w = 0, float aT = 1e-4, Color3f aC = Color3f(0.7,0.9,1), bool isLit = true);
        void setLocalized(bool in);
        void curveMesh(VRPlanetPtr planet, Vec2d coords, PosePtr pSInv);
        void setMeshTer(vector<vector<vector<Vec3d>>> in);
        void setWaterLevel(float w);
        void setLit(bool isLit);
        void setInvertY(bool invertSatY, bool invertTopoY);
        void setAtmosphericEffect(float thickness, Color3f color);
        void setHeightScale(float s);
        void setMap( VRTexturePtr tex, int channel = 0, Vec4d rect = Vec4d(0,0,1,1) );
        void setTexture( VRTexturePtr tex, Color4f mCol, float mAmount, Vec4d rect = Vec4d(0,0,1,1) );
        void loadMap( string path, int channel = 0, bool shout = true );
        VRTexturePtr getMap();
        VRTexturePtr getTexture();
        Vec2d getTexelSize();
        Vec2d getSize();
        double getGrid();
        void setupGeo(VRCameraPtr cam = 0);
        void setLODFactor(double in);
        double getLODFactor();

        Vec2d toUVSpace(Vec2d uv);
        Vec2d fromUVSpace(Vec2d uv);

        virtual bool applyIntersectionAction(Action* ia) override;

        void physicalize(bool b);

        void projectOSM();

        Vec2d getTexCoord( Vec2d p );
        double getHeight( Vec2d p, bool useEmbankments = true );
        Vec3d getNormal( Vec3d p );

        void setHeightOffset(bool enab);
        double getHeightOffset();

        Vec3d elevatePoint( Vec3d p, float offset = 0, bool useEmbankments = true );
        void elevatePose( PosePtr p, float offset = 0 );
        void elevatePolygon( VRPolygonPtr p, float offset = 0, bool useEmbankments = true );
        void elevateObject( VRTransformPtr p, float offset = 0 );
        void elevateVertices( VRGeometryPtr p, float offset = 0 );
        void projectTangent( Vec3d& t, Vec3d p);

        void flatten(vector<Vec2d> perimeter, float h);
        void paintHeights( string woods, string gravel );
        void paintHeights( string path, Color4f mCol = Color4f(1,1,1,1), float mAmount = 0, Vec4d rect = Vec4d(0,0,1,1) );
        void paintHeights( VRTexturePtr tex, Color4f mCol = Color4f(1,1,1,1), float mAmount = 0, Vec4d rect = Vec4d(0,0,1,1) );
        void addEmbankment(string ID, PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4);

        vector<Vec3d> probeHeight( Vec2d p);


        void clear();
};

OSG_END_NAMESPACE;

#endif // VRTERRAIN_H_INCLUDED
