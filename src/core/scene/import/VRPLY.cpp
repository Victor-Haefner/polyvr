#include "VRPLY.h"
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

template<class T>
T readBin(ifstream& file) {
    T v;
    file.read((char*)&v, sizeof(T));
    return v;
}

template<typename T>
bool parseBinProp(ifstream& file, property& prop, T& value) {
    if (prop.type == "float") { value = readBin<float>(file); return true; }
    if (prop.type == "uchar") { value = readBin<unsigned char>(file); return true; }
    cout << "PLY parsing ERROR: parseBinProp does not handle type " << prop.type << endl;
    return false;
}

void parseVertex(element& e, ifstream& file, VRGeoData& geo, VRProgress& progress, string format) {
    string line;
    Vec3d p;
    Vec3d n = Vec3d(0,1,0);
    Vec4i c;
    Vec2d t;
    bool doP = 0, doN = 0, doC = 0, doC4 = 0, doT = 0;
    for (int i=0; i<e.N; i++) {
        progress.update(1);
        if (format == "ascii") {
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
                if (prop.name == "a") { iss >> c[3]; doC4 = 1; }
                if (prop.name == "red") { iss >> c[0]; doC = 1; }
                if (prop.name == "green") { iss >> c[1]; doC = 1; }
                if (prop.name == "blue") { iss >> c[2]; doC = 1; }
                if (prop.name == "alpha") { iss >> c[3]; doC4 = 1; }
                if (prop.name == "s") { iss >> t[0]; doT = 1; }
                if (prop.name == "t") { iss >> t[1]; doT = 1; }
            }
        }

        if (format == "ble") { // binary little endian
            for (auto prop : e.properties) {
                bool b = false;
                if (prop.name == "x") { b = parseBinProp(file, prop, p[0]); doP = 1; }
                if (prop.name == "y") { b = parseBinProp(file, prop, p[1]); doP = 1; }
                if (prop.name == "z") { b = parseBinProp(file, prop, p[2]); doP = 1; }
                if (prop.name == "nx") { b = parseBinProp(file, prop, n[0]); doN = 1; }
                if (prop.name == "ny") { b = parseBinProp(file, prop, n[1]); doN = 1; }
                if (prop.name == "nz") { b = parseBinProp(file, prop, n[2]); doN = 1; }
                if (prop.name == "r") { b = parseBinProp(file, prop, c[0]); doC = 1; }
                if (prop.name == "g") { b = parseBinProp(file, prop, c[1]); doC = 1; }
                if (prop.name == "b") { b = parseBinProp(file, prop, c[2]); doC = 1; }
                if (prop.name == "a") { b = parseBinProp(file, prop, c[3]); doC4 = 1; }
                if (prop.name == "red") { b = parseBinProp(file, prop, c[0]); doC = 1; }
                if (prop.name == "green") { b = parseBinProp(file, prop, c[1]); doC = 1; }
                if (prop.name == "blue") { b = parseBinProp(file, prop, c[2]); doC = 1; }
                if (prop.name == "alpha") { b = parseBinProp(file, prop, c[3]); doC4 = 1; }
                if (prop.name == "s") { b = parseBinProp(file, prop, t[0]); doT = 1; }
                if (prop.name == "t") { b = parseBinProp(file, prop, t[1]); doT = 1; }
                if (!b) {
                    cout << " parseVertex failed at: " << prop.name << " p " << p << endl;
                    return;
                }
            }
        }

        if (doC4) geo.pushColor( Color4f(c[0]/255., c[1]/255., c[2]/255., c[3]/255.) );
        else if (doC) geo.pushColor( Color3f(c[0]/255., c[1]/255., c[2]/255.) );
        if (doP) geo.pushPos(p);
        if (doN) geo.pushNorm(n);
        if (doT) geo.pushTexCoord(t);
    }
}

