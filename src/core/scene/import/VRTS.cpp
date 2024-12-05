#include "VRTS.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include <fstream>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"

OSG_BEGIN_NAMESPACE;

void loadTS(string filename, VRTransformPtr res, map<string, string> options) {
    VRGeoData geo;
    auto mat = VRMaterial::create("tsMat");
    mat->setLit(false);
    mat->setDiffuse(Color3f(0.8,0.8,0.6));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    Vec3d offset;
    if (options.count("offset")) toValue(options["offset"], offset);

    ifstream file(filename.c_str());
    string line;
    Vec3d n(0,1,0);
    while (getline(file, line)) {
        auto data = splitString(line);
        if (data[0] == "VRTX") {
            Vec3d p(toDouble(data[2]), toDouble(data[3]), toDouble(data[4]));
            geo.pushVert(p + offset, n);
        }

        if (data[0] == "TRGL") {
            geo.pushTri( toInt(data[1]), toInt(data[2]), toInt(data[3]) );
        }
    }
    file.close();

    auto Geo = geo.asGeometry(filename);
    Geo->setMaterial(mat);
	Geo->updateNormals();
    res->addChild( Geo );
}

void writeTS(VRGeometryPtr geo, string path) {
    // TODO
}

OSG_END_NAMESPACE;
