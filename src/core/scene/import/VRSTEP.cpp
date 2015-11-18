#include "VRSTEP.h"

#include <STEPfile.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>
#include <boost/bind.hpp>

#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"

/*

IMPORTANT: ..not compiling? you need to install the stepcode package!
open a terminal
cd ~/Project/polyvr/dependencies
sudo git pull
cd ubuntu_14.04
sudo gdebi -n libstepcode-dev.deb

*/

using namespace std;
using namespace OSG;

void VRSTEP::loadT(string file, STEPfilePtr sfile, bool* done) {
    sfile->ReadExchangeFile(file);
    registry->ResetSchemas();
    registry->ResetEntities();
    *done = true;
}

VRSTEP::VRSTEP() {
    registry = RegistryPtr( new Registry( SchemaInit ) ); // schema
    instances = InstMgrPtr( new InstMgr() ); // instances
    sfile = STEPfilePtr( new STEPfile( *registry, *instances, "", false ) ); // file

    addType("Direction", "a1A0f|a1A1f|a1A2f");
    addType("Cartesian_Point", "a1A0f|a1A1f|a1A2f");
}

void VRSTEP::addType(string type, string path) {
    Type t;
    t.path = path;
    t.cb = VRFunction<STEPentity*>::create("STEPtypeCb", boost::bind( &VRSTEP::parse, this, _1, path ));
    types[type] = t;
}

void VRSTEP::parse(STEPentity* e, string path) {
    if (resVec3f.count(e->STEPfile_id)) return;
    Vec3f v = queryVec<Vec3f, float>(e, path);
    resVec3f[e->STEPfile_id] = v;
}

void VRSTEP::open(string file) {
    bool done = false;
    thread t(&VRSTEP::loadT, this, file, sfile, &done);

    while(!done) {
        auto p = sfile->GetReadProgress();
        cout << "progress " << p << endl;
        sleep(1);
    }

    t.join();
}


string VRSTEP::indent(int lvl) {
    string s;
    for ( int i=0; i< lvl; i++) s += "    ";
    return s;
}

template<class T> T VRSTEP::query(STEPentity* e, string path) {
    auto toint = [](char c) { return int(c-'0'); };

    int j = 1;
    STEPattribute* curAttr = 0;
    STEPaggregate* curAggr = 0;
    SingleLinkNode* curAggrNode = 0;
    for (int i=0; i<path.size(); i+=j) {
        j = 1;
        auto c = path[i];
        if (c == 'a') {
            j = 2;
            int ai = toint(path[i+1]);
            curAttr = &e->attributes[ai];
            curAggr = 0;
        }
        if (c == 'A') {
            j = 2;
            if (!curAttr) continue;
            curAggr = curAttr->Aggregate();
            if (!curAggr) { cout << "VRSTEP::query " << i << " is not an Aggregate!\n"; return 0; }
            if ('0' <= path[i+1] && path[i+1] <= '9') {
                int Ai = toint(path[i+1]);
                curAggrNode = curAggr->GetHead();
                for (int i=0; i<Ai; i++) curAggrNode = curAggrNode->NextNode();
            }
            curAttr = 0;
        }
        if (c == 'f') {
            if (curAttr) if (auto r = curAttr->Real() ) return *r;
            if (curAggrNode) return ((RealNode*)curAggrNode)->value;
        }
    }

    return 0;
}

template<class V, class T> V VRSTEP::queryVec(STEPentity* se, string paths) {
    vector<string> pathsv = splitString(paths, '|');
    V v; int i=0;
    for (auto p : pathsv) { v[i] = query<T>(se, p); i++; }
    return v;
}

bool VRSTEP::parseCircle(STEPentity* se) {
    if (resCircle.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 3) return false;
    auto sPos = se->attributes[1].Select(); if (!sPos) return false;
    auto r = se->attributes[2].Real(); if (!r) return false;
    if (sPos->ValueType() != ENTITY_TYPE) return false;
    auto ap = (SdaiAxis2_placement*)sPos;
    if (!ap->IsAxis2_placement_3d()) return false;
    SdaiAxis2_placement_3d* ap3 = *ap; // entity
    if (!parsePose((STEPentity*)ap3)) return false;
    Circle c;
    c.p = resPose[ap3->STEPfile_id];
    c.r = *r;
    resCircle[se->STEPfile_id] = c;
    return true;
}

