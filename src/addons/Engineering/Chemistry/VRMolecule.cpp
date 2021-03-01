#include "VRAtom.h"
#include "VRMolecule.h"
#include "VRMoleculeMat.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"
#include "core/tools/VRAnnotationEngine.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGQuaternion.h>
#include <OpenSG/OSGMaterial.h>

using namespace OSG;

VRBond::VRBond(int t, int s, VRAtom* a2, VRAtom* a1) { type = t; atom1 = a1; atom2 = a2; slot = s; }
VRBond::VRBond() {}

VRMolecule::VRMolecule(string name) : VRGeometry(name) {
    if (VRAtom::PeriodicTable.size() == 0) VRAtom::initAtomicTables();
    bonds_geo = VRGeometry::create("bonds");
    coords_geo = VRGeometry::create("coords");

    // TODO: not working with syncnodes
    /*labels = VRAnnotationEngine::create();
    labels->setBillboard(true);
    //labels->setOnTop(false);
    labels->setSize(0.1);*/
}

VRMoleculePtr VRMolecule::create(string name) {
    auto ptr = VRMoleculePtr(new VRMolecule(name) );
    if (ptr->bonds_geo) ptr->addChild(ptr->bonds_geo);
    if (ptr->coords_geo) ptr->addChild(ptr->coords_geo);
    if (ptr->labels) ptr->addChild(ptr->labels);
    return ptr;
}

VRMoleculePtr VRMolecule::ptr() { return static_pointer_cast<VRMolecule>( shared_from_this() ); }

int VRMolecule::addAtom(string a) {
    VRAtom* atm = new VRAtom(a, getID());
    atoms[atm->getID()] = atm;
    nonFullAtoms[atm->getID()] = atm;
    return atm->getID();
}

void VRMolecule::connectAtom(VRAtom* b, int bType, bool extra) {
    bool appended = false;

    // first try with nonFullAtoms
    vector<int> filled;
    for (auto a : nonFullAtoms) {
        appended = a.second->append(b, bType, extra);
        if (a.second->full) filled.push_back(a.first);
        if (appended) break;
    }
    for (auto ID : filled) nonFullAtoms.erase(ID);
    if (appended) return;

    // try again with all atoms
    for (auto a : atoms) if (a.second->append(b, bType, extra)) return;
}

void VRMolecule::connectAtom(int ID, int t) {
    if (atoms.count(ID) == 0) return;
    VRAtom* at = atoms[ID];
    if (at->full) return;
    connectAtom(at, t, false);
}

void VRMolecule::remAtom(int ID) {
    if (atoms.count(ID) == 0) return;
    VRAtom* a = atoms[ID];
    atoms.erase(ID);
    delete a;
}

