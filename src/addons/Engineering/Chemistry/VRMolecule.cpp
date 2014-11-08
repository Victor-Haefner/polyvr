#include "VRMolecule.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"

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
	AtomicStructures["invalid"] = vector<Matrix>();

    float s2 = sqrt(2);
    float s3 = sqrt(3);
    float _3 = 1.0/3.0;
    Matrix m;

    // linear structure
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );
	MatrixLookAt( m, Vec3f(0,0,1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["linear"].push_back( m );

    // planar structure
	MatrixLookAt( m, Vec3f(0,0,-1), Vec3f(0,0,0), Vec3f(0,-1,0) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,2*s2/3,_3), Vec3f(0,0,0), Vec3f(0,-_3, 2*s2/3) ); AtomicStructures["iso"].push_back( m );
	MatrixLookAt( m, Vec3f(0,-2*s2/3,_3), Vec3f(0,0,0), Vec3f(0,-_3, -2*s2/3) ); AtomicStructures["iso"].push_back( m );

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
    bonds_geo = new VRGeometry("bonds");
    addChild(bonds_geo);

    set(definition);
}

void VRMolecule::addAtom(string a, string b) {
    VRAtom* at = new VRAtom(a, atoms.size());
    if (b == "s") at->bondType = 1;
    if (b == "d") at->bondType = 2;
    if (b == "t") at->bondType = 3;
    for (auto a : atoms) if (a->append(at)) break;
    atoms.push_back(at);
}

void VRMolecule::updateGeo() {
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRecPtr      cols = GeoVec3fProperty::create();

    for (auto a : atoms) {
        cols->addValue(a->getParams().color);
        Pos->addValue(a->getTransformation()[3]);
        //Norms->addValue(a->getTransformation()[1]);
        Vec3f n = Vec3f(0.1*a->bondType, 1,1);
        Norms->addValue(n);
        cout << "norm " << n << endl;
    }

    for (uint i=0; i<Pos->size(); i++) {
        Indices->addValue(i);
    }

    GeoUInt32PropertyRecPtr     Indices2 = GeoUInt32Property::create();
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

    mat->setVertexShader(vp);
    mat->setFragmentShader(fp);
    mat->setGeometryShader(gp);

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
}

vector<string> VRMolecule::parse(string mol) {
    mol += "%  "; // add an ending flag
    vector<string> m;
    uint i = 0;

    while (i < mol.size() ) {
        if (mol[i] == '%') return m; // check for ending flag

        // check for double or triple bounds
        string bond = "s"; // single
        if (mol[i] == '-') bond = "d"; // double
        if (mol[i] == '=') bond = "t"; // triple
        if (mol[i] == '-' or mol[i] == '=') i++;

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
            for (int k=0; k<N; k++) {
                m.push_back(bond);
                m.push_back(atom);
            }
        }

        i += j;
    }


    return m;
}

void VRMolecule::set(string definition) {
    if (PeriodicTable.size() == 0) initAtomicTables();

    vector<string> mol = parse(definition);
    atoms.clear();

    for (int i=0; i<mol.size(); i+=2) {
        string b = mol[i];
        string a = mol[i+1];
        addAtom(a,b);
    }

    for (auto a : atoms) a->computeGeo();
    for (auto a : atoms) a->computePositions();
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
        a = random()%aN;
        m += types[a];
        m += toString(int(1+random()%4));
    }

    set(m);
}

string VRMolecule::fp =
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

string VRMolecule::vp =
"#version 120\n"
GLSL(
varying mat4 view;
varying mat4 model;
varying vec4 color;

void main( void ) {
    view = gl_ProjectionMatrix;
    model = gl_ModelViewMatrix;
    color = gl_Color;
    gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;
}
);

string VRMolecule::gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in mat4 view[];
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
	p.z -= 0.02;

	float a = OSGViewportSize.x/OSGViewportSize.y;

	vec4 u = vec4(s/a,0,0,0);
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
