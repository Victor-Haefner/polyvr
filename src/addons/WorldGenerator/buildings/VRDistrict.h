#ifndef MODULEBUILDINGS_H
#define MODULEBUILDINGS_H

#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRTextureMosaic.h"
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
        static string matFDShdr;

        map<string, VRBuildingPtr> buildings;
        VRGeometryPtr facades;
        VRGeometryPtr roofs;
        VRTextureMosaicPtr texture;
        map<string, vector<Vec2i>> chunkIDs;
        VRMaterialPtr b_mat;

        void computeGeometry();
        void init();

    public:
        VRDistrict();
        ~VRDistrict();

        static VRDistrictPtr create();
        VRDistrictPtr ptr();

        void addTexture(VRTexturePtr tex, string type);
        void addTextures(string folder, string type);
        VRTextureMosaicPtr getTexture();

        Vec4d getChunkUV(string type, int i);

        void addBuilding( VRPolygonPtr p, int stories, string housenumber, string street );
        void remBuilding( string street, string housenumber );
        void clear();
};

OSG_END_NAMESPACE;

#endif // MODULEBUILDINGS_H