bool VRSTEP::parseOrientedEdge(STEPentity* se) {
    if (resOrientedEdge.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 5) return false;
    auto ec = se->attributes[3].Entity(); if (!ec) return false;
    auto b  = se->attributes[4].Boolean(); if (!b) return false;
    if (!resEdgeCurve.count(ec->STEPfile_id)) if (!parseEdgeCurve(ec)) return false;
    OrientedEdge oe;
    oe.ec = resEdgeCurve[ec->STEPfile_id];
    oe.dir = *b;
    resOrientedEdge[se->STEPfile_id] = oe;
    return true;
}

bool VRSTEP::parseEdgeCurve(STEPentity* se) {
    if (resEdgeCurve.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 5) return false;
    auto start1 = se->attributes[1].Entity(); if (!start1) return false;
    auto end1   = se->attributes[2].Entity(); if (!end1)   return false;
    auto geo    = se->attributes[3].Entity(); if (!geo) return false;
    if (start1->AttributeCount() != 2) return false;
    if (end1  ->AttributeCount() != 2) return false;
    auto start2 = start1->attributes[1].Entity(); if (!start2) return false;
    auto end2   = end1  ->attributes[1].Entity(); if (!end2)   return false;
    //if (!parseVec3f(start2)) return false;
    //if (!parseVec3f(end2)) return false;
    EdgeCurve ec;
    ec.start = resVec3f[start2->STEPfile_id];
    ec.end   = resVec3f[end2->STEPfile_id];
    if (parseCircle(geo)) ec.c = resCircle[geo->STEPfile_id];
    //if (parseLine(geo))   ec.l = resLine[geo->STEPfile_id];
    resEdgeCurve[se->STEPfile_id] = ec;
    return true;
}

bool VRSTEP::parseVector(STEPentity* se) {
    if (resVec3f.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 3) return false;
    auto dir = se->attributes[1].Entity(); if (!dir) return false;
    auto L   = se->attributes[2].Real();   if (!L) return false;
    //if (!parseVec3f(dir)) return false;
    Vec3f v = resVec3f[dir->STEPfile_id];
    v *= *L;
    resVec3f[se->STEPfile_id] = v;
    return true;
}

bool VRSTEP::parsePose(STEPentity* se) {
    if (resPose.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 4) return false;
    vector<STEPentity*> ents(3);
    vector<STEPattribute*> attribs(3);
    for (int i=0; i<3; i++) {
        attribs[i] = &se->attributes[i+1];
        if (attribs[i]->BaseType() != 256) return false;
    }
    for (int i=0; i<3; i++) {
        ents[i] = attribs[i]->Entity();
        //if (!parseVec3f(ents[i])) return false;
    }
    resPose[se->STEPfile_id] = pose( resVec3f[ents[0]->STEPfile_id],
                                     resVec3f[ents[1]->STEPfile_id],
                                     resVec3f[ents[2]->STEPfile_id] );
    return true;
}

bool VRSTEP::parseAdvancedBrepShapeRepresentation(STEPentity* se) {
    if (string(se->EntityName()) != "Advanced_Brep_Shape_Representation") return false;
    if (resObject.count(se->STEPfile_id)) return true;
    if (se->AttributeCount() != 3) return false;
    SDAI_String* sname = se->attributes[0].String(); if (!sname) return false;
    string name; sname->asStr(name);

    auto ents = getAggregateEntities(&se->attributes[1]);
    for (auto e : ents) {
        ;
    }

    auto res = VRGeometry::create(name);
    resObject[se->STEPfile_id] = res;
    return true;
}

vector<STEPentity*> VRSTEP::getAggregateEntities(STEPattribute* attr) {
    vector<STEPentity*> res;
    if (attr->BaseType() != ENTITY_TYPE) return res;
    auto aggr = attr->Aggregate(); if (!aggr) return res;
    for( auto sn = (EntityNode*)aggr->GetHead(); sn != NULL; sn = (EntityNode*)sn->NextNode()) {
        res.push_back( (STEPentity*)sn->node );
    }
    return res;
}

