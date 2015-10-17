#ifndef MAPGEOMETRYGENERATOR_H
#define	MAPGEOMETRYGENERATOR_H

#include "core/scene/VRScene.h"
#include "core/tools/VRText.h"
#include "core/objects/geometry/VRGeometry.h"
#include "World.h"
#include "StreetAlgos.h"
#include "TextureManager.h"
#include "Timer.h"

#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct GeometryData {
    GeoPnt3fPropertyRecPtr      pos;
    GeoVec3fPropertyRefPtr      norms;
    GeoUInt32PropertyRefPtr     inds;
    GeoVec2fPropertyRecPtr      texs;
    GeoVec2fPropertyRecPtr      texs2;

    GeometryData();
    void clear();
};

class MapGeometryGenerator {
public:
    //VRScene* scene;
    TextureManager* texManager;

    //MapGeometryGenerator(VRScene* scene, TextureManager* texManager) {
    MapGeometryGenerator(TextureManager* texManager);

    void updateWorldGeometry(World* world);
};

OSG_END_NAMESPACE;

#endif	/* MAPGEOMETRYGENERATOR_H */

