#ifndef MAPGEOMETRYGENERATOR_H
#define	MAPGEOMETRYGENERATOR_H

#include <OpenSG/OSGGeoProperties.h>

class TextureManager;

OSG_BEGIN_NAMESPACE;
using namespace std;

class World;

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
    private:
        TextureManager* texManager;

    public:
        MapGeometryGenerator(TextureManager* texManager);

        void updateWorldGeometry(World* world);
};

OSG_END_NAMESPACE;

#endif	/* MAPGEOMETRYGENERATOR_H */

