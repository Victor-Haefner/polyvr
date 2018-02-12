#ifndef MODULEBUILDINGS_H
#define MODULEBUILDINGS_H

#include "core/objects/object/VRObject.h"
#include "../VRWorldGeneratorFwd.h"
#include "../VRWorldModule.h"
#include <map>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRDistrict : public VRObject, public VRWorldModule {
    private:
        static string matVShdr;
        static string matFShdr;

        map<string, VRBuildingPtr> buildings;
        VRGeometryPtr facades;
        VRGeometryPtr roofs;
        VRMaterialPtr b_mat;

        void computeGeometry();
        void init();

    public:
        VRDistrict();
        ~VRDistrict();

        static VRDistrictPtr create();

        void setTexture(string path);

        void addBuilding( VRPolygonPtr p, int stories, string housenumber, string street );
        void remBuilding( string street, string housenumber );

        void clear();
};

OSG_END_NAMESPACE;

#endif // MODULEBUILDINGS_H

