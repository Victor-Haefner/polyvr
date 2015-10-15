#include "VRPLY.h"
#include "core/objects/geometry/VRGeometry.h"

#include <fstream>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>

OSG_BEGIN_NAMESPACE;

VRGeometryPtr loadPly(string filename) {
    ifstream file(filename.c_str());
    string s;
    for(int i=0; i<16; i++) getline(file, s); // jump to data

    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      Cols = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

    Mat->setLit(false);
    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    int N = 0;
    string line;
    while (std::getline(file, line)) {
        istringstream iss(line);
        Vec3f p, n;
        Vec3i c;
        if (!(iss >> p[0] >> p[1] >> p[2] >> n[0] >> n[1] >> n[2] >> c[0] >> c[1] >> c[2])) {
            cout << "\nBREAK PLY IMPORT AT " << N << endl;
            break;
        } // error

        Pos->addValue(p);
        Norms->addValue(n);
        Cols->addValue(Vec3f(c[0]/255., c[1]/255., c[2]/255.));
        Indices->addValue(N);
        N++;
    }
    file.close();

    Type->addValue(GL_POINTS);
    Length->addValue(N);

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setColors(Cols);
    geo->setMaterial(Mat);

    VRGeometryPtr res = VRGeometry::create(filename);
    res->setMesh(geo);
    return res;
}

OSG_END_NAMESPACE;
