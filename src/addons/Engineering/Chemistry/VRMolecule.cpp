#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "addons/Engineering/VRNumberingEngine.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGShaderVariableOSG.h>
#include <OpenSG/OSGQuaternion.h>

#define GLSL(shader) #shader

using namespace OSG;

map<string, PeriodicTableEntry> PeriodicTable;
map<string, vector<Matrix> > AtomicStructures;

#include <OpenSG/OSGMatrixUtility.h>

void initAtomicTables() { // TODO: set colors
	PeriodicTable["H"] = PeriodicTableEntry(1, 0.37, Vec3f(1,1,1));
	PeriodicTable["He"] = PeriodicTableEntry(8, 0.5, Vec3f(1,0,1));

	PeriodicTable["Li"] = PeriodicTableEntry(1, 1.52, Vec3f(0.6,0.4,0.0));
	PeriodicTable["Be"] = PeriodicTableEntry(2, 1.11, Vec3f(0.6,0.0,0.6));
	PeriodicTable["B"] = PeriodicTableEntry(3, 0.88, Vec3f(0.4,1.0,0.4));
	PeriodicTable["C"] = PeriodicTableEntry(4, 0.77, Vec3f(0.4,0.4,0.4));
	PeriodicTable["N"] = PeriodicTableEntry(5, 0.7, Vec3f(0,0,1));
	PeriodicTable["O"] = PeriodicTableEntry(6, 0.66, Vec3f(1,0,0));
	PeriodicTable["F"] = PeriodicTableEntry(7, 0.64, Vec3f(0,0,1));
	PeriodicTable["Ne"] = PeriodicTableEntry(8, 0.70, Vec3f(0,0,1));

	PeriodicTable["Na"] = PeriodicTableEntry(1, 1.86, Vec3f(0,0,1));
	PeriodicTable["Mg"] = PeriodicTableEntry(2, 1.60, Vec3f(0,1,0));
	PeriodicTable["Al"] = PeriodicTableEntry(3, 1.43, Vec3f(0.6,0.6,0.6));
	PeriodicTable["Si"] = PeriodicTableEntry(4, 1.17, Vec3f(0.5,1,0));
	PeriodicTable["P"] = PeriodicTableEntry(5, 1.1, Vec3f(0.8,0.5,0));
	PeriodicTable["S"] = PeriodicTableEntry(6, 1.04, Vec3f(1,1,0));
	PeriodicTable["Cl"] = PeriodicTableEntry(7, 0.99, Vec3f(0.3,1,0));
	PeriodicTable["Ar"] = PeriodicTableEntry(8, 0.94, Vec3f(0,1,0.5));

	AtomicStructures["tetra"] = vector<Matrix>();
	AtomicStructures["iso"] = vector<Matrix>();
	AtomicStructures["linear"] = vector<Matrix>();
	AtomicStructures["single"] = vector<Matrix>();
	AtomicStructures["none"] = vector<Matrix>();
	AtomicStructures["invalid"] = vector<Matrix>();

    float s2 = sqrt(2);
    float s3 = sqrt(3);
    float _3 = 1.0/3.0;
    Matrix m;

    // linear structure
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );
	MatrixLookAt( m, Vec3f(0,0,1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );

	AtomicStructures["single"] = AtomicStructures["linear"];

    // planar structure
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0, s3/2, 0.5), Vec3f(0,0,0), Vec3f(0, 0.5, s3/2) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0, -s3/2, 0.5), Vec3f(0,0,0), Vec3f(0, 0.5, -s3/2) ); AtomicStructures["iso"].push_back( m );

    // tetraeder structure
	Vec3f tP0 = Vec3f(0, 0, -1);
	Vec3f tP1 = Vec3f(sqrt(2.0/3), -s2/3, _3);
	Vec3f tP2 = Vec3f(-sqrt(2.0/3), -s2/3, _3);
	Vec3f tP3 = Vec3f(0, 2*s2/3, _3);

	Vec3f tU0 = Vec3f(0, -1, 0);
	Vec3f tU1 = Vec3f(-s3/6, 1.0/6, 2*s2/3);
	Vec3f tU2 = Vec3f(s3/6, 1.0/6, 2*s2/3);
	Vec3f tU3 = Vec3f(0, -_3, 2*s2/3);

	MatrixLookAt( m, tP0, Vec3f(0,0,0), tU0); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP1, Vec3f(0,0,0), tU1); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP2, Vec3f(0,0,0), tU2); AtomicStructures["tetra"].push_back( m );
	MatrixLookAt( m, tP3, Vec3f(0,0,0), tU3); AtomicStructures["tetra"].push_back( m );
}