void parseFace(element& e, ifstream& file, VRGeoData& geo, VRProgress& progress, string format) {
    string line;
    int N = 0, lastN = 0, k = 0, i1 = 0;
    for (int i=0; i<e.N; i++) {
        progress.update(1);

        if (format == "ascii") {
            getline(file, line);
            istringstream iss(line);
            iss >> N;
            if (N != lastN) {
                if (N == 1) { geo.pushType(GL_POINTS); cout << "add GL_POINTS type\n"; }
                if (N == 2) { geo.pushType(GL_LINES); cout << "add GL_LINES type\n"; }
                if (N == 3) { geo.pushType(GL_TRIANGLES); cout << "add GL_TRIANGLES type\n"; }
                if (N == 4) { geo.pushType(GL_QUADS); cout << "add GL_QUADS type\n"; }
                if (k > 0) { geo.pushLength(k*N); cout << "add length " << k*N << endl; }
                k = 0;
                lastN = N;
            }
            for (int j=0; j<N; j++) {
                iss >> i1;
                geo.pushIndex(i1);
            }
        }

        if (format == "ble") { // binary little endian
            N = readBin<unsigned char>(file);
            if (N > 4) { cout << "PLY parsing ERROR: primitive has wrong N " << N << endl; return; }
            if (N != lastN) {
                if (N == 1) { geo.pushType(GL_POINTS); cout << "add GL_POINTS type\n"; }
                if (N == 2) { geo.pushType(GL_LINES); cout << "add GL_LINES type\n"; }
                if (N == 3) { geo.pushType(GL_TRIANGLES); cout << "add GL_TRIANGLES type\n"; }
                if (N == 4) { geo.pushType(GL_QUADS); cout << "add GL_QUADS type\n"; }

                if (k > 0) { geo.pushLength(k*N); cout << "add length " << k*N << endl; }
                k = 0;
                lastN = N;
            }
            for (int j=0; j<N; j++) geo.pushIndex( readBin<unsigned int>(file) );
        }

        k++;
    }
    if (k > 0) { geo.pushLength(k*N); cout << "add length " << k*N << endl; }
}

void loadPly(string filename, VRTransformPtr res) {
    VRGeoData geo;
    auto mat = VRMaterial::create("plyMat");
    mat->setLit(false);
    mat->setDiffuse(Color3f(0.8,0.8,0.6));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    string format = "ascii";
    ifstream file(filename.c_str());
    string line;
    list<element> elements;
    int headerEnd = 0;
    while (getline(file, line)) {
        if (line == "end_header") {
            headerEnd = file.tellg();
            break;
        }
        auto data = splitString(line, ' ');
        if (data[0] == "format") {
            if (data[1] == "binary_little_endian") format = "ble";
        }
        if (data[0] == "element") elements.push_back( element(data[1], toInt(data[2]) ) );
        if (data[0] == "property") {
            if (data.size() == 3) elements.back().properties.push_back( property(data[1], data[2]) );
            else elements.back().properties.push_back( property(data[1], data[2]) );
        }
    }

    if (format == "ble") {
        cout << " PLY is binary, headerEnd is at: " << headerEnd << endl;
        file.close();
        file = ifstream(filename.c_str(), ios::binary);
        file.seekg(headerEnd);
    }

    int N = 0;
    for (auto e : elements) N += e.N;
    VRProgress progress("load PLY " + filename, N);

    for (auto e : elements) {
        if (e.type == "vertex") {
            parseVertex(e, file, geo, progress, format);
            continue;
        }

        if (e.type == "face") {
            parseFace(e, file, geo, progress, format);
            continue;
        }

        cout << "\nWarning! unknown element " << e.type << " with " << e.N << " entries.\n";
    }
    file.close();

    /*if (Pos->size() > 0) { // TODO: add to VRGeoData ?
        if (Type->size() == 0) Type->addValue(GL_POINTS);
        if (Length->size() == 0) {
            Length->addValue(int(Pos->size()));
            for (uint i=0; i< Pos->size(); i++) Indices->addValue(i);
        }
    }*/

    cout << "\n summary:\n";
    cout << "  file header:";
    for (auto e : elements) cout << " " << e.N;
    cout << endl;
    cout << "  types: " << geo.getDataSize(0) << endl;
    cout << "  lengths: " << geo.getDataSize(1) << endl;
    cout << "  indices: " << geo.getDataSize(2) << endl;
    cout << "  positions: " << geo.getDataSize(3) << endl;
    cout << "  normals: " << geo.getDataSize(4) << endl;
    cout << "  colors3: " << geo.getDataSize(5) << endl;
    cout << "  colors4: " << geo.getDataSize(6) << endl;
    cout << "  texcoords: " << geo.getDataSize(7) << endl;

    auto Geo = geo.asGeometry(filename);
    Geo->setMaterial(mat);
    res->addChild( Geo );
}

