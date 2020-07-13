#include "VRAtom.h"
#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRTransform.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "addons/Engineering/VRNumberingEngine.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGQuaternion.h>

using namespace OSG;

map<string, PeriodicTableEntry> VRAtom::PeriodicTable = map<string, PeriodicTableEntry>();
map<string, vector<Matrix4d> > VRAtom::AtomicStructures = map<string, vector<Matrix4d> >();

void VRAtom::initAtomicTables() { // TODO: set colors
	PeriodicTable["H"] = PeriodicTableEntry(1, 0.37, Color3f(1,1,1));
	PeriodicTable["He"] = PeriodicTableEntry(8, 0.5, Color3f(1,0,1));

	PeriodicTable["Li"] = PeriodicTableEntry(1, 1.52, Color3f(0.6,0.4,0.0));
	PeriodicTable["Be"] = PeriodicTableEntry(2, 1.11, Color3f(0.6,0.0,0.6));
	PeriodicTable["B"] = PeriodicTableEntry(3, 0.88, Color3f(0.4,1.0,0.4));
	PeriodicTable["C"] = PeriodicTableEntry(4, 0.77, Color3f(0.4,0.4,0.4));
	PeriodicTable["N"] = PeriodicTableEntry(5, 0.7, Color3f(0,0,1));
	PeriodicTable["O"] = PeriodicTableEntry(6, 0.66, Color3f(1,0,0));
	PeriodicTable["F"] = PeriodicTableEntry(7, 0.64, Color3f(0,0,1));
	PeriodicTable["Ne"] = PeriodicTableEntry(8, 0.70, Color3f(0,0,1));

	PeriodicTable["Na"] = PeriodicTableEntry(1, 1.86, Color3f(0,0,1));
	PeriodicTable["Mg"] = PeriodicTableEntry(2, 1.60, Color3f(0,1,0));
	PeriodicTable["Al"] = PeriodicTableEntry(3, 1.43, Color3f(0.6,0.6,0.6));
	PeriodicTable["Si"] = PeriodicTableEntry(4, 1.17, Color3f(0.5,1,0));
	PeriodicTable["P"] = PeriodicTableEntry(5, 1.1, Color3f(0.8,0.5,0));
	PeriodicTable["S"] = PeriodicTableEntry(6, 1.04, Color3f(1,1,0));
	PeriodicTable["Cl"] = PeriodicTableEntry(7, 0.99, Color3f(0.3,1,0));
	PeriodicTable["Ar"] = PeriodicTableEntry(8, 0.94, Color3f(0,1,0.5));

	AtomicStructures["tetra"] = vector<Matrix4d>();
	AtomicStructures["iso"] = vector<Matrix4d>();
	AtomicStructures["linear"] = vector<Matrix4d>();
	AtomicStructures["single"] = vector<Matrix4d>();
	AtomicStructures["none"] = vector<Matrix4d>();
	AtomicStructures["invalid"] = vector<Matrix4d>();

    float s2 = sqrt(2);
    float s3 = sqrt(3);
    float _3 = 1.0/3.0;
    Matrix4d m;

    // linear structure
	MatrixLookAt( m, Vec3d(0,0,-1), Vec3d(0,0,0), Vec3d(0,-1,0) ); AtomicStructures["linear"].push_back( m );
	MatrixLookAt( m, Vec3d(0,0,1), Vec3d(0,0,0), Vec3d(0,-1,0) ); AtomicStructures["linear"].push_back( m );

	AtomicStructures["single"] = AtomicStructures["linear"];

    // planar structure
	MatrixLookAt( m, Vec3d(0,0,-1), Vec3d(0,0,0), Vec3d(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3d(0, s3/2, 0.5), Vec3d(0,0,0), Vec3d(0, 0.5, s3/2) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3d(0, -s3/2, 0.5), Vec3d(0,0,0), Vec3d(0, 0.5, -s3/2) ); AtomicStructures["iso"].push_back( m );

    // tetraeder structure
	Vec3d tP0 = Vec3d(0, 0, -1);
	Vec3d tP1 = Vec3d(sqrt(2.0/3), -s2/3, _3);
	Vec3d tP2 = Vec3d(-sqrt(2.0/3), -s2/3, _3);
	Vec3d tP3 = Vec3d(0, 2*s2/3, _3);

	Vec3d tU0 = Vec3d(0, -1, 0);
	Vec3d tU1 = Vec3d(-s3/6, 1.0/6, 2*s2/3);
	Vec3d tU2 = Vec3d(s3/6, 1.0/6, 2*s2/3);
	Vec3d tU3 = Vec3d(0, -_3, 2*s2/3);

	MatrixLookAt( m, tP0, Vec3d(0,0,0), tU0); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP1, Vec3d(0,0,0), tU1); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP2, Vec3d(0,0,0), tU2); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP3, Vec3d(0,0,0), tU3); AtomicStructures["tetra"].push_back( m );
}

PeriodicTableEntry::PeriodicTableEntry() {}
PeriodicTableEntry::PeriodicTableEntry(int valence_electrons, float radius, Color3f color) {
    this->valence_electrons = valence_electrons;
    this->color = color;
    this->radius = radius;
}

