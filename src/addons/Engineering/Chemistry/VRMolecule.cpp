#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

map<string, PeriodicTableEntry> PeriodicTable;
map<string, vector<Matrix> > AtomicStructures;

//#include <OpenSG/OSGMatrixUtility.h>
void MatrixLookAt( Matrix& m, Vec3f p, Vec3f a, Vec3f u) {
    Vec3f d = p - a;
    Vec3f r = u.cross(d);
    m[0] = Vec4f(r[0], r[1], r[2], 0);
    m[1] = Vec4f(u[0], u[1], u[2], 0);
    m[2] = Vec4f(d[0], d[1], d[2], 0);
    m[3] = Vec4f(p[0], p[1], p[2], 1);
}

void initAtomicTables() {
	PeriodicTable["H"] = PeriodicTableEntry(1, Vec3f(1,1,1));
	PeriodicTable["C"] = PeriodicTableEntry(4, Vec3f(0.4,0.4,0.4));
	PeriodicTable["N"] = PeriodicTableEntry(5, Vec3f(0,0,1));
	PeriodicTable["O"] = PeriodicTableEntry(6, Vec3f(1,0,0));
	PeriodicTable["S"] = PeriodicTableEntry(5, Vec3f(1,1,0));
	PeriodicTable["P"] = PeriodicTableEntry(6, Vec3f(0.8,0.5,0));
	PeriodicTable["Cl"] = PeriodicTableEntry(7, Vec3f(0,1,0));

	AtomicStructures["tetra"] = vector<Matrix>();
	AtomicStructures["iso"] = vector<Matrix>();
	AtomicStructures["linear"] = vector<Matrix>();
	AtomicStructures["single"] = vector<Matrix>();
	AtomicStructures["invalid"] = vector<Matrix>();

    Matrix m;
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );
	MatrixLookAt( m, Vec3f(0,0,1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );

    float s2 = sqrt(2);
    float s3 = sqrt(3);
    float _3 = 1.0/3.0;

	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,-sqrt(8.0/9),_3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,sqrt(8.0/9),_3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );

	Vec3f tP0 = Vec3f(0, 0, -1);
	Vec3f tP1 = Vec3f(0, 2*s2/3, _3);
	Vec3f tP2 = Vec3f(sqrt(2.0/3), -s2/3, _3);
	Vec3f tP3 = Vec3f(-sqrt(2.0/3), -s2/3, _3);

	Vec3f tU0 = Vec3f(0, -1, 0);
	Vec3f tU1 = Vec3f(0, -_3, 2*s2/3);
	Vec3f tU2 = Vec3f(-s3/6, 1.0/6, 2*s2/3);
	Vec3f tU3 = Vec3f(s3/6, 1.0/6, 2*s2/3);

	m.setIdentity(); MatrixLookAt( m, tP0, Vec3f(0,0,0), tU0) ; AtomicStructures["tetra"].push_back( m );
	m.setIdentity(); MatrixLookAt( m, tP1, Vec3f(0,0,0), tU1) ; AtomicStructures["tetra"].push_back( m );
	m.setIdentity(); MatrixLookAt( m, tP2, Vec3f(0,0,0), tU2); AtomicStructures["tetra"].push_back( m );
	m.setIdentity(); MatrixLookAt( m, tP3, Vec3f(0,0,0), tU3); AtomicStructures["tetra"].push_back( m );
}

PeriodicTableEntry::PeriodicTableEntry() {}
PeriodicTableEntry::PeriodicTableEntry(int valence_electrons, Vec3f color) {
    this->valence_electrons = valence_electrons;
    this->color = color;
}

VRAtom::VRAtom(string type, int ID) {
    this->ID = ID;
    this->type = type;
    params = PeriodicTable[type];
}

PeriodicTableEntry VRAtom::getParams() { return params; }
Matrix VRAtom::getTransformation() { return transformation; }
vector<VRAtom*> VRAtom::getBonds() { return bonds; }
int VRAtom::getID() { return ID; }

void VRAtom::computeGeo() {
    if (bonds.size() > 4) geo = "invalid";
    if (bonds.size() == 4) geo = "tetra";
    if (bonds.size() == 3) geo = "iso";
    if (bonds.size() == 2) geo = "linear";
    if (bonds.size() == 1) geo = "single";
    if (bonds.size() == 0) geo = "none";
}

void VRAtom::computePositions() {
    string g = geo;
    if (AtomicStructures.count(geo) == 0) { cout << "Error: " << geo << " is invalid!\n"; return; }

    vector<Matrix> structure = AtomicStructures[geo];
    for (uint i=0; i<bonds.size(); i++) {
        VRAtom* b = bonds[i];
        if (b->ID <= ID) continue;
        if (i >= structure.size()) break;
        b->transformation = transformation;
        b->transformation.mult(structure[i]);
    }
}

