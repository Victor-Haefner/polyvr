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

    addType< tuple<int, double> >( "Circle", "a1se|a2f" );
    /*addType< tuple<double, double, double> >("Direction", "a1A0f|a1A1f|a1A2f");
    addType< tuple<double, double, double> >("Cartesian_Point", "a1A0f|a1A1f|a1A2f");
    addType< tuple<int, bool> >( "Oriented_Edge", "a3e|a4b" );
    addType< tuple<int, int, int> >( "Edge_Curve", "a1e|a2e|a3e" );
    addType< tuple<int, int, int> >( "Axis2_Placement_3d", "a1e|a2e|a3e" );
    addType< tuple<int> >( "Manifold_Solid_Brep", "a1e" );
    addType< tuple<int> >( "Plane", "a1e" );
    addType< tuple<int, double> >( "Cylindrical_Surface", "a1e|a2f" );
    addType< tuple<int, int> >( "Line", "a1e|a2e" );
    addType< tuple<int, double> >( "Vector", "a1e|a2f" );
    addType< tuple<int> >( "Vertex_Point", "a1e" );
    addType< tuple<int, bool> >( "Face_Bound", "a1e|a2b" );
    addType< tuple<int, bool> >( "Face_Outer_Bound", "a1e|a2b" );
    addType< tuple<vector<int>, int, bool> >( "Advanced_Face", "a1Ve|a2e|a3b" );
    addType< tuple<string, vector<int> > >( "Advanced_Brep_Shape_Representation", "a0S|a1Ve" );
    addType< tuple<vector<int> > >( "Closed_Shell", "a1Ve" );
    addType< tuple<vector<int> > >( "Edge_Loop", "a1Ve" );*/

    blacklist["Application_Context"] = 1;
    blacklist["Application_Protocol_Definition"] = 1;
    blacklist["Applied_Person_And_Organization_Assignment"] = 1;
    blacklist["Context_Dependent_Shape_Representation"] = 1;
    blacklist["Conversion_Based_Unit"] = 1;
    blacklist["Dimensional_Exponents"] = 1;
    blacklist["Draughting_Pre_Defined_Colour"] = 1;
    blacklist["Fill_Area_Style"] = 1;
    blacklist["Fill_Area_Style_Colour"] = 1;
    blacklist["Geometric_Representation_Context"] = 1;
    blacklist["Item_Defined_Transformation"] = 1;
    blacklist["Length_Unit"] = 1;
    blacklist["Mechanical_Design_Geometric_Presentation_Representation"] = 1;
    blacklist["Named_Unit"] = 1;
    blacklist["Next_Assembly_Usage_Occurrence"] = 1;
    blacklist["Organization"] = 1;
    blacklist["Person"] = 1;
    blacklist["Person_And_Organization"] = 1;
    blacklist["Person_And_Organization_Role"] = 1;
    blacklist["Plane_Angle_Measure_With_Unit"] = 1;
    blacklist["Presentation_Layer_Assignment"] = 1;
    blacklist["Presentation_Style_Assignment"] = 1;
    blacklist["Product"] = 1;
    blacklist["Product_Category"] = 1;
    blacklist["Product_Context"] = 1;
    blacklist["Product_Definition"] = 1;
    blacklist["Product_Definition_Context"] = 1;
    blacklist["Product_Definition_Formation_With_Specified_Source"] = 1;
    blacklist["Product_Definition_Shape"] = 1;
    blacklist["Product_Related_Product_Category"] = 1;
    blacklist["Representation_Relationship"] = 1;
    blacklist["Shape_Definition_Representation"] = 1;
    blacklist["Shape_Representation"] = 1;
    blacklist["Shape_Representation_Relationship"] = 1;
    blacklist["Styled_Item"] = 1;
    blacklist["Surface_Side_Style"] = 1;
    blacklist["Surface_Style_Fill_Area"] = 1;
    blacklist["Surface_Style_Usage"] = 1;
    blacklist["Uncertainty_Measure_With_Unit"] = 1;
}

template<class T> void VRSTEP::addType(string typeName, string path) {
    Type type;
    type.path = path;
    type.cb = VRFunction<STEPentity*>::create("STEPtypeCb", boost::bind( &VRSTEP::parse<T>, this, _1, path, typeName ));
    types[typeName] = type;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, string& t, char c) {
    if (c == 'S') if (a) if (auto r = a->String() ) { r->asStr(t); return true; }
    return false;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, int& t, char c) {
    if (c == 'e') {
        if (e) { t = e->STEPfile_id; return true; }
        if (an) { t = ((STEPentity*)((EntityNode*)an)->node)->STEPfile_id; return true; }
    }
    if (c == 'i') if (a) if (auto r = a->Integer() ) { t = *r; return true; }
    return false;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, double& t, char c) {
    if (c == 'f') {
        if (a) if (auto r = a->Real() ) { t = *r; return true; }
        if (an) { t = ((RealNode*)an)->value; return true; }
    }
    return false;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, bool& t, char c) {
    if (c == 'b') if (a) if (auto r = a->Boolean() ) { t = *r; return true; }
    return false;
}

