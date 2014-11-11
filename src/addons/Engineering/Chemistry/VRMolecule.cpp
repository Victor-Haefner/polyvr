#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "addons/Factory/VRNumberingEngine.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGShaderVariableOSG.h>

#define GLSL(shader) #shader

using namespace OSG;

map<string, PeriodicTableEntry> PeriodicTable;
map<string, vector<Matrix> > AtomicStructures;

#include <OpenSG/OSGMatrixUtility.h>

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
PeriodicTableEntry::PeriodicTableEntry(int valence_electrons, Vec3f color) {
    this->valence_electrons = valence_electrons;
    this->color = color;
}

VRBond::VRBond(int t, int s, VRAtom* a) { type = t; atom = a; slot = s; }
VRBond::VRBond() {}

VRAtom::VRAtom(string type, int ID) {
    this->ID = ID;
    this->type = type;
    params = PeriodicTable[type];
}

VRAtom::~VRAtom() {
    for (auto b : bonds) {
        VRAtom* a = b.second.atom;
        a->full = false;
        a->bonds.erase(b.second.slot);
    }
}

PeriodicTableEntry VRAtom::getParams() { return params; }
Matrix VRAtom::getTransformation() { return transformation; }
void VRAtom::setTransformation(Matrix m) { transformation = m; }
map<int, VRBond> VRAtom::getBonds() { return bonds; }
int VRAtom::getID() { return ID; }

void VRAtom::setID(int ID) {
    this->ID = ID;
}

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
    // fill in the duplets
    for (int i=4; i<params.valence_electrons; i++) bonds[7-i] = VRBond(4,7-i,0);

    computeGeo();

    string g = geo;
    if (AtomicStructures.count(geo) == 0) { cout << "Error: " << geo << " is invalid!\n"; return; }

    vector<Matrix> structure = AtomicStructures[geo];
    for (auto& b : bonds) {
        if (b.first >= structure.size()) break;
        if (b.second.extra) continue;

        Matrix T = transformation;
        Matrix S = structure[b.second.slot];

        VRAtom* a = b.second.atom;
        if (a == 0) {
            S[3] *= 0.2; S[3][3] = 1;
            T.mult(S);
            T.mult(Pnt3f(0.2,0,0), b.second.p1);
            T.mult(Pnt3f(-0.2,0,0), b.second.p2);
            continue;
        }

        if (a->ID <= ID) continue;
        T.mult(S);
        a->transformation = T;
    }
}

bool VRAtom::append(VRBond bond) {
    VRAtom* at = bond.atom;
    if (full or at->full or at == this) return false;
    for (auto b : bonds) if (b.second.atom == at) return false;

    int bmax = 4 - abs(params.valence_electrons - 4);

    int slot=0;
    for (; bonds.count(slot) == 1; slot++);

    bond.slot = slot;
    bonds[slot] = bond;
    bound_valence_electrons += bond.type;

    bond.atom = this;
    at->append(bond);

    //print();
    if (bound_valence_electrons >= bmax) full = true;
    return true;
}

void VRAtom::print() {
    cout << " ID: " << ID << " Type: " << type << " boundEl: " << bound_valence_electrons << " geo: " << geo << " pos: " << Vec3f(transformation[3]);
    cout << " bonds with: ";
    for (auto b : bonds) {
        if (b.second.atom == 0) cout << " " << "pair";
        else cout << " " << b.second.atom->ID;
    }
    cout << endl;
}


VRMolecule::VRMolecule(string definition) : VRGeometry(definition) {
    bonds_geo = new VRGeometry("bonds");
    addChild(bonds_geo);

    labels = new VRNumberingEngine();
    labels->setBillboard(true);
    labels->setOnTop(true);
    labels->setSize(0.1);
    addChild(labels);

    set(definition);
}

void VRMolecule::addAtom(string a, int t) {
    VRBond b(t, 0, new VRAtom(a, getID()) );
    atoms[b.atom->getID()] = b.atom;
    addAtom(b);
}

void VRMolecule::addAtom(VRBond b) {
    for (auto a : atoms) if (a.second->append(b)) break;
}

