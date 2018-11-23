#ifndef VRTRAFFICSIGNS_H_INCLUDED
#define VRTRAFFICSIGNS_H_INCLUDED

#include <map>
#include <OpenSG/OSGVector.h>
#include "VRRoadBase.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTrafficSigns : public VRRoadBase {
    private:
        struct TrafficSign {
            int ID = -1;
            VRTransformPtr t;
            string type;
            string OSMID;

            TrafficSign();
            ~TrafficSign();
        };

        VRGeometryPtr selfPtr;
        VRTextureMosaicPtr megaTex;
        map<string,map<string,Vec2i>> matrix;
        map<Vec2i,string> signNameByID;
        vector<string> types;
        vector<vector<string>> allFileNames;
        string country;
        map<int,TrafficSign> trafficSignsByID;
        VRObjectPtr baseModel;

        int maxSignsPerRow = 40;

    public:
        VRTrafficSigns();
        ~VRTrafficSigns();

        static VRTrafficSignsPtr create();
        void addTrafficSign(string type, PosePtr pose);
        void loadTextures();
        void setMegaTexture(VRTextureMosaicPtr megaTex);
        string getName(Vec2i ID);
        string getOSMTag(Vec2i ID);

        PosePtr getPosition(int ID);
        VRTextureMosaicPtr getTextureMosaic();
        VRGeometryPtr getGeometry();

        void setMatrix(string country);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIGNS_H_INCLUDED
