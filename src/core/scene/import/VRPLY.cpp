#include "VRPLY.h"
#include "core/objects/geometry/VRGeometry.h"

#include <fstream>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"

OSG_BEGIN_NAMESPACE;

struct property {
    string type;
    string name;
    property(string type, string name) : type(type), name(name) {}
};

struct element {
    string type;
    int N;
    vector<property> properties;
    element(string type, int N) : type(type), N(N) {}
};

VRGeometryPtr loadPly(string filename) {
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

    ifstream file(filename.c_str());
    string line;
    list<element> elements;
    while (getline(file, line)) {
        if (line == "end_header") break;
        auto data = splitString(line, ' ');
        if (data[0] == "element") elements.push_back( element(data[1], toInt(data[2]) ) );
        if (data[0] == "property") {
            if (data.size() == 3) elements.back().properties.push_back( property(data[1], data[2]) );
            else elements.back().properties.push_back( property(data[1], data[2]) );
        }
    }

    int N = 0;
    for (auto e : elements) N += e.N;
    VRProgress progress("load PLY " + filename, N);

    for (auto e : elements) {
        if (e.type == "vertex") {
            Vec3f p, n;
            Vec3i c;
            Vec2f t;
            bool doP = 0, doN = 0, doC = 0, doT = 0;
            for (int i=0; i<e.N; i++) {
                progress.update(1);
                getline(file, line);
                istringstream iss(line);
                for (auto prop : e.properties) {
                    if (prop.name == "x") { iss >> p[0]; doP = 1; }
                    if (prop.name == "y") { iss >> p[1]; doP = 1; }
                    if (prop.name == "z") { iss >> p[2]; doP = 1; }
                    if (prop.name == "nx") { iss >> n[0]; doN = 1; }
                    if (prop.name == "ny") { iss >> n[1]; doN = 1; }
                    if (prop.name == "nz") { iss >> n[2]; doN = 1; }
                    if (prop.name == "r") { iss >> c[0]; doC = 1; }
                    if (prop.name == "g") { iss >> c[1]; doC = 1; }
                    if (prop.name == "b") { iss >> c[2]; doC = 1; }
                    if (prop.name == "s") { iss >> t[0]; doT = 1; }
                    if (prop.name == "t") { iss >> t[1]; doT = 1; }
                }

                if (doP) Pos->addValue(p);
                if (doN) Norms->addValue(n);
                if (doC) Cols->addValue(Vec3f(c[0]/255., c[1]/255., c[2]/255.));
                if (doT) Tex->addValue(t);
            }
            continue;
        }

        if (e.type == "face") {
            int N = 0, lastN = 0, k = 0, i1 = 0;
            for (int i=0; i<e.N; i++) {
                progress.update(1);
                getline(file, line);
                istringstream iss(line);
                iss >> N;
                if (N != lastN) {
                    if (N == 1) { Type->addValue(GL_POINTS); cout << "add GL_POINTS type\n"; }
                    if (N == 2) { Type->addValue(GL_LINES); cout << "add GL_LINES type\n"; }
                    if (N == 3) { Type->addValue(GL_TRIANGLES); cout << "add GL_TRIANGLES type\n"; }
                    if (N == 4) { Type->addValue(GL_QUADS); cout << "add GL_QUADS type\n"; }
                    if (k > 0) { Length->addValue(k*N); cout << "add length " << k*N << endl; }
                    k = 0;
                    lastN = N;
                }
                for (int j=0; j<N; j++) {
                    iss >> i1;
                    Indices->addValue(i1);
                }
                k++;
            }
            if (k > 0) { Length->addValue(k*N); cout << "add length " << k*N << endl; }
            continue;
        }

        cout << "\nWarning! unknown element " << e.type << " with " << e.N << " entries.\n";
    }
    file.close();

    cout << "\n summary:\n";
    cout << "  file header:";
    for (auto e : elements) cout << " " << e.N;
    cout << endl;
    cout << "  types: " << Type->size() << endl;
    cout << "  lengths: " << Length->size() << endl;
    cout << "  indices: " << Indices->size() << endl;
    cout << "  positions: " << Pos->size() << endl;
    cout << "  normals: " << Norms->size() << endl;
    cout << "  colors: " << Cols->size() << endl;
    cout << "  texcoords: " << Tex->size() << endl;

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    if (Norms->size() == Pos->size()) geo->setNormals(Norms);
    if (Cols->size()  == Pos->size()) geo->setColors(Cols);
    if (Tex->size()   == Pos->size()) geo->setTexCoords(Tex);
    geo->setMaterial(Mat);

    VRGeometryPtr res = VRGeometry::create(filename);
    res->setMesh(geo);
    return res;
}

OSG_END_NAMESPACE;