void VRSTEP::traverseEntity(STEPentity *se, int lvl) {
    string type = se->EntityName();

    int ID = se->STEPfile_id;
    bool k = ID == 1837 || ID == 1836;
    if (k) cout << indent(lvl) << "Entity: " << redBeg << se->EntityName() << " (id: " << se->STEPfile_id << ")" << colEnd << " Type: " << se->getEDesc()->Type() << endl;
    else cout << indent(lvl) << "Entity: " << se->EntityName() << " (id: " << se->STEPfile_id << " ) Type: " << se->getEDesc()->Type() << endl;

    if (types.count(type)) (*types[type].cb)(se);

    if ( parseVector(se) ) return;
    if ( parsePose(se) ) return;
    if ( parseCircle(se) ) return;
    if ( parseEdgeCurve(se) ) return;
    if ( parseOrientedEdge(se) ) return;
    if ( parseAdvancedBrepShapeRepresentation(se) ) return;

    STEPattribute* attr;
    se->ResetAttributes();
    while ( (attr = se->NextAttribute()) != NULL ) {
        cout << indent(lvl+1) << "A: " << attr->Name() << " : " << attr->asStr() << " TypeName: " << attr->TypeName() << " Type: " << attr->Type() << endl;
        if ( attr->Entity() && !attr->IsDerived()) traverseEntity( attr->Entity(), lvl+2);
        if ( auto a = attr->Aggregate() ) traverseAggregate(a, attr->BaseType(), lvl+2);
        if ( auto s = attr->Select() ) traverseSelect(s, lvl+2, attr);
        if ( auto i = attr->Integer() ) cout << indent(lvl+1) << " Integer: " << *i << endl;
        if ( auto r = attr->Real() ) cout << indent(lvl+1) << " Real: " << *r << endl;
        if ( auto n = attr->Number() ) cout << indent(lvl+1) << " Number: " << *n << endl;
        if ( auto s = attr->String() ) cout << indent(lvl+1) << " String: " << s << endl;
        if ( auto b = attr->Binary() ) cout << indent(lvl+1) << " Binary: " << b << endl;
        if ( auto e = attr->Enum() ) cout << indent(lvl+1) << " Enum: " << *e << endl;
        if ( auto l = attr->Logical() ) cout << indent(lvl+1) << " Logical: " << *l << endl;
        if ( auto b = attr->Boolean() ) cout << indent(lvl+1) << " Boolean: " << *b << endl;
        //if ( auto u = attr->Undefined() ) cout << "Undefined: " << u << endl;
        //if ( attr->Type() == REFERENCE_TYPE ) cout << indent(lvl+1) << " ref: " << attr << " " << endl;
    }
}

void VRSTEP::traverseSelect(SDAI_Select* s, int lvl, STEPattribute* attr) {
    string stype;
    s->UnderlyingTypeName().asStr(stype);
    SdaiAxis2_placement* v;

    switch(s->ValueType()) {
        case ENTITY_TYPE:
            v = (SdaiAxis2_placement*)s;
            if (v->IsAxis2_placement_2d()) { SdaiAxis2_placement_2d* a2 = *v; traverseEntity( (STEPentity*)a2, lvl+2); break; }
            if (v->IsAxis2_placement_3d()) { SdaiAxis2_placement_3d* a3 = *v; traverseEntity( (STEPentity*)a3, lvl+2); break; }
        default:
            cout << indent(lvl) << "Select type not handled: " << s->ValueType() << endl;
    }
}

