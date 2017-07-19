#ifndef MODULESTREETS_H
#define MODULESTREETS_H

#include "BaseModule.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include <map>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGeoData;
class StreetJoint;
class StreetSegment;

struct StreetType {
    string type = "road";
    float width = 1;
    float bridgeHeight = 0;
    VRMaterialPtr mat;
    bool cars = false;
    StreetType() {}
    StreetType(string t, float w, float b, VRMaterialPtr m, bool c)
                : type(t), width(w), bridgeHeight(b), mat(m), cars(c) {}
};

class ModuleStreets: public BaseModule {
    public:
        ModuleStreets(bool t, bool p);

        virtual void loadBbox(MapGrid::Box bbox);
        virtual void unloadBbox(MapGrid::Box bbox);
        void physicalize(bool b);

        Vec3d elevate(Vec2d p, float h);

    private:
        map<string, VRGeometryPtr> meshes;
        map<string, StreetType> types;
        map<string, Vec4d> signTCs;
        map<string, VRAnnotationEnginePtr> annotations;
        VRMaterialPtr matStreet;
        VRMaterialPtr matSigns;
        VRMaterialPtr matLights;

        void makeStreetLight(StreetSegment* seg, VRGeoData* geo);
        void makeStreetSign(Vec3d pos, string name, VRGeoData* geo);
        void makeStreetNameSign(StreetSegment* seg, VRAnnotationEnginePtr ae);
        void makeSegment(StreetSegment* s, map<string, StreetJoint*>& joints, VRGeoData* geo, VRGeoData* geo2);
        void makeCurve(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo);
        void makeJoint(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo);
        void makeJoint31(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo, VRGeoData* signs2);

        void pushQuad(Vec3d a1, Vec3d a2, Vec3d b2, Vec3d b1, Vec3d normal, VRGeoData* geo, Vec2d tc1, Vec2d tc2, Vec2d tc3, Vec2d tc4);
        void pushQuad(Vec3d a1, Vec3d a2, Vec3d b2, Vec3d b1, Vec3d normal, VRGeoData* geo, Vec4d tc = Vec4d(0,1,0,1));
        void pushStreetQuad(Vec3d a1, Vec3d a2, Vec3d b2, Vec3d b1, Vec3d normal, VRGeoData* geo, bool isSide = false, Vec3d tc = Vec3d(0,1,1));
        void pushTriangle(Vec3d c, Vec3d a1, Vec3d a2, Vec3d normal, VRGeoData* geo, Vec2d t1, Vec2d t2, Vec2d t3 );
};

OSG_END_NAMESPACE;

#endif // MODULESTREETS_H



