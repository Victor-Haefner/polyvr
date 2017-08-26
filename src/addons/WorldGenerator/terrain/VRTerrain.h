#ifndef VRTERRAIN_H_INCLUDED
#define VRTERRAIN_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"
#include "core/math/polygon.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/WorldGenerator/VRWorldModule.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Action;

class VREmbankment {
    private:
        pathPtr p1, p2, p3, p4;
        VRPolygon area;

    public:
        VREmbankment(pathPtr p1, pathPtr p2, pathPtr p3, pathPtr p4);
        ~VREmbankment();
        static VREmbankmentPtr create(pathPtr p1, pathPtr p2, pathPtr p3, pathPtr p4);

        bool isInside(Vec2d p);
        float getHeight(Vec2d p);

        VRGeometryPtr createGeometry();
};

class VRTerrain : public VRGeometry, public VRWorldModule {
    private:
        static string vertexShader;
        static string fragmentShader;
        static string tessControlShader;
        static string tessEvaluationShader;

        Vec2d size = Vec2d(100,100);
        Vec2f texelSize = Vec2f(0.01,0.01); // shader parameter
        float resolution = 1; // shader parameter
        float heightScale = 1; // shader parameter
        double grid = 64;
        VRTexturePtr tex;
        VRMaterialPtr mat;
        shared_ptr<vector<float>> physicsHeightBuffer;

        map<string, VREmbankmentPtr> embankments;

        void updateTexelSize();
        void setupGeo();
        void setupMat();

    public:
        VRTerrain(string name);
        ~VRTerrain();
        static VRTerrainPtr create(string name = "terrain");

        void setSimpleNoise();

        void setParameters( Vec2d size, double resolution, double heightScale );
        void setMap( VRTexturePtr tex, int channel = 3 );
        void loadMap( string path, int channel = 3 );

        virtual bool applyIntersectionAction(Action* ia);

        void physicalize(bool b);

        void projectOSM();

        float getHeight( const Vec2d& p );
        void elevatePoint( Vec3d& p, float offset = 0 );
        void elevatePose( posePtr p, float offset = 0 );
        void elevateObject( VRTransformPtr p, float offset = 0 );
        void projectTangent( Vec3d& t, Vec3d p);

        void paintHeights(string path);
        void addEmbankment(string ID, pathPtr p1, pathPtr p2, pathPtr p3, pathPtr p4);
};

OSG_END_NAMESPACE;

#endif // VRTERRAIN_H_INCLUDED