PeriodicTableEntry::PeriodicTableEntry() {}
PeriodicTableEntry::PeriodicTableEntry(int valence_electrons, float radius, Vec3f color) {
    this->valence_electrons = valence_electrons;
    this->color = color;
    this->radius = radius;
}

VRBond::VRBond(int t, int s, VRAtom* a2, VRAtom* a1) { type = t; atom1 = a1; atom2 = a2; slot = s; }
VRBond::VRBond() {}

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
Matrix VRAtom::getTransformation() { return transformation; }
void VRAtom::setTransformation(Matrix m) { transformation = m; }
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

    vector<Matrix> structure = AtomicStructures[geo];
    for (auto& b : bonds) {
        if (b.first >= (int)structure.size()) break;
        if (b.second.extra) continue;

        Matrix T = transformation;
        Matrix S = structure[b.first];

        VRAtom* a = b.second.atom2;
        if (a == 0) { // duplets
            float r = 0.5*b.second.atom1->getParams().radius;
            S[3] *= r; S[3][3] = 1;
            T.mult(S);
            T.mult(Pnt3f(1,0,0)*r, b.second.p1);
            T.mult(Pnt3f(-1,-0,0)*r, b.second.p2);
            continue;
        }

        if (a->ID <= ID) continue;
        T.mult(S);
        a->transformation = T;
    }
}

bool VRAtom::append(VRAtom* at, int bType, bool extra) {
    VRBond bond;
    bond.type = bType;
    bond.extra = extra;
    bond.atom2 = at;
    if (full || at->full || at == this) return false;
    for (auto b : bonds) if (b.second.atom2 == at) return false;
    for (auto b : at->bonds) if (b.second.atom2 == this) return false;

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
    cout << " ID: " << ID << " Type: " << type  << " full?: " << full << " boundEl: " << bound_valence_electrons << " geo: " << geo << " pos: " << Vec3f(transformation[3]);
    cout << " bonds with: ";
    for (auto b : bonds) {
        if (b.second.atom2 == 0) cout << " " << "pair";
        else cout << " " << b.second.atom2->ID << "(" << b.first << "," << b.second.slot << ")";
    }
    cout << endl;
}

