#ifndef VRTERRAIN_H_INCLUDED
#define VRTERRAIN_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/polygon.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/WorldGenerator/VRWorldModule.h"
#include <boost/thread/recursive_mutex.hpp>

using namespace std;
OSG_BEGIN_NAMESPACE;

class Action;

class VREmbankment : public VRGeometry{
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

class VRTerrain : public VRGeometry, public VRWorldModule {
    private:
        static string vertexShader;
        static string fragmentShader;
        static string fragmentShaderDeferred;
        static string tessControlShader;
        static string tessEvaluationShader;

        Vec2d size = Vec2d(100,100);
        Vec2f texelSize = Vec2f(0.01,0.01); // shader parameter
        float resolution = 1; // shader parameter
        float heightScale = 1; // shader parameter
        double grid = 512;
        //double grid = 64;
        VRTexturePtr heigthsTex;
        VRMaterialPtr mat;
        shared_ptr<vector<float>> physicsHeightBuffer;

        map<string, VREmbankmentPtr> embankments;
        vector<Vec3d> edgePoints;
        vector<vector<Vec3d>> meshTer;

        boost::recursive_mutex& mtx(); // physics

        void setHeightTexture(VRTexturePtr t);
        void updateTexelSize();
        void setupGeo();
        void setupMat();

        void btPhysicalize();
        void vrPhysicalize();

    public:
        VRTerrain(string name);
        ~VRTerrain();
        static VRTerrainPtr create(string name = "terrain");
        VRTerrainPtr ptr();

        void setSimpleNoise();
        Boundingbox getBoundingBox();

        void setParameters( Vec2d size, double resolution, double heightScale, float w = 0, float aT = 1e-4, Color3f aC = Color3f(0.7,0.9,1));
        void setEdgepoints(vector<Vec3d> in);
        void setMeshTer(vector<vector<Vec3d>> in);
        void setWaterLevel(float w);
        void setAtmosphericEffect(float thickness, Color3f color);
        void setHeightScale(float s);
        void setMap( VRTexturePtr tex, int channel = 3 );
        void loadMap( string path, int channel = 3 );
        VRTexturePtr getMap();
        Vec2f getTexelSize();
        Vec2d getSize();

        Vec2d toUVSpace(Vec2d uv);
        Vec2d fromUVSpace(Vec2d uv);

        virtual bool applyIntersectionAction(Action* ia);

        void physicalize(bool b);

        void projectOSM();

        double getHeight( const Vec2d& p, bool useEmbankments = true );
        Vec3d elevatePoint( Vec3d p, float offset = 0, bool useEmbankments = true );
        void elevatePose( PosePtr p, float offset = 0 );
        void elevatePolygon( VRPolygonPtr p, float offset = 0, bool useEmbankments = true );
        void elevateObject( VRTransformPtr p, float offset = 0 );
        void elevateVertices( VRGeometryPtr p, float offset = 0 );
        void projectTangent( Vec3d& t, Vec3d p);

        void flatten(vector<Vec2d> perimeter, float h);
        void paintHeights(string woods, string gravel);
        void addEmbankment(string ID, PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4);

        vector<Vec3d> probeHeight( Vec2d p);

        void clear();
};

OSG_END_NAMESPACE;

#endif // VRTERRAIN_H_INCLUDED