template<typename T> bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, vector<T>& vec, char t) {
    for( ; an != NULL; an = an->NextNode() ) {
        T v;
        if (!getValue(0,0,an,v,t)) return false;
        vec.push_back(v);
    }
    return true;
}

STEPentity* getSelectEntity(SDAI_Select* s) { // TODO
    auto ap = (SdaiAxis2_placement*)s;
    if (!ap->IsAxis2_placement_3d()) { cout << "getSelectEntity, not an placement 3d!\n"; return 0; }
    SdaiAxis2_placement_3d* ap3 = *ap; // entity
    auto e = (STEPentity*)ap3;
    return e;
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
                e = getSelectEntity(curSel);
            }
        }

        if (c == 's') {
            if (!curAttr) continue;
            curSel = curAttr->Select();
            if (!curSel) { cout << "VRSTEP::query " << i << " is not a Select!\n"; return false; }
            curAttr = 0;
            curAggrNode = 0;
        }

        if (c == 'V') {
            if (!curAttr) continue;
            curAggr = curAttr->Aggregate();
            if (!curAggr) { cout << "VRSTEP::query " << i << " is not an Aggregate!\n"; return false; }
            return getValue(e, curAttr, curAggr->GetHead(), t, path[i+1]);
        }

        if (isLast) return getValue(e, curAttr, curAggrNode, t, c);
        //if (isLast) cout << " t " << t << endl;
    }
    return false;
}

// helper function to set tuple members
template<class T, size_t N> struct Setup {
    static void setup(T& t, STEPentity* e, vector<string>& paths) {
        Setup<T, N-1>::setup(t, e, paths);
        query(e, paths[N-1], get<N-1>(t));
    }
};

template<class T> struct Setup<T, 1> {
    static void setup(T& t, STEPentity* e, vector<string>& paths) {
        query(e, paths[0], get<0>(t));
    }
};

template<class... Args> void setup(tuple<Args...>& t, STEPentity* e, string paths) {
    auto vpaths = splitString(paths, '|');
    Setup<decltype(t), sizeof...(Args)>::setup(t, e, vpaths);
}
// end helper function

template<size_t i, class T, class R>
R getTi(void* d) {
    T* t = (T*)d;
    return std::get<i>(*t);
}

template<class T> void VRSTEP::parse(STEPentity* e, string path, string type) {
    if (instanceByID.count(e->STEPfile_id)) return;
    auto t = new T();
    setup(*t, e, path);
    Instance i;
    i.data = t;
    i.ID = e->STEPfile_id;
    i.type = type;
    instanceByID[e->STEPfile_id] = i;
    instancesByType[type].push_back(i);
    cout << i.type << " A1 " << get<0>(*t) << endl;
    cout << i.type << " A2 " << getTi<0, T, int >(t) << endl;
    cout << i.type << " A3 " << getTi<0, tuple<int, double>, int >(i.data) << endl;
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

void VRSTEP::traverseEntity(STEPentity *se, int lvl) {
    string type = se->EntityName();
    int ID = se->STEPfile_id;

    /*bool k = ID == 1837 || ID == 1836;
    if (k) cout << indent(lvl) << "Entity: " << redBeg << se->EntityName() << " (id: " << ID << ")" << colEnd << " Type: " << se->getEDesc()->Type() << endl;
    else cout << indent(lvl) << "Entity: " << se->EntityName() << " (id: " << ID << " ) Type: " << se->getEDesc()->Type() << endl;
*/
    if (instanceByID.count(ID)) return;
    if (types.count(type)) { (*types[type].cb)(se); return; }
    if (blacklist.count(type) && blacklist[type]) { blacklisted++; return; }

    return;

//    if (resObject.count(ID)) return;

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
    blacklisted = 0;

    for( int i=0; i<instances->InstanceCount(); i++) {
        STEPentity* se = instances->GetSTEPentity(i);
        //if (se->STEPfile_id == 1840) break;
        string name = se->EntityName();
        //if (name != "Advanced_Brep_Shape_Representation") continue;
        //if (name != "Circle") continue;
        traverseEntity(se,1);
    }

    //for (auto i : instancesByType["Advanced_Brep_Shape_Representation"]) {
    for (auto i : instancesByType["Circle"]) {
        cout << " " << i.get<0, int, double>() << endl;
        //cout << " " << getTi<0, tuple<string, vector<int> >, string >(i.data) << endl;
        //cout << i.type << " " << getTi<0, tuple<int, double>, int >(i.data) << endl;
    }

    cout << "build results:\n";
    cout << instanceByID.size() << " STEP entities parsed\n";
    cout << blacklisted << " STEP blacklisted entities ignored\n";
    cout << resObject.size() << " VR objects created\n";
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