bool VRAtom::append(VRAtom* at) {
    if (full or at->full or at == this) return false;
    for (auto b : bonds) if (b == at) return false;

    uint bmax = 4 - abs(params.valence_electrons - 4);
    bonds.push_back(at);
    at->append(this);

    if (bonds.size() == bmax) full = true;
    return true;
}

void VRAtom::print() {
    cout << " " << ID << " " << type << " " << geo;
    for (auto b : bonds) cout << " " << b->ID;
    cout << endl;
}



VRMolecule::VRMolecule(string definition) : VRGeometry(definition) {
    if (PeriodicTable.size() == 0) initAtomicTables();

    bonds_geo = new VRGeometry("bonds");
    addChild(bonds_geo);

    vector<string> mol = parse(definition);
    for (auto a : mol) addAtom(a);
    for (auto a : atoms) a->computeGeo();
    //for (auto a : atoms) a->print();
    for (auto a : atoms) a->computePositions();
    updateGeo();
}

void VRMolecule::addAtom(string a) {
    VRAtom* at = new VRAtom(a, atoms.size());
    for (auto a : atoms) if (a->append(at)) break;
    atoms.push_back(at);
}

void VRMolecule::updateGeo() {
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRefPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRefPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRefPtr      cols = GeoVec3fProperty::create();

    for (auto a : atoms) {
        cols->addValue(a->getParams().color);
        Pos->addValue(a->getTransformation()[3]);
        Norms->addValue(a->getTransformation()[1]);
        cout << " updateGeo " << a->getID() << " " << a->getTransformation()[3] << endl;
    }

    for (uint i=0; i<Pos->size(); i++) {
        Indices->addValue(i);
    }

    GeoUInt32PropertyRefPtr     Indices2 = GeoUInt32Property::create();
    for (auto a : atoms) {
        for (auto b : a->getBonds()) {
            if (b->getID() > a->getID()) {
                Indices2->addValue(a->getID());
                Indices2->addValue(b->getID());
            }
        }
    }

    VRMaterial* mat = VRMaterial::get("atoms");
    mat->setPointSize(40);
    mat->setLit(false);

    VRMaterial* mat2 = VRMaterial::get("molecule_bonds");
    mat2->setLineWidth(5);
    mat2->setLit(false);

    setType(GL_POINTS);
    setPositions(Pos);
    setNormals(Norms);
    setColors(cols);
    setIndices(Indices);
    setMaterial(mat);

    bonds_geo->setType(GL_LINES);
    bonds_geo->setPositions(Pos);
    bonds_geo->setNormals(Norms);
    bonds_geo->setColors(cols);
    bonds_geo->setIndices(Indices2);
    bonds_geo->setMaterial(mat2);




    /*VRMaterial* mat3 = VRMaterial::get("normals");
    mat3->setLineWidth(5);
    mat3->setLit(false);

    GeoPnt3fPropertyRecPtr      Pos2 = GeoPnt3fProperty::create();
    GeoUInt32PropertyRefPtr     Indices3 = GeoUInt32Property::create();
    GeoVec3fPropertyRefPtr      cols2 = GeoVec3fProperty::create();
    GeoVec3fPropertyRefPtr      norms2 = GeoVec3fProperty::create();


    int i=0;
    for (auto a : atoms) {
        cols2->addValue(Vec3f(0,1,1));
        cols2->addValue(Vec3f(0,1,1));
        norms2->addValue(Vec3f(0,1,0));
        norms2->addValue(Vec3f(0,1,0));
        Pos2->addValue(a->getTransformation()[3]);
        Pos2->addValue(a->getTransformation()[3] + a->getTransformation()[1]);
        Indices3->addValue(i);
        Indices3->addValue(i+1);
        i +=2;
        cout << " normal " << a->getID() << " " << a->getTransformation()[1] << endl;
    }

    VRGeometry* normals = new VRGeometry("normals");
    addChild(normals);
    bonds_geo->setType(GL_LINES);
    bonds_geo->setPositions(Pos2);
    bonds_geo->setNormals(norms2);
    bonds_geo->setColors(cols2);
    bonds_geo->setIndices(Indices3);
    bonds_geo->setMaterial(mat3);*/
}

vector<string> VRMolecule::parse(string mol) {
    mol += "%  "; // add an ending flag
    vector<string> m;
    uint i = 0;

    while (i < mol.size() ) {
        if (mol[i] == '%') return m; // check for ending flag

        int j = 1;
        string atom = mol.substr(i, 2);
        if (PeriodicTable.count(atom)) j = 2; // search first for double atom names like Cl

        atom = mol.substr(i, j);

        string X = "";
        for (int k=0; mol[i+j+k] >= '0' and mol[i+j+k] <= '9'; k++) {
            X += mol[i+j+k];
        }

        int N = 1;
        if (X.size() > 0) N = toInt(X);

        if (PeriodicTable.count(atom)) {
            for (int k=0; k<N; k++) m.push_back(atom);
        }

        i += j;
    }


    return m;
}