void VRSTEP::traverseAggregate(STEPaggregate *sa, int atype, int lvl) {
    string s; sa->asStr(s);
    cout << indent(lvl) << "Aggregate: " << s << endl;

    STEPentity* sse;
    SelectNode* sen;
    SDAI_Select* sdsel;
    PrimitiveType etype, ebtype;

    for( EntityNode* sn = (EntityNode*)sa->GetHead(); sn != NULL; sn = (EntityNode*)sn->NextNode()) {
        switch (atype) {
            case ENTITY_TYPE: // 100
                sse = (STEPentity*)sn->node;
                etype = sse->getEDesc()->Type();
                ebtype = sse->getEDesc()->BaseType();
                switch (etype) {
                    case SET_TYPE:
                    case LIST_TYPE: traverseAggregate((STEPaggregate *)sse, ebtype, lvl+2); break;
                    case ENTITY_TYPE: traverseEntity(sse,lvl+2); break;
                    default: cout << indent(lvl+1) << "entity Type not handled:" << etype << endl;
                }
                break;
            case SELECT_TYPE: // 80
                sen = (SelectNode*)sn;
                sdsel = sen->node;
                sdsel->UnderlyingTypeName().asStr(s);
                cout << "SELECT " << s << endl;
                break;
            case INTEGER_TYPE: // 1
            case REAL_TYPE: // 2
            case BOOLEAN_TYPE: // 4
            case LOGICAL_TYPE: // 8
            case STRING_TYPE: // 10
            case BINARY_TYPE: // 20
            case ENUM_TYPE: // 40
            case AGGREGATE_TYPE: // 200
            case NUMBER_TYPE: // 400
            default: cout << indent(lvl+1) << "aggregate Type not handled:" << atype << endl;
            ;
        }
    }
}

void VRSTEP::build() {
    for( int i=0; i<instances->InstanceCount(); i++) {
        STEPentity* se = instances->GetSTEPentity(i);
        //if (se->STEPfile_id == 1840) break;
        string name = se->EntityName();
        //if (name != "Advanced_Brep_Shape_Representation") continue;
        if (name != "Circle") continue;
        traverseEntity(se,1);
    }

    cout << "build results:\n";
    cout << resVec3f.size() << " vectors\n";
    cout << resPose.size() << " poses\n";
    cout << resObject.size() << " objects\n";

    /*cout << "Type enum:\n";
    cout << "INTEGER_TYPE: " << INTEGER_TYPE << endl;
    cout << "REAL_TYPE: " << REAL_TYPE << endl;
    cout << "BOOLEAN_TYPE: " << BOOLEAN_TYPE << endl;
    cout << "LOGICAL_TYPE: " << LOGICAL_TYPE << endl;
    cout << "STRING_TYPE: " << STRING_TYPE << endl;
    cout << "BINARY_TYPE: " << BINARY_TYPE << endl;
    cout << "ENUM_TYPE: " << ENUM_TYPE << endl;
    cout << "SELECT_TYPE: " << SELECT_TYPE << endl;
    cout << "AGGREGATE_TYPE: " << AGGREGATE_TYPE << endl;
    cout << "NUMBER_TYPE: " << NUMBER_TYPE << endl;
    cout << "ARRAY_TYPE: " << ARRAY_TYPE << endl;
    cout << "BAG_TYPE: " << BAG_TYPE << endl;
    cout << "SET_TYPE: " << SET_TYPE << endl;
    cout << "LIST_TYPE: " << LIST_TYPE << endl;
    cout << "GENERIC_TYPE: " << GENERIC_TYPE << endl;
    cout << "REFERENCE_TYPE: " << REFERENCE_TYPE << endl;
    cout << "UNKNOWN_TYPE: " << UNKNOWN_TYPE << endl;*/
}

VRTransformPtr VRSTEP::load(string file) {
    open(file);
    build();
    VRTransformPtr root = VRTransform::create(file);
    for (auto o : resObject) root->addChild(o.second);
    return root;
}

/* ------------------------------ DOC -----------------------------------

The shape entities that get translated to scene objects:

advanced_brep_shape_representation
faceted_brep_shape_representation
geometrically_bounded_wireframe_shape_representation
geometrically_bounded_surface_shape_representation
hybrid representations
manifold_surface_shape_representation

The topological entities:

vertices
edges
loops
faces
shells
solids

The geometric entities:

points -> Vec3f
vectors -> Vec3f
directions -> Vec3f
curves
surfaces

*/





