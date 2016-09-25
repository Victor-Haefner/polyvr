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

        Vec3f elevate(Vec2f p, float h);

    private:
        map<string, VRGeometryPtr> meshes;
        map<string, StreetType> types;
        map<string, Vec4f> signTCs;
        map<string, VRAnnotationEnginePtr> annotations;
        VRMaterialPtr matStreet;
        VRMaterialPtr matSigns;
        VRMaterialPtr matLights;

        void makeStreetLight(StreetSegment* seg, VRGeoData* geo);
        void makeStreetSign(Vec3f pos, string name, VRGeoData* geo);
        void makeStreetNameSign(StreetSegment* seg, VRAnnotationEnginePtr ae);
        void makeSegment(StreetSegment* s, map<string, StreetJoint*>& joints, VRGeoData* geo, VRGeoData* geo2);
        void makeCurve(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo);
        void makeJoint(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo);
        void makeJoint31(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo, VRGeoData* signs2);

        void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, Vec2f tc1, Vec2f tc2, Vec2f tc3, Vec2f tc4);
        void pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, Vec4f tc = Vec4f(0,1,0,1));
        void pushStreetQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, bool isSide = false, Vec3f tc = Vec3f(0,1,1));
        void pushTriangle(Vec3f c, Vec3f a1, Vec3f a2, Vec3f normal, VRGeoData* geo, Vec2f t1, Vec2f t2, Vec2f t3 );
};

OSG_END_NAMESPACE;

#endif // MODULESTREETS_H



