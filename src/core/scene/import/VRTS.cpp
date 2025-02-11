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
#include "core/utils/system/VRSystem.h"

OSG_BEGIN_NAMESPACE;

void loadTS(string filename, VRTransformPtr res, map<string, string> options) {
    VRGeoData geo;
    string currentName;
    auto mat = VRMaterial::create("tsMat");
    mat->setLit(false);
    mat->setDiffuse(Color3f(0.8,0.8,0.6));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    Vec3d n(0,1,0);

    Vec3d offset;
    if (options.count("offset")) toValue(options["offset"], offset);

    auto pushGeo = [&]() {
        if (geo.size() == 0) return;
        auto Geo = geo.asGeometry( getFileName(filename) + "_" + currentName );
        Geo->setMaterial(mat);
        Geo->updateNormals();
        res->addChild( Geo );
    };

    auto pushVert = [&](int idx, Vec3d p) {
        if (idx >= geo.size()) {
            while (idx > geo.size()) {
                geo.pushVert(Vec3d(), n);
            }
            geo.pushVert(p, n);
        } else {
            //geo.setVert(idx, p + offset, n);
        }
    };

    ifstream file(filename.c_str());
    string line;
    while (getline(file, line)) {
        auto data = splitString(line);
        string cmd = stripString(data[0]);

        if (cmd == "NAME") {
            auto v = splitString(line, '"');
            if (v.size() > 2) currentName = v[1];
        }

        if (cmd == "ATOM" || cmd == "PATOM") {
            size_t idx = toLong(data[1]);
            size_t idy = toLong(data[2]);
            if (idy < geo.size()) pushVert(idx, Vec3d(geo.getPosition(idy)));
        }

        if (cmd == "VRTX" || cmd == "PVRTX") {
            size_t idx = toLong(data[1]);
            Vec3d p(toDouble(data[2]), toDouble(data[4]), toDouble(data[3]));
            p += offset;
            p[2] *= -1;
            pushVert(idx, p);
        }

        if (cmd == "TRGL") {
            geo.pushTri( toInt(data[1]), toInt(data[2]), toInt(data[3]) );
        }

        if (cmd == "END") {
            pushGeo();
            geo = VRGeoData();
        }
    }

    file.close();
    pushGeo();
}

void writeTS(VRGeometryPtr geo, string path) {
    // TODO
}

OSG_END_NAMESPACE;
