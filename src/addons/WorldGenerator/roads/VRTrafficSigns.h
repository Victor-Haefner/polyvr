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
        int ID;
        VRGeometryPtr selfPtr;
        VRTextureMosaicPtr megaTex;
        string type;
        string country;
        Vec2i texID;

    public:
        VRTrafficSigns();
        ~VRTrafficSigns();

        static VRTrafficSignsPtr create();
        void setMegaTexture(VRTextureMosaicPtr megaTex);

        PosePtr getPosition(int ID);
        VRGeometryPtr getGeometry();

        void setMatrix(string country);
};

OSG_END_NAMESPACE;

#endif // VRTRAFFICSIGNS_H_INCLUDED