void VRAtom::propagateTransformation(Matrix& T, uint flag, bool self) {
    if (flag == recFlag) return;
    recFlag = flag;

    if (self) {
        Matrix m = T;
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


VRMolecule::VRMolecule(string definition) : VRGeometry(definition) {
    bonds_geo = VRGeometry::create("bonds");
    coords_geo = VRGeometry::create("coords");

    labels = VRNumberingEngine::create();
    labels->setBillboard(true);
    labels->setOnTop(false);
    labels->setSize(0.1);

    set(definition);
}

VRMoleculePtr VRMolecule::create(string definition) {
    auto ptr = VRMoleculePtr(new VRMolecule(definition) );
    ptr->addChild(ptr->bonds_geo);
    ptr->addChild(ptr->coords_geo);
    ptr->addChild(ptr->labels);
    return ptr;
}

VRMoleculePtr VRMolecule::ptr() { return static_pointer_cast<VRMolecule>( shared_from_this() ); }

void VRMolecule::addAtom(string a, int t) {
    VRAtom* at = new VRAtom(a, getID());
    atoms[at->getID()] = at;
    addAtom(at, t);
}

void VRMolecule::addAtom(VRAtom* b, int bType, bool extra) {
    for (auto a : atoms) if (a.second->append(b, bType, extra)) break;
}

void VRMolecule::addAtom(int ID, int t) {
    if (atoms.count(ID) == 0) return;
    VRAtom* at = atoms[ID];
    if (at->full) return;
    addAtom(at, t, true);
}

void VRMolecule::remAtom(int ID) {
    if (atoms.count(ID) == 0) return;
    VRAtom* a = atoms[ID];
    atoms.erase(ID);
    delete a;
}

void VRMolecule::updateGeo() {
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRecPtr      cols = GeoVec3fProperty::create();

    GeoPnt3fPropertyRecPtr      Pos2 = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms2 = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices2 = GeoUInt32Property::create();

    float r_scale = 0.6;

    int i=0;
    int j=0;
    for (auto a : atoms) {
        PeriodicTableEntry aP = a.second->getParams();
        cols->addValue(aP.color);
        Pos->addValue(a.second->getTransformation()[3]);
        Norms->addValue( Vec3f(0, r_scale*aP.radius, 0) );
        Indices->addValue(i++);

        // bonds
        for (auto b : a.second->getBonds()) {
            if (b.second.atom2 == 0) { // duplet
                Pos2->addValue(b.second.p1);
                Pos2->addValue(b.second.p2);
                Norms2->addValue( Vec3f(0, 1, 0) );
                Norms2->addValue( Vec3f(0.1*b.second.type, 1,1) );
                Indices2->addValue(j++);
                Indices2->addValue(j++);
                continue;
            }

            if (b.second.atom2->getID() < a.first) {
                PeriodicTableEntry bP = b.second.atom2->getParams();
                Pos2->addValue(a.second->getTransformation()[3]);
                Pos2->addValue(b.second.atom2->getTransformation()[3]);
                Norms2->addValue( Vec3f(0, 1, 0) );
                Norms2->addValue( Vec3f(0.1*b.second.type, r_scale*aP.radius, r_scale*bP.radius) );
                Indices2->addValue(j++);
                Indices2->addValue(j++);
            }
        }
    }

    // atoms geometry
    VRMaterialPtr mat = VRMaterial::get("atoms");
    mat->setPointSize(40);
    mat->setLit(false);
    mat->setVertexShader(a_vp, "moleculesVS");
    mat->setFragmentShader(a_fp, "moleculesFS");
    mat->setGeometryShader(a_gp, "moleculesGS");

    setType(GL_POINTS);
    setPositions(Pos);
    setNormals(Norms);
    setColors(cols);
    setIndices(Indices);
    setMaterial(mat);

    // bonds geometry
    VRMaterialPtr mat2 = VRMaterial::get("molecule_bonds");
    mat2->setLineWidth(5);
    mat2->setLit(false);
    mat2->setVertexShader(b_vp, "moleculeBondsVS");
    mat2->setFragmentShader(b_fp, "moleculeBondsFS");
    mat2->setGeometryShader(b_gp, "moleculeBondsGS");

    bonds_geo->setType(GL_LINES);
    bonds_geo->setPositions(Pos2);
    bonds_geo->setNormals(Norms2);
    bonds_geo->setColors(cols);
    bonds_geo->setIndices(Indices2);
    bonds_geo->setMaterial(mat2);

    updateLabels();
    updateCoords();
}

bool isNumber(char c) { return (c >= '0' && c <= '9'); }

string parseNumber(string in, int offset) {
    string X = ""; //parse number
    for (int k=0; isNumber(in[offset+k]); k++) X += in[offset+k];
    return X;
}

vector<string> VRMolecule::parse(string mol, bool verbose) {
    mol += "%  "; // add an ending flag
    vector<string> res;
    uint i = 0;
    string X;

    if (verbose) cout << "parse " << mol << endl;
    while (i < mol.size() ) {
        if (verbose) cout << "  c: " << mol[i];
        if (mol[i] == '%') break; // check for ending flag

        // check for double || triple bounds
        string bond = "1"; // single
        if (mol[i] == '-') bond = "2"; // double
        if (mol[i] == '=') bond = "3"; // triple
        if (mol[i] == '-' || mol[i] == '=') i++;

        // check for bond with ID atom2
        X = parseNumber(mol, i); //parse number
        if (X.size() > 0) {
            res.push_back(bond);
            res.push_back(X);
            i += X.size();
            if (verbose) cout << " ID: " << X;
            continue;
        }

        int j = 1;
        string atom2 = mol.substr(i, 2);
        if (PeriodicTable.count(atom2)) j = 2; // search first for double atom2 names like Cl
        atom2 = mol.substr(i, j); // final atom2 type string

        X = parseNumber(mol, i+j); //parse number
        if (X.size() > 0 && verbose) cout << " N: " << X;
        j += X.size();

        int N = 1;
        if (X.size() > 0) N = toInt(X);

        if (PeriodicTable.count(atom2)) {
            for (int k=0; k<N; k++) {
                res.push_back(bond);
                res.push_back(atom2);
            }
        }

        i += j;
    }

    if (verbose) cout << endl;


    return res;
}

void VRMolecule::set(string definition) {
    if (PeriodicTable.size() == 0) initAtomicTables();
    this->definition = definition;

    vector<string> mol = parse(definition, false);
    atoms.clear();

    for (uint i=0; i<mol.size(); i+=2) {
        string a = mol[i+1];
        int b = toInt(mol[i]);
        if (isNumber(a[0])) addAtom(toInt(a), b);
        else addAtom(a,b);
    }

    for (auto a : atoms) a.second->computePositions();
    //for (auto a : atoms) a->print();

    updateGeo();
}

void VRMolecule::setRandom(int N) {
    string m;
    int a = 0;

    vector<string> types;
    for(auto a : PeriodicTable) types.push_back(a.first);
    int aN = types.size();

    for (int i=0; i<N; i++) {
        int bt = rand()%20;
        if (bt == 0) m += '=';
        else if (bt < 4) m += '-';

		a = rand() % aN;
        m += types[a];
		m += toString(int(1 + rand() % 4));
    }

    set(m);
}

string VRMolecule::getDefinition() { return definition; }

int VRMolecule::getID() {
    int i=0;
    while (atoms.count(i)) i++;
    return i;
}

uint VRMolecule::getFlag() {
	return VRGlobals::CURRENT_FRAME + rand();
}

void VRMolecule::rotateBond(int a, int b, float f) {
    if (atoms.count(a) == 0) return;
    if (atoms.count(b) == 0) return;
    VRAtom* A = atoms[a];
    VRAtom* B = atoms[b];

	uint now = VRGlobals::CURRENT_FRAME + rand();
    A->recFlag = now;

    Vec3f p1 = Vec3f( A->getTransformation()[3] );
    Vec3f p2 = Vec3f( B->getTransformation()[3] );
    Vec3f dir = p2-p1;
    Quaternion q(dir, f);
    Matrix R;
    R.setRotate(q);

    Matrix T;
    T[3] = B->getTransformation()[3];
    Matrix _T;
    T.inverse(_T);
    T.mult(R);
    T.mult(_T);

    //cout << "ROTATE bound " << a << "-" << b << " around " << dir << " with " << f << endl;

    B->propagateTransformation(T, now);
    updateGeo();
}

void VRMolecule::changeBond(int a, int b, int t) {
    if (atoms.count(a) == 0) return;
    if (atoms.count(b) == 0) return;
    VRAtom* A = atoms[a];
    VRAtom* B = atoms[b];
    if (A == 0 || B == 0) return;

    A->detach(B);
    A->append(B, t);
    updateGeo();
}

void VRMolecule::substitute(int a, VRMoleculePtr m, int b) {
    if (atoms.count(a) == 0) return;
    if (m->atoms.count(b) == 0) return;

    Matrix am = atoms[a]->getTransformation();
    Matrix bm = m->atoms[b]->getTransformation();

    map<int, VRBond> bondsA = atoms[a]->getBonds();
    map<int, VRBond> bondsB = m->atoms[b]->getBonds();
    if (bondsA.count(0) == 0) return;
    if (bondsB.count(0) == 0) return;

    VRAtom* A = bondsA[0].atom2;
    VRAtom* B = bondsB[0].atom2;
    int Ai = A->getID();
    int Bi = B->getID();
    remAtom(a);
    m->remAtom(b);

    if (atoms.count(Ai) == 0) { cout << "AA\n"; return; }
    if (m->atoms.count(Bi) == 0) { cout << "BB\n"; return; }

    // copy atoms
    for (auto at : m->atoms) {
        int ID = getID();
        at.second->setID(ID);
        atoms[ID] = at.second;
    }
    m->set(m->getDefinition());

    // attach molecules
    A->append(B, 1, true);

    // transform new atoms
	uint now = VRGlobals::CURRENT_FRAME + rand();
    A->recFlag = now;
    bm.invert();
    Matrix Bm = B->getTransformation();
    bm.mult(Bm);
    bm.setTranslate(Vec3f(0,0,0));
    am.mult(bm);
    MatrixLookAt( bm, Vec3f(0,0,0), Vec3f(0,0,1), Vec3f(0,-1,0) );
    bm.mult(am);
    bm[3] = am[3];
    B->propagateTransformation(bm, now);

    updateGeo();
}

void VRMolecule::setLocalOrigin(int ID) {
    if (atoms.count(ID) == 0) return;

	uint now = VRGlobals::CURRENT_FRAME + rand();
    Matrix m = atoms[ID]->getTransformation();
    m.invert();

    Matrix im;
    MatrixLookAt( im, Vec3f(0,0,0), Vec3f(0,0,1), Vec3f(0,1,0) );
    im.mult(m);

    atoms[ID]->propagateTransformation(im, now);
}

void VRMolecule::attachMolecule(int a, VRMoleculePtr m, int b) {
    if (atoms.count(a) == 0) return;
    if (m->atoms.count(b) == 0) return;

    /*if (time > 0.00001) { // just an idea
        path* p = new path();
        m->startPathAnimation(path* p, time, offset, bool redirect = true);
        return;
    }*/

    VRAtom* A = atoms[a];
    VRAtom* B = m->atoms[b];
    m->setLocalOrigin(b);

    for (auto at : m->atoms) { // copy atoms
        int ID = getID();
        at.second->setID(ID);
        atoms[ID] = at.second;
    }
    m->set(m->getDefinition());

    A->append(B, 1); // attach molecules
    A->computePositions();

    // transform new atoms
    uint now = getFlag();
    A->recFlag = now;
    Matrix bm = B->getTransformation();
    B->propagateTransformation(bm, now, false);

    updateGeo();
}

void VRMolecule::showLabels(bool b) { if (doLabels == b) return; doLabels = b; updateLabels(); }
void VRMolecule::showCoords(bool b) { if (doCoords == b) return; doCoords = b; updateCoords(); }

void VRMolecule::updateCoords() {
    coords_geo->hide();
    if (!doCoords) return;

    coords_geo->show();

    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRecPtr      cols = GeoVec3fProperty::create();

    int i=0;
    for (auto a : atoms) {
        float s = 0.4;
        Vec4f p0 = a.second->getTransformation()[3];
        Pos->addValue( p0 );
        Pos->addValue( p0 + s*a.second->getTransformation()[0] );
        Pos->addValue( p0 + s*a.second->getTransformation()[1] );
        Pos->addValue( p0 + s*a.second->getTransformation()[2] );
        cols->addValue(Vec3f(0,0,0));
        cols->addValue(Vec3f(1,0,0));
        cols->addValue(Vec3f(0,1,0));
        cols->addValue(Vec3f(0,0,1));
        Norms->addValue( Vec3f(0, 1, 0) );
        Norms->addValue( Vec3f(0, 1, 0) );
        Norms->addValue( Vec3f(0, 1, 0) );
        Norms->addValue( Vec3f(0, 1, 0) );
        Indices->addValue(i+0);
        Indices->addValue(i+1);
        Indices->addValue(i+0);
        Indices->addValue(i+2);
        Indices->addValue(i+0);
        Indices->addValue(i+3);
        i+=4;
    }

    // atoms geometry
    VRMaterialPtr mat = VRMaterial::get("coords");
    mat->setLineWidth(2);
    mat->setLit(false);

    coords_geo->setType(GL_LINES);
    coords_geo->setPositions(Pos);
    coords_geo->setNormals(Norms);
    coords_geo->setColors(cols);
    coords_geo->setIndices(Indices);
    coords_geo->setMaterial(mat);
}

void VRMolecule::updateLabels() {
    labels->clear();
    if (!doLabels) return;

    labels->add(Vec3f(), atoms.size(), 0, 0);

    int i=0;
    for (auto a : atoms) {
        Vec3f p = Vec3f(a.second->getTransformation()[3]);
        labels->set(i++, p, a.first);
    }
}

VRAtom* VRMolecule::getAtom(int ID) {
    if (atoms.count(ID) != 0) return atoms[ID];
    return 0;
}

string VRMolecule::a_fp =
"#version 120\n"
GLSL(
in vec2 texCoord;
in vec4 Color;

void main( void ) {
	vec2 p = 2* ( vec2(0.5, 0.5) - texCoord );
	float r = sqrt(dot(p,p));
	if (r > 1.0) discard;

	float f = 1.2 - (1.0-sqrt(1.0-r))/(r);
	vec4 amb = vec4(0.2);
	gl_FragColor = Color*f + amb;
}
);

string VRMolecule::a_vp =
"#version 120\n"
GLSL(
varying vec4 color;
varying vec3 normal;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;

void main( void ) {
    color = osg_Color;
    normal = osg_Normal.xyz;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);

string VRMolecule::a_gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in vec4 color[];
in vec3 normal[];
out vec2 texCoord;
out vec4 Color;

void emitVertex(in vec4 p, in vec2 tc) {
	gl_Position = p;
	texCoord = tc;
	EmitVertex();
}

void emitQuad(in float s, in vec4 tc) {
	vec4 p = gl_PositionIn[0];

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 u = vec4(s*a,0,0,0);
	vec4 v = vec4(0,s,0,0);

	vec4 p1 = p -u -v;
	vec4 p2 = p -u +v;
	vec4 p3 = p +u +v;
	vec4 p4 = p +u -v;

	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p2, vec2(tc[0], tc[3]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	EndPrimitive();
	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	emitVertex(p4, vec2(tc[1], tc[2]));
	EndPrimitive();
}

void main() {
	Color = color[0];
	emitQuad(normal[0][1], vec4(0,1,0,1));
}
);

string VRMolecule::b_fp =
"#version 120\n"
GLSL(
in vec2 texCoord;
in vec4 Color;

void main( void ) {
	float r = texCoord.y;
	r = 2.8*abs(r - floor(r) - 0.5);
	if (r > 1.0) discard;

	float f = 1.2 - (1.0-sqrt(1.0-r))/(r);
	vec4 amb = vec4(0.2);
	gl_FragColor = Color*f + amb;
}
);

string VRMolecule::b_vp =
"#version 120\n"
GLSL(
varying vec3 normal;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;

void main( void ) {
    normal = osg_Normal.xyz;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);

string VRMolecule::b_gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (lines) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in vec3 normal[];
out vec2 texCoord;
out vec4 Color;

void emitVertex(in vec4 p, in vec2 tc) {
	gl_Position = p;
	texCoord = tc;
	EmitVertex();
}

void emitQuad(in float s, in float f, in vec4 tc) {
	vec4 pl1 = gl_PositionIn[0];
	vec4 pl2 = gl_PositionIn[1];

	vec3 n2 = normal[1];

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 d = pl2-pl1;
	pl1 += 0.7*n2.y*d;
	pl2 -= 0.7*n2.z*d;

	vec3 x = cross(d.xyz, vec3(0,0,1));
	x.x *= a;
	x = f*normalize(x);
	x.x *= a;
	vec4 v = vec4(x,0);

	vec4 p1 = pl1 -v;
	vec4 p2 = pl1 +v;
	vec4 p3 = pl2 +v;
	vec4 p4 = pl2 -v;

	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p2, vec2(tc[0], tc[3]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	EndPrimitive();
	emitVertex(p1, vec2(tc[0], tc[2]));
	emitVertex(p3, vec2(tc[1], tc[3]));
	emitVertex(p4, vec2(tc[1], tc[2]));
	EndPrimitive();
}

void main() {
	vec3 n1 = normal[0];
	vec3 n2 = normal[1];
	float b = n2.x;

	Color = vec4(0.5, 0.5, 0.5, 1.0);
	vec4 tc = vec4(0,1,0,1);
	float w = 0.09;
	float k = 1.0;

	if (b > 0.0 && b < 0.15) Color.y += 0.5; /* single */

	if (b > 0.15 && b < 0.25) { /* double */
		Color.x += 0.5;
		Color.y += 0.5;
		w = 1.5*w;
		k = 2.0;
	}

	if (b > 0.25 && b < 0.35) { /* triple */
		Color.x += 0.5;
		w = 2*w;
		k = 3.0;
	}

	if (b > 0.35 && b < 0.45) { /* el pair */
		Color.x += 0.5;
		Color.y += 0.3;
		w = 2*w;
		k = 1.0;
	}

	emitQuad(0.2, w, vec4(0.0, k, 0.0, k));
}
);