void VRMolecule::updateGeo() {
    VRGeoData atomsData;
    VRGeoData bondsData;

    float r_scale = 0.6;

    for (auto a : atoms) {
        PeriodicTableEntry aP = a.second->getParams();
        atomsData.pushVert( Pnt3d(a.second->getTransformation()[3]), Vec3d(0, r_scale*aP.radius, 0), aP.color );
        atomsData.pushPoint();

        // bonds
        for (auto b : a.second->getBonds()) {
            if (b.second.atom2 == 0) { // duplet
                bondsData.pushVert(b.second.p1, Vec3d(0, 1, 0));
                bondsData.pushVert(b.second.p2, Vec3d(0.1*b.second.type, 1,1));
                bondsData.pushLine();
                continue;
            }

            if (b.second.atom2->getID() < a.first) {
                PeriodicTableEntry bP = b.second.atom2->getParams();
                bondsData.pushVert( Pnt3d(a.second->getTransformation()[3]), Vec3d(0, 1, 0));
                bondsData.pushVert( Pnt3d(b.second.atom2->getTransformation()[3]), Vec3d(0.1*b.second.type, r_scale*aP.radius, r_scale*bP.radius));
                bondsData.pushLine();
            }
        }
    }

    atomsData.apply(ptr());
    if (bonds_geo) bondsData.apply(bonds_geo);

    material = VRMoleculeMat::create();
    material->apply(ptr(), bonds_geo);
    coords_geo->setMaterial(material->getCoordsMaterial()); // avoids sync warning

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
    unsigned int i = 0;
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
        if (VRAtom::PeriodicTable.count(atom2)) j = 2; // search first for double atom2 names like Cl
        atom2 = mol.substr(i, j); // final atom2 type string

        X = parseNumber(mol, i+j); //parse number
        if (X.size() > 0 && verbose) cout << " N: " << X;
        j += X.size();

        int N = 1;
        if (X.size() > 0) N = toInt(X);

        if (VRAtom::PeriodicTable.count(atom2)) {
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
    this->definition = definition;

    vector<string> mol = parse(definition, false);
    atoms.clear();

    for (unsigned int i=0; i<mol.size(); i+=2) {
        string a = mol[i+1];
        int b = toInt(mol[i]);
        if (isNumber(a[0])) connectAtom(toInt(a), b);
        else {
            auto atmID = addAtom(a);
            connectAtom(atmID, b);
        }
    }

    for (auto a : atoms) a.second->computePositions();
    //for (auto a : atoms) a->print();

    updateGeo();
}

void VRMolecule::setRandom(int N) {
    string m;
    int a = 0;
    vector<string> types;
    for(auto a : VRAtom::PeriodicTable) types.push_back(a.first);
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

unsigned int VRMolecule::getFlag() {
	return VRGlobals::CURRENT_FRAME + rand();
}

void VRMolecule::rotateBond(int a, int b, float f) {
    if (atoms.count(a) == 0) return;
    if (atoms.count(b) == 0) return;
    VRAtom* A = atoms[a];
    VRAtom* B = atoms[b];

	unsigned int now = VRGlobals::CURRENT_FRAME + rand();
    A->recFlag = now;

    Vec3d p1 = Vec3d( A->getTransformation()[3] );
    Vec3d p2 = Vec3d( B->getTransformation()[3] );
    Vec3d dir = p2-p1;
    Quaterniond q(dir, f);
    Matrix4d R;
    R.setRotate(q);

    Matrix4d T;
    T[3] = B->getTransformation()[3];
    Matrix4d _T;
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

    Matrix4d am = atoms[a]->getTransformation();
    Matrix4d bm = m->atoms[b]->getTransformation();

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
	unsigned int now = VRGlobals::CURRENT_FRAME + rand();
    A->recFlag = now;
    bm.invert();
    Matrix4d Bm = B->getTransformation();
    bm.mult(Bm);
    bm.setTranslate(Vec3d(0,0,0));
    am.mult(bm);
    MatrixLookAt( bm, Vec3d(0,0,0), Vec3d(0,0,1), Vec3d(0,-1,0) );
    bm.mult(am);
    bm[3] = am[3];
    B->propagateTransformation(bm, now);

    updateGeo();
}

void VRMolecule::setLocalOrigin(int ID) {
    if (atoms.count(ID) == 0) return;

	unsigned int now = VRGlobals::CURRENT_FRAME + rand();
    Matrix4d m = atoms[ID]->getTransformation();
    m.invert();

    Matrix4d im;
    MatrixLookAt( im, Vec3d(0,0,0), Vec3d(0,0,1), Vec3d(0,1,0) );
    im.mult(m);

    atoms[ID]->propagateTransformation(im, now);
}

void VRMolecule::update() {
    updateGeo();
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
    unsigned int now = getFlag();
    A->recFlag = now;
    Matrix4d bm = B->getTransformation();
    B->propagateTransformation(bm, now, false);

    updateGeo();
}

void VRMolecule::showLabels(bool b) { if (doLabels == b) return; doLabels = b; updateLabels(); }
void VRMolecule::showCoords(bool b) { if (doCoords == b) return; doCoords = b; updateCoords(); }

void VRMolecule::updateCoords() {
    if (!coords_geo) return;
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
        Vec4d p0 = a.second->getTransformation()[3];
        Pos->addValue( p0 );
        Pos->addValue( p0 + a.second->getTransformation()[0]*s );
        Pos->addValue( p0 + a.second->getTransformation()[1]*s );
        Pos->addValue( p0 + a.second->getTransformation()[2]*s );
        cols->addValue(Vec3d(0,0,0));
        cols->addValue(Vec3d(1,0,0));
        cols->addValue(Vec3d(0,1,0));
        cols->addValue(Vec3d(0,0,1));
        Norms->addValue( Vec3d(0, 1, 0) );
        Norms->addValue( Vec3d(0, 1, 0) );
        Norms->addValue( Vec3d(0, 1, 0) );
        Norms->addValue( Vec3d(0, 1, 0) );
        Indices->addValue(i+0);
        Indices->addValue(i+1);
        Indices->addValue(i+0);
        Indices->addValue(i+2);
        Indices->addValue(i+0);
        Indices->addValue(i+3);
        i+=4;
    }

    // atoms geometry
    coords_geo->setType(GL_LINES);
    coords_geo->setPositions(Pos);
    coords_geo->setNormals(Norms);
    coords_geo->setColors(cols);
    coords_geo->setIndices(Indices);
    coords_geo->setMaterial(material->getCoordsMaterial());
}

void VRMolecule::updateLabels() {
    if (!labels) return;
    labels->clear();
    if (!doLabels) return;

    //labels->add(Vec3d(), atoms.size(), 0, 0);
    labels->add(Vec3d(), toString(atoms.size()));

    int i=0;
    for (auto a : atoms) {
        Vec3d p = Vec3d(a.second->getTransformation()[3]);
        labels->set(i++, p, toString(a.first));
    }
}

VRAtom* VRMolecule::getAtom(int ID) {
    if (atoms.count(ID) != 0) return atoms[ID];
    return 0;
}

void VRMolecule::setAtomPosition(int ID, Vec3d pos) {
    VRAtom* a = getAtom( ID );
    if (a) {
        Matrix4d m;
        m.setTranslate(pos);
        a->setTransformation(m);
    }
}

Vec3d VRMolecule::getAtomPosition(int ID) {
    VRAtom* a = getAtom( ID );
    if (a == 0) return Vec3d(0,0,0);
    auto m = getWorldMatrix();
    m.mult( a->getTransformation() );
    return Vec3d(m[3]);
}