VRAtom::VRAtom(string type, int ID) {
    this->ID = ID;
    this->type = type;
    params = PeriodicTable[type];

    // fill in the duplets
    for (int i=4; i<params.valence_electrons; i++) {
        if (i == 4) bonds[2] = VRBond(4,2,0, this);
        if (i == 5) bonds[1] = VRBond(4,1,0, this);
        if (i == 6) bonds[3] = VRBond(4,3,0, this);
        if (i == 7) bonds[0] = VRBond(4,0,0, this);
    }
}

VRAtom::~VRAtom() {
    for (auto b : bonds) {
        VRAtom* a = b.second.atom2;
        a->full = false;
        a->bound_valence_electrons -= b.second.type;
        a->bonds.erase(b.second.slot);
    }
}

PeriodicTableEntry VRAtom::getParams() { return params; }
Matrix4d VRAtom::getTransformation() { return transformation; }
void VRAtom::setTransformation(Matrix4d m) { transformation = m; }
map<int, VRBond>& VRAtom::getBonds() { return bonds; }
int VRAtom::getID() { return ID; }
void VRAtom::setID(int ID) { this->ID = ID; }

void VRAtom::computeGeo() {
    int bN = bonds.size();
    if (bN > 4) geo = "invalid";
    if (bN == 4) geo = "tetra";
    if (bN == 3) geo = "iso";
    if (bN == 2) geo = "linear";
    if (bN == 1) geo = "single";
    if (bN == 0) geo = "none";
}

void VRAtom::computePositions() {
    computeGeo();

    string g = geo;
    if (AtomicStructures.count(geo) == 0) { cout << "Error: " << geo << " is invalid!\n"; return; }

    vector<Matrix4d> structure = AtomicStructures[geo];
    for (auto& b : bonds) {
        if (b.first >= (int)structure.size()) break;
        if (b.second.extra) continue;

        Matrix4d T = transformation;
        Matrix4d S = structure[b.first];

        VRAtom* a = b.second.atom2;
        if (a == 0) { // duplets
            float r = 0.5*b.second.atom1->getParams().radius;
            S[3] *= r; S[3][3] = 1;
            T.mult(S);
            T.mult(Pnt3d(1,0,0)*r, b.second.p1);
            T.mult(Pnt3d(-1,-0,0)*r, b.second.p2);
            continue;
        }

        if (a->ID <= ID) continue;
        T.mult(S);
        a->transformation = T;
    }
}

bool VRAtom::append(VRAtom* at, int bType, bool extra) {
    if (full || at->full || at == this) return false;
    for (auto b : bonds) if (b.second.atom2 == at) return false;
    for (auto b : at->bonds) if (b.second.atom2 == this) return false;

    VRBond bond;
    bond.type = bType;
    bond.extra = extra;
    bond.atom2 = at;

    int bmax1 = 4 - abs(params.valence_electrons - 4);
    int bmax2 = 4 - abs(at->params.valence_electrons - 4);

    int slot1=0;
    int slot2=0;
    for (; bonds.count(slot1) == 1; slot1++);
    for (; at->bonds.count(slot2) == 1; slot2++);

    bond.atom1 = this;
    bond.slot = slot2;
    bonds[slot1] = bond;

    bond.atom2 = this;
    bond.atom1 = at;
    bond.slot = slot1;
    at->bonds[slot2] = bond;

    bound_valence_electrons += bond.type;
    at->bound_valence_electrons += bond.type;

    if (bound_valence_electrons >= bmax1) full = true;
    if (at->bound_valence_electrons >= bmax2) at->full = true;

    //print();
    //cout << " "; at->print();
    return true;
}

void VRAtom::detach(VRAtom* a) {
    for (auto b : bonds) {
        if (b.second.atom2 != a) continue;
        full = false;
        bound_valence_electrons -= b.second.type;
        bonds.erase(b.first);
        break;
    }

    for (auto b : a->bonds) {
        if (b.second.atom2 != this) continue;
        a->full = false;
        a->bound_valence_electrons -= b.second.type;
        a->bonds.erase(b.first);
        break;
    }
}

void VRAtom::print() {
    cout << " ID: " << ID << " Type: " << type  << " full?: " << full << " boundEl: " << bound_valence_electrons << " geo: " << geo << " pos: " << Vec3d(transformation[3]);
    cout << " bonds with: ";
    for (auto b : bonds) {
        if (b.second.atom2 == 0) cout << " " << "pair";
        else cout << " " << b.second.atom2->ID << "(" << b.first << "," << b.second.slot << ")";
    }
    cout << endl;
}

void VRAtom::propagateTransformation(Matrix4d& T, unsigned int flag, bool self) {
    if (flag == recFlag) return;
    recFlag = flag;

    if (self) {
        Matrix4d m = T;
        m.mult(transformation);
        transformation = m;
    }

    for (auto& b : bonds) {
        if (b.second.atom2 == 0) { // duplet
            T.mult(b.second.p1, b.second.p1);
            T.mult(b.second.p2, b.second.p2);
        }
        else b.second.atom2->propagateTransformation(T, flag);
    }
}
