#ifndef VRATLAS_H
#define VRATLAS_H

#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/VRTransform.h"
#include "../terrain/VRTerrain.h"
#include "GISFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAtlas : public std::enable_shared_from_this<VRAtlas>  {
    private:
        string filepath;
        VRTransformPtr atlas;
        VRUpdateCbPtr updatePtr;
        void update();
        VRGeometryPtr generatePatch(string id);
        VRTerrainPtr generateTerrain(string id, float edgeLength);

    public:
        VRAtlas();
        ~VRAtlas();
        static VRAtlasPtr create();
		//VRAtlasPtr ptr();

        VRTransformPtr setup();
        void test();
};

OSG_END_NAMESPACE;

#endif // VRATLAS_H