void VRMolecule::addAtom(int ID, int t) {
    if (ID >= atoms.size() or ID < 0) return;
    VRAtom* at = atoms[ID];
    if (at->full) return;

    VRBond b(t, 0, at);
    b.extra = true;
    addAtom(b);
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

    // hack to avoid the single point bug
    if (atoms.size() == 1) atoms[1] = atoms[0];

    int i=0;
    int j=0;
    cout << "GEN\n";
    for (auto a : atoms) {
        a.second->print();
        cols->addValue(a.second->getParams().color);
        Pos->addValue(a.second->getTransformation()[3]);
        Norms->addValue( Vec3f(0, 1, 0) );
        Indices->addValue(i++);

        // bonds
        for (auto b : a.second->getBonds()) {
            if (b.second.atom == 0) { // duplet
                Pos2->addValue(b.second.p1);
                Pos2->addValue(b.second.p2);
                Norms2->addValue( Vec3f(0, 1, 0) );
                Norms2->addValue( Vec3f(0.1*b.second.type, 1,1) );
                Indices2->addValue(j++);
                Indices2->addValue(j++);
                continue;
            }

            if (b.second.atom->getID() < a.first) {
                Pos2->addValue(a.second->getTransformation()[3]);
                Pos2->addValue(b.second.atom->getTransformation()[3]);
                Norms2->addValue( Vec3f(0, 1, 0) );
                Norms2->addValue( Vec3f(0.1*b.second.type, 1,1) );
                Indices2->addValue(j++);
                Indices2->addValue(j++);
            }
        }
    }

    // atoms geometry
    VRMaterial* mat = VRMaterial::get("atoms");
    mat->setPointSize(40);
    mat->setLit(false);
    mat->setVertexShader(a_vp);
    mat->setFragmentShader(a_fp);
    mat->setGeometryShader(a_gp);

    setType(GL_POINTS);
    setPositions(Pos);
    setNormals(Norms);
    setColors(cols);
    setIndices(Indices);
    setMaterial(mat);

    // bonds geometry
    VRMaterial* mat2 = VRMaterial::get("molecule_bonds");
    mat2->setLineWidth(5);
    mat2->setLit(false);
    mat2->setVertexShader(b_vp);
    mat2->setFragmentShader(b_fp);
    mat2->setGeometryShader(b_gp);

    bonds_geo->setType(GL_LINES);
    bonds_geo->setPositions(Pos2);
    bonds_geo->setNormals(Norms2);
    bonds_geo->setColors(cols);
    bonds_geo->setIndices(Indices2);
    bonds_geo->setMaterial(mat2);

    updateLabels();
}

bool isNumber(char c) { return (c >= '0' and c <= '9'); }

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

        // check for double or triple bounds
        string bond = "1"; // single
        if (mol[i] == '-') bond = "2"; // double
        if (mol[i] == '=') bond = "3"; // triple
        if (mol[i] == '-' or mol[i] == '=') i++;

        // check for bond with ID atom
        X = parseNumber(mol, i); //parse number
        if (X.size() > 0) {
            res.push_back(bond);
            res.push_back(X);
            i += X.size();
            if (verbose) cout << " ID: " << X;
            continue;
        }

        int j = 1;
        string atom = mol.substr(i, 2);
        if (PeriodicTable.count(atom)) j = 2; // search first for double atom names like Cl
        atom = mol.substr(i, j); // final atom type string

        X = parseNumber(mol, i+j); //parse number
        if (X.size() > 0 and verbose) cout << " N: " << X;
        j += X.size();

        int N = 1;
        if (X.size() > 0) N = toInt(X);

        if (PeriodicTable.count(atom)) {
            for (int k=0; k<N; k++) {
                res.push_back(bond);
                res.push_back(atom);
            }
        }

        i += j;
    }

    if (verbose) cout << endl;


    return res;
}

void VRMolecule::set(string definition) {
    if (PeriodicTable.size() == 0) initAtomicTables();

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
        int bt = random()%20;
        if (bt == 0) m += '=';
        else if (bt < 4) m += '-';
        a = random()%aN;
        m += types[a];
        m += toString(int(1+random()%4));
    }

    set(m);
}

int VRMolecule::getID() {
    int i=0;
    while (atoms.count(i)) i++;
    return i;
}

void VRMolecule::substitute(int a, VRMolecule* m, int b) {
    if (atoms.count(a) == 0) return;
    if (m->atoms.count(b) == 0) return;

    Matrix am = atoms[a]->getTransformation();

    map<int, VRBond> bondsA = atoms[a]->getBonds();
    map<int, VRBond> bondsB = m->atoms[b]->getBonds();
    if (bondsA.count(0) == 0) return;
    if (bondsB.count(0) == 0) return;

    VRAtom* A = bondsA[0].atom;
    VRAtom* B = bondsB[0].atom;
    remAtom(a);
    m->remAtom(b);

    for (auto at : m->atoms) {
        at.second->setID( getID() );
        atoms[at.second->getID()] = at.second;

        Matrix atm = at.second->getTransformation();
        Matrix Am = am;//A->getTransformation();
        Am.mult(atm);
        at.second->setTransformation(Am);
    }
    m->atoms.clear();

    VRBond bond(1,0,B);
    bond.extra = true;
    A->append(bond);

    updateGeo();
    m->updateGeo();
}

void VRMolecule::showLabels(bool b) {
    if (doLabels == b) return;
    doLabels = b;
    updateLabels();
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

void main( void ) {
    color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
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
	emitQuad(0.2, vec4(0,1,0,1));
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

void main( void ) {
    normal = gl_Normal;
    gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
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

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 d = pl2-pl1;
	pl1 += 0.9*s*d;
	pl2 -= 0.9*s*d;

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
	float w = 0.06;
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
