#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"

#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

map<string, PeriodicTableEntry> PeriodicTable;
map<string, vector<Matrix> > AtomicStructures;

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

    float _3 = 1.0/3.0;
    float l2 = sqrt(2);
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,-sqrt(8.0/9),_3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,sqrt(8.0/9),_3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );

	MatrixLookAt( m, Vec3f(0, 0, -1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, Vec3f(0, sqrt(8.0/9), _3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, Vec3f(sqrt(7.0/9), -_3, _3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, Vec3f(-sqrt(7.0/9), -_3, _3), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["tetra"].push_back( m );
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
    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRefPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRefPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRefPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRefPtr      cols = GeoVec3fProperty::create();

    Type->addValue(GL_POINTS);
    Type->addValue(GL_LINES);

    for (auto a : atoms) {
        cols->addValue(a->getParams().color);
        Pos->addValue(a->getTransformation()[3]);
    }

    for (uint i=0; i<Pos->size(); i++) {
        Norms->addValue(Vec3f(0,1,0));
        Indices->addValue(i);
    }

    int bN = 0;
    for (auto a : atoms) {
        for (auto b : a->getBonds()) {
            if (b->getID() > a->getID()) {
                Indices->addValue(a->getID());
                Indices->addValue(b->getID());
                bN++;
            }
        }
    }

    Length->addValue(Pos->size());
    Length->addValue(2*bN);


    VRMaterial* mat = VRMaterial::get("molecules");
    mat->setPointSize(40);
    mat->setLineWidth(5);
    mat->setLit(false);

    setTypes(Type);
    setPositions(Pos);
    setNormals(Norms);
    setIndices(Indices);
    setLengths(Length);
    setMaterial(mat);
    setColors(cols);
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