void writePly(VRGeometryPtr geo, string path) {
	if (!geo) return;

	auto pos = geo->getMesh()->geo->getPositions();
	auto norms = geo->getMesh()->geo->getNormals();
	auto cols = geo->getMesh()->geo->getColors();
	auto texc = geo->getMesh()->geo->getTexCoords();
	auto inds = geo->getMesh()->geo->getIndices();
	auto types = geo->getMesh()->geo->getTypes();
	auto lengths = geo->getMesh()->geo->getLengths();

	int Np = pos->size();
	int Nn = norms->size();
	int Nc = cols->size();
	int Nt = texc->size();
	int Nl = lengths->size();
    int Nfaces = 0;

	if (Np == 0) return;

	auto writeVertices = [&]() {
		string data;
		Pnt3d p;
		Vec3d n,c;
		Vec2d t;
        for (int i=0; i<Np; i++) {
            p = Pnt3d(pos->getValue<Pnt3f>(i));
            if (Nn == Np) n = Vec3d(norms->getValue<Vec3f>(i));
            if (Nc == Np) c = Vec3d(cols->getValue<Vec3f>(i));
            if (Nt == Np) t = Vec2d(texc->getValue<Vec2f>(i));

            data += toString(p);
            if (Nn == Np) data += " "+toString(n);
            if (Nc == Np) data += " "+toString(Vec3i(c*255));
            if (Nt == Np) data += " "+toString(t);
            data += "\n";
        }
		return data;
	};

	auto check = [&](Vec3i v) {
		return (v[0] != v[1] && v[0] != v[2] && v[1] != v[2]);
	};

	auto writeIndices = [&]() {
		int j = 0;
		Nfaces = 0;
		string data;
		for (int k=0; k<Nl; k++) {
            int t = types->getValue<UInt8>(k);
            int l = lengths->getValue<UInt32>(k);
			int p = 3;

			if (t == 4) { // triangle
				for (int i=0; i<l; i++) {
					if (i%p == 0) data += toString(p);
					int in = inds->getValue<UInt32>(j);
					data += " "+toString(in);
					if (i%p == p-1) {
						data += "\n";
						Nfaces += 1;
					}
					j += 1;
				}
			}

			if (t == 5) { // triangle strip
				for (int i=0; i<l; i++) {
					int in = inds->getValue<UInt32>(j);
					if (i == 0) data += toString(p); // first triangle of strip
					if (i < 3) data += " "+toString(in);
					if (i == 2) {
						data += "\n";
						Nfaces += 1;
					}
					if (i > 2) {
                        int in1 = inds->getValue<UInt32>(j-2);
                        int in2 = inds->getValue<UInt32>(j-1);
                        int in3 = inds->getValue<UInt32>(j);
						Vec3i veci = Vec3i(in1, in2, in3);
						if (i%2 == 1) veci = Vec3i(in2, in1, in3);
						if (check(veci)) {
							data += toString(p)+' '+toString(veci)+"\n";
							Nfaces += 1;
						}
					}
					j += 1;
				}
			}

			if (t != 4 && t != 5) cout << "PLY write: bad type " << t << endl;
		}

		return data;
	};

	string header =
	"ply\n"
	"format ascii 1.0\n"
	"comment Created by PolyVR - https://github.com/Victor-Haefner/polyvr\n"
	"element vertex "+toString(Np)+"\n"
	"property float x\n"
	"property float y\n"
	"property float z\n";
    if (Nn == Np) header += "property float nx\n";
    if (Nn == Np) header += "property float ny\n";
    if (Nn == Np) header += "property float nz\n";
    if (Nc == Np) header += "property uchar red\n";
    if (Nc == Np) header += "property uchar green\n";
    if (Nc == Np) header += "property uchar blue\n";
    if (Nt == Np) header += "property float s\n";
    if (Nt == Np) header += "property float t\n";
    header += "element face "+toString(Nfaces)+"\n"
    "property list uchar uint vertex_indices\n"
    "end_header\n";

	cout << "PLY export " << geo->getName() << endl;

	auto vertsData = writeVertices();
	auto indsData = writeIndices();


    fstream f;
    f.open (path, fstream::out | fstream::app);
	f << header;
	f << vertsData;
	f << indsData;
	f.close();
}

OSG_END_NAMESPACE;
