#ifndef VRTRAFFICSIGNS_H_INCLUDED
#define VRTRAFFICSIGNS_H_INCLUDED

#include <map>
#include <OpenSG/OSGVector.h>
#include "VRRoadBase.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRTransform.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficSign;
class VRTrafficSigns;

class VRTrafficSign : public VRTransform {
    private:
        Vec2i ID;
        VRTransformPtr t;
        string type;
        string OSMID;

    public:
        VRTrafficSign();
        ~VRTrafficSign();
        static VRTrafficSignPtr create();

        void setID(Vec2i);
};

class VRTrafficSigns : public VRRoadBase {
    private:
        VRGeometryPtr selfPtr;
        VRTextureMosaicPtr megaTex;
        map<string,map<string,Vec2i>> matrix;
        map<Vec2i,string> signNameByID;
        vector<string> types;
        vector<vector<string>> allFileNames;
        string country;
        map<int,VRTrafficSign> trafficSignsByID;

        VRObjectPtr baseModel;
        VRGeometryPtr baseGeoSign;
        VRGeometryPtr baseGeoPole;
        VRMaterialPtr baseMaterial;
        VRMaterialPtr baseMaterialPole;

        int maxSignsPerRow = 40;

        void setupBaseSign();

    public:
        VRTrafficSigns();
        ~VRTrafficSigns();

        static VRTrafficSignsPtr create();
        VRTrafficSignPtr addTrafficSign(string type, PosePtr pose);
        void addSign(string type, PosePtr pose);
        void loadTextures();
        void setMegaTexture(VRTextureMosaicPtr megaTex);

        Vec2i getVecID(string type);
        string getName(Vec2i ID);
        string getOSMTag(Vec2i ID);

        PosePtr getPosition(int ID);
        VRTextureMosaicPtr getTextureMosaic();
        VRGeometryPtr getGeometry();

        void setMatrix(string country);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIGNS_H_INCLUDED
