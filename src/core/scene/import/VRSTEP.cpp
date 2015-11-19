#include "VRSTEP.h"

#include <STEPfile.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>
#include <tuple>
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

    addType< tuple<double, double, double> >("Direction", "a1A0f|a1A1f|a1A2f");
    addType< tuple<double, double, double> >("Cartesian_Point", "a1A0f|a1A1f|a1A2f");
    addType< tuple<int, double> >( "Circle", "a1se|a2f" );
    addType< tuple<int, bool> >( "Oriented_Edge", "a3e|a4b" );
    addType< tuple<int, int, int> >( "Edge_Curve", "a1ea1e|a2ea1e|a3e" );
    addType< tuple<int, int, int> >( "Axis2_Placement_3d", "a1e|a2e|a3e" );
    addType< tuple<int> >( "ManifoldSolidBrep", "a1e" );
}

template<class T> void VRSTEP::addType(string typeName, string path) {
    Type type;
    type.path = path;
    type.cb = VRFunction<STEPentity*>::create("STEPtypeCb", boost::bind( &VRSTEP::parse<T>, this, _1, path ));
    types[typeName] = type;
}

void getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, string& t, char c) {
    if (c == 'S') if (a) if (auto r = a->String() ) r->asStr(t);
}

void getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, int& t, char c) {
    if (c == 'e') t = e->STEPfile_id;
    if (c == 'i') if (a) if (auto r = a->Integer() ) t = *r;
}

void getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, double& t, char c) {
    if (c == 'f') {
        if (a) if (auto r = a->Real() ) t = *r;
        if (an) t = ((RealNode*)an)->value;
    }
}

void getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, bool& t, char c) {
    if (c == 'b') if (a) if (auto r = a->Boolean() ) t = *r;
}

template<typename T> bool query(STEPentity* e, string path, T& t) {
    auto toint = [](char c) { return int(c-'0'); };

    int j = 1;
    STEPattribute* curAttr = 0;
    STEPaggregate* curAggr = 0;
    SDAI_Select* curSel = 0;
    SingleLinkNode* curAggrNode = 0;
    for (int i=0; i<path.size(); i+=j) {
        bool isLast = (i == path.size()-1);
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
            if (!curAggr) { cout << "VRSTEP::query " << i << " is not an Aggregate!\n"; return false; }
            if ('0' <= path[i+1] && path[i+1] <= '9') {
                int Ai = toint(path[i+1]);
                curAggrNode = curAggr->GetHead();
                for (int i=0; i<Ai; i++) curAggrNode = curAggrNode->NextNode();
            }
            curAttr = 0;
        }
        if (c == 'e') {
            if (curAttr) e = curAttr->Entity();
            if (curSel) {
                if (curSel->ValueType() != ENTITY_TYPE) { cout << "VRSTEP::query " << i << " is not an entity!\n"; return false; }
                e = *((STEPentity**)curSel);
            }
        }
        if (c == 's') {
            if (!curAttr) continue;
            curSel = curAttr->Select();
            if (!curSel) { cout << "VRSTEP::query " << i << " is not a Select!\n"; return false; }
            curAttr = 0;
            curAggrNode = 0;
        }
        if (isLast) getValue(e, curAttr, curAggrNode, t, c);
    }
    return false;
}

template<class V, class T> bool queryVec(STEPentity* se, string paths, V& v) {
    int i=0;
    for (auto p : splitString(paths, '|')) {
        if (!query<T>(se, p, v[i])) return false;
        i++;
    }
    return true;
}

// helper function to set tuple members
template<class T, size_t N> struct Setup {
    static void setup(const T& t, STEPentity* e, vector<string>& paths) {
        Setup<T, N-1>::setup(t, e, paths);
        auto v = get<N-1>(t);
        query(e, paths[N-1], v);
    }
};

template<class T> struct Setup<T, 1> {
    static void setup(const T& t, STEPentity* e, vector<string>& paths) {
        auto v = get<0>(t);
        query(e, paths[0], v);
    }
};

template<class... Args> void setup(const tuple<Args...>& t, STEPentity* e, string paths) {
    auto vpaths = splitString(paths, '|');
    Setup<decltype(t), sizeof...(Args)>::setup(t, e, vpaths);
}
// end helper function

template<class T> void VRSTEP::parse(STEPentity* e, string path) {
    if (resMap.count(e->STEPfile_id)) return;
    T* t = new T();
    setup(*t, e, path);
    resMap[e->STEPfile_id] = t;
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

bool VRSTEP::parseClosedShell(STEPentity* se) {
    auto* s = new ClosedShell();
    s->faces = getAggregateEntities(&se->attributes[1]);
    resMap[se->STEPfile_id] = s; return true;
}

bool VRSTEP::parseAdvancedBrepShapeRepresentation(STEPentity* se) {
    auto* s = new AdvancedBrepShapeRepresentation();
    if ( !query(se, "a0S", s->name) ) return false;
    s->items = getAggregateEntities(&se->attributes[1]);
    resMap[se->STEPfile_id] = s; return true;
}

/*{
    auto res = VRGeometry::create(name);
    resObject[se->STEPfile_id] = res; return true;
}*/

vector<int> VRSTEP::getAggregateEntities(STEPattribute* attr) {
    vector<int> res;
    if (attr->BaseType() != ENTITY_TYPE) return res;
    auto aggr = attr->Aggregate(); if (!aggr) return res;
    for( auto sn = (EntityNode*)aggr->GetHead(); sn != NULL; sn = (EntityNode*)sn->NextNode()) {
        auto e = (STEPentity*)sn->node;
        res.push_back( e->STEPfile_id );
    }
    return res;
}

void VRSTEP::traverseEntity(STEPentity *se, int lvl) {
    string type = se->EntityName();

    int ID = se->STEPfile_id;
    bool k = ID == 1837 || ID == 1836;
    if (k) cout << indent(lvl) << "Entity: " << redBeg << se->EntityName() << " (id: " << se->STEPfile_id << ")" << colEnd << " Type: " << se->getEDesc()->Type() << endl;
    else cout << indent(lvl) << "Entity: " << se->EntityName() << " (id: " << se->STEPfile_id << " ) Type: " << se->getEDesc()->Type() << endl;

    if (types.count(type)) { (*types[type].cb)(se); return; }

    if (resMap.count(se->STEPfile_id)) return;
    if ( type == "ClosedShell" && parseClosedShell(se) ) return;
    if ( type == "Advanced_Brep_Shape_Representation" && parseAdvancedBrepShapeRepresentation(se) ) return;

//    if (resObject.count(se->STEPfile_id)) return;

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
    cout << resMap.size() << " instances\n";
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





