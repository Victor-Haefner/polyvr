#include "VRSTEP.h"

#include <STEPfile.h>
#include <STEPcomplex.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>
#include <boost/bind.hpp>

#include <OpenSG/OSGGeoProperties.h>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/polygon.h"

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

    addType< tuple<STEPentity*, double> >( "Circle", "a1se|a2f", true );
    addType< tuple<double, double, double> >("Direction", "a1A0f|a1A1f|a1A2f");
    addType< tuple<double, double, double> >("Cartesian_Point", "a1A0f|a1A1f|a1A2f");
    addType< tuple<STEPentity*, bool> >( "Oriented_Edge", "a3e|a4b" );
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Edge_Curve", "a1e|a2e|a3e" );
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Axis2_Placement_3d", "a1e|a2e|a3e" );
    addType< tuple<STEPentity*> >( "Manifold_Solid_Brep", "a1e" );
    addType< tuple<STEPentity*> >( "Plane", "a1e" );
    addType< tuple<STEPentity*, double> >( "Cylindrical_Surface", "a1e|a2f" );
    addType< tuple<STEPentity*, STEPentity*> >( "Line", "a1e|a2e" );
    addType< tuple<STEPentity*, double> >( "Vector", "a1e|a2f" );
    addType< tuple<STEPentity*> >( "Vertex_Point", "a1e" );
    addType< tuple<STEPentity*, bool> >( "Face_Bound", "a1e|a2b" );
    addType< tuple<STEPentity*, bool> >( "Face_Outer_Bound", "a1e|a2b" );
    addType< tuple<vector<STEPentity*>, STEPentity*, bool> >( "Advanced_Face", "a1Ve|a2e|a3b" );
    addType< tuple<string, vector<STEPentity*> > >( "Advanced_Brep_Shape_Representation", "a0S|a1Ve" );
    addType< tuple<vector<STEPentity*> > >( "Closed_Shell", "a1Ve" );
    addType< tuple<vector<STEPentity*> > >( "Edge_Loop", "a1Ve" );

    // assembly entities
    addType< tuple<STEPentity*, STEPentity*> >( "Product_Definition_Relationship", "a3e|a4e" );
    addType< tuple<STEPentity*, STEPentity*> >( "Product_Definition_Usage", "a3e|a4e" );
    addType< tuple<STEPentity*, STEPentity*> >( "Assembly_Component_Usage", "a3e|a4e" );
    addType< tuple<STEPentity*, STEPentity*> >( "Next_Assembly_Usage_Occurrence", "a3e|a4e", false );

    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Context_Dependent_Shape_Representation", "a0ec0e|a0ec1e|a1e", true );
    addType< tuple<STEPentity*, STEPentity*> >( "Representation_Relationship", "a2e|a3e", false );
    addType< tuple<STEPentity*> >( "Representation_Relationship_With_Transformation", "a0se", true );
    addType< tuple<STEPentity*, STEPentity*> >( "Item_Defined_Transformation", "a2e|a3e", false );

    addType< tuple<vector<STEPentity*>, STEPentity*> >( "Shape_Representation", "a1Ve|a2e", false );
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Representation_Relationship", "a2e|a3e", false );
    //addType< tuple<STEPentity*, STEPentity*> >( "Geometric_Representation_Context", "a2e|a3e" );

    //addType< tuple<STEPentity*, STEPentity*> >( "Product_Definition_Shape", "a2se");

    {
    blacklist["Application_Context"] = 1;
    blacklist["Application_Protocol_Definition"] = 0;
    blacklist["Applied_Person_And_Organization_Assignment"] = 0;
    blacklist["Conversion_Based_Unit"] = 0;
    blacklist["Dimensional_Exponents"] = 0;
    blacklist["Draughting_Pre_Defined_Colour"] = 1;
    blacklist["Fill_Area_Style"] = 1;
    blacklist["Fill_Area_Style_Colour"] = 1;
    blacklist["Geometric_Representation_Context"] = 1;
    blacklist["Global_Uncertainty_Assigned_Context"] = 1;
    blacklist["Global_Unit_Assigned_Context"] = 1;
    blacklist["Length_Unit"] = 0;
    blacklist["Mechanical_Design_Geometric_Presentation_Representation"] = 0;
    blacklist["Named_Unit"] = 0;
    blacklist["Organization"] = 0;
    blacklist["Person"] = 0;
    blacklist["Person_And_Organization"] = 0;
    blacklist["Person_And_Organization_Role"] = 0;
    blacklist["Plane_Angle_Measure_With_Unit"] = 0;
    blacklist["Presentation_Layer_Assignment"] = 0;
    blacklist["Presentation_Style_Assignment"] = 1;
    blacklist["Product"] = 1;
    blacklist["Product_Category"] = 0;
    blacklist["Product_Context"] = 1;
    blacklist["Product_Definition"] = 1;
    blacklist["Product_Definition_Context"] = 1;
    blacklist["Product_Definition_Formation_With_Specified_Source"] = 1; // revision belonging to a product
    blacklist["Product_Related_Product_Category"] = 0;
    blacklist["Representation_Context"] = 1;
    blacklist["Shape_Definition_Representation"] = 0;
    blacklist["Styled_Item"] = 1;
    blacklist["Surface_Side_Style"] = 1;
    blacklist["Surface_Style_Fill_Area"] = 1;
    blacklist["Surface_Style_Usage"] = 1;
    blacklist["Uncertainty_Measure_With_Unit"] = 1;
    }
}

template<class T> void VRSTEP::addType(string typeName, string path, bool print) {
    Type type;
    type.print = print;
    type.path = path;
    type.cb = VRFunction<STEPentity*>::create("STEPtypeCb", boost::bind( &VRSTEP::parse<T>, this, _1, path, typeName ));
    types[typeName] = type;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, string& t, char c) {
    if (c == 'S') if (a) if (auto r = a->String() ) { r->asStr(t); return true; }
    return false;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, int& t, char c) {
    if (c == 'i') if (a) if (auto r = a->Integer() ) { t = *r; return true; }
    return false;
}

bool getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, STEPentity*& t, char c) {
    if (c == 'e') {
        if (e) { t = e;
            cout << " getValue " << e << endl; return true; }
        if (an) { t = ((STEPentity*)((EntityNode*)an)->node); return true; }
    }
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

STEPentity* getSelectEntity(SDAI_Select* s) {
    string stype;
    s->UnderlyingTypeName().asStr(stype);

    switch(s->ValueType()) {
        case ENTITY_TYPE:
            if (stype == "Axis2_Placement_3d") {
                auto v = (SdaiAxis2_placement*)s;
                if (v->IsAxis2_placement_2d()) { SdaiAxis2_placement_2d* o = *v; return o; }
                if (v->IsAxis2_placement_3d()) { SdaiAxis2_placement_3d* o = *v; return o; }
            }
            if (stype == "Characterized_Product_Definition") {
                auto v = (SdaiCharacterized_product_definition*)s;
                if (v->IsProduct_definition()) { SdaiProduct_definition* o = *v; return o; }
                if (v->IsProduct_definition_relationship()) { SdaiProduct_definition_relationship* o = *v; return o; }
                //SdaiProduct_definition_relationship* o = *v; return o;
            }
            if (stype == "Item_Defined_Transformation") {
                auto v = (SdaiTransformation*)s;
                if (v->IsItem_defined_transformation()) { SdaiItem_defined_transformation* o = *v; return o; }
                if (v->IsFunctionally_defined_transformation()) { SdaiFunctionally_defined_transformation* o = *v; return o; }
                break;
            }
            cout << " Select entity not handled: " << stype << endl;
            break;
        default:
            cout << "Select type not handled: " << s->ValueType() << endl;
    }
    return 0;
}

template<typename T> bool query(STEPentity* e, string path, T& t) {
    auto toInt = [](char c) { return int(c-'0'); };

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
            int ai = toInt(path[i+1]);
            if (e->AttributeCount() <= ai) return false;
            curAttr = &e->attributes[ai];
            curAggr = 0;
        }

        if (c == 'A') {
            j = 2;
            if (!curAttr) continue;
            curAggr = curAttr->Aggregate();
            if (!curAggr) { cout << "VRSTEP::query " << i << " is not an Aggregate!\n"; return false; }
            if ('0' <= path[i+1] && path[i+1] <= '9') {
                int Ai = toInt(path[i+1]);
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

        if (c == 'c') {
            j = 2;
            if (!e->IsComplex()) continue;
            auto ce = ( (STEPcomplex*)e )->head;
            int ci = toInt(path[i+1]);
            for (int i=0; i<ci; i++) ce = ce->sc;
            e = ce;
            curAggrNode = 0;
            curAttr = 0;
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
    if (instanceByID.count(e)) return;
    auto t = new T();
    setup(*t, e, path);
    Instance i;
    i.data = t;
    i.ID = e->STEPfile_id;
    i.entity = e;
    i.type = type;
    instancesByType[type].push_back(i);
    instanceByID[e] = i;
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

void VRSTEP::traverseEntity(STEPentity* se, int lvl, STEPcomplex* cparent) {
    if (se->IsComplex()) {
        auto sc = ( (STEPcomplex*)se )->head;
        if (sc != cparent) {
            while(sc) {
                traverseEntity(sc, lvl, ( (STEPcomplex*)se )->head);
                sc = sc->sc;
            }
            return;
        }
    }

    string type = se->EntityName();
    STEPentity* ID = se;

    bool red = (type == "Advanced_Brep_Shape_Representation" ||
        type == "Shape_Definition_Representation" ||
        type == "Shape_Representation_Relationship" ||
        type == "Shape_Representation" );

    bool green = (type == "Axis2_Placement_3d" ||
                  type == "Item_Defined_Transformation");

    bool blue = (type == "Product_Definition_Relationship" ||
        type == "Product_Definition_Usage" ||
        type == "Assembly_Component_Usage" ||
        type == "Next_Assembly_Usage_Occurrence");

    if (red) cout << redBeg;
    if (green) cout << greenBeg;
    if (blue) cout << blueBeg;
    cout << indent(lvl) << "Entity " << ID << (se->IsComplex() ? " (C) " : "") << ": " << se->EntityName() << endl;
    if (red || green || blue) cout << colEnd;

    if (instanceByID.count(ID) && !types[type].print) return;
    if (types.count(type) && types[type].cb) { (*types[type].cb)(se); if (!types[type].print) return; }
    if (blacklist.count(type) && blacklist[type]) { blacklisted++; return; }
    //if (blacklist.count(type)) { blacklisted++; return; }

    STEPattribute* attr;
    se->ResetAttributes();
    while ( (attr = se->NextAttribute()) != NULL ) {
        cout << indent(lvl+1) << "A: " << attr->Name() << " : " << attr->asStr();
        if ( attr->Entity() && !attr->IsDerived()) { cout << endl; traverseEntity( attr->Entity(), lvl+2); }
        if ( auto a = attr->Aggregate() ) { cout << endl; traverseAggregate(a, attr->BaseType(), lvl+2); }
        if ( auto s = attr->Select() ) { cout << endl; traverseSelect(s, lvl+2); }
        if ( auto i = attr->Integer() ) cout << " Integer: " << *i << endl;
        if ( auto r = attr->Real() ) cout << " Real: " << *r << endl;
        if ( auto n = attr->Number() ) cout << " Number: " << *n << endl;
        if ( auto s = attr->String() ) { string ss; s->asStr(ss); cout << " String: " << ss << endl; }
        if ( auto b = attr->Binary() ) cout << " Binary: " << b << endl;
        if ( auto e = attr->Enum() ) cout << " Enum: " << *e << endl;
        if ( auto l = attr->Logical() ) cout << " Logical: " << *l << endl;
        if ( auto b = attr->Boolean() ) cout << " Boolean: " << *b << endl;
        //if ( auto u = attr->Undefined() ) cout << "Undefined: " << u << endl;
        //if ( attr->Type() == REFERENCE_TYPE ) cout << indent(lvl+1) << " ref: " << attr << " " << endl;
    }
}

//template<class S> void

void VRSTEP::traverseSelect(SDAI_Select* s, int lvl) {
    cout << indent(lvl) << "resolve select\n";
    auto e = getSelectEntity(s);
    if (e) traverseEntity(e, lvl+1);
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
                    case ENTITY_TYPE: traverseEntity(sse, lvl+2); break;
                    default: cout << indent(lvl+1) << "entity Type not handled:" << etype << endl;
                }
                break;
            case SELECT_TYPE: // 80
                sen = (SelectNode*)sn;
                sdsel = sen->node;
                traverseSelect(sdsel, lvl+2);
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

Vec3f toVec3f(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instanceByID) {
    if (!instanceByID.count(i)) { cout << "toVec3f FAILED with instance " << i << endl; return Vec3f(); }
    auto I = instanceByID[i];
    double L = 1.0;
    if (I.type == "Vertex_Point") I = instanceByID[ I.get<0, STEPentity*>() ];
    if (I.type == "Vector") {
        I = instanceByID[ I.get<0, STEPentity*, double>() ];
        L = I.get<1, STEPentity*, double>();
    }
    if (I.type == "Cartesian_Point" || I.type == "Direction") {
        return Vec3f(I.get<0, double, double, double>(),
                     I.get<1, double, double, double>(),
                     I.get<2, double, double, double>())*L;
    }
    cout << "toVec3f FAILED with instance type " << I.type << endl;
    return Vec3f();
}

pose toPose(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instanceByID) {
    auto I = instanceByID[i];
    if (I.type == "Axis2_Placement_3d") {
        Vec3f p = toVec3f( I.get<0, STEPentity*, STEPentity*, STEPentity*>(), instanceByID);
        Vec3f d = toVec3f( I.get<1, STEPentity*, STEPentity*, STEPentity*>(), instanceByID);
        Vec3f u = toVec3f( I.get<2, STEPentity*, STEPentity*, STEPentity*>(), instanceByID);
        return pose(p,d,u);
    }
    cout << "toPose FAILED with instance type " << I.type << endl;
    return pose();
}

struct VRSTEP::Bound : VRSTEP::Instance {
    vector<Vec3f> points;

    Bound() {}
    Bound(Instance& i, map<STEPentity*, Instance>& instanceByID) : Instance(i) {

        if (type == "Face_Bound" || type == "Face_Outer_Bound") {
            auto& Loop = instanceByID[ get<0, STEPentity*, bool>() ];
            bool dir = get<1, STEPentity*, bool>();
            for (auto l : Loop.get<0, vector<STEPentity*> >() ) {
                auto& Edge = instanceByID[l];
                if (Edge.type == "Oriented_Edge") {
                    auto& EdgeElement = instanceByID[ Edge.get<0, STEPentity*, bool>() ];
                    bool edir = Edge.get<1, STEPentity*, bool>();
                    if (EdgeElement.type == "Edge_Curve") {
                        Vec3f EBeg = toVec3f( EdgeElement.get<0, STEPentity*, STEPentity*, STEPentity*>(), instanceByID );
                        Vec3f EEnd = toVec3f( EdgeElement.get<1, STEPentity*, STEPentity*, STEPentity*>(), instanceByID );
                        auto& EdgeGeo = instanceByID[ EdgeElement.get<2, STEPentity*, STEPentity*, STEPentity*>() ];
                        if (EdgeGeo.type == "Line") {
                            Vec3f p = toVec3f( EdgeGeo.get<0, STEPentity*, STEPentity*>(), instanceByID );
                            Vec3f d = toVec3f( EdgeGeo.get<1, STEPentity*, STEPentity*>(), instanceByID );
                            points.push_back(EBeg); // TODO: check if beg and end are on the line!
                            points.push_back(EEnd);
                        }
                        if (EdgeGeo.type == "Circle") {
                            pose c = toPose( EdgeGeo.get<0, STEPentity*, double>(), instanceByID );
                            float r = EdgeGeo.get<1, STEPentity*, double>();
                        }
                    }
                }
            }
        }

    }
};

struct VRSTEP::Surface : VRSTEP::Instance {
    vector<Bound> bounds;
    pose trans;

    Surface(Instance& i, map<STEPentity*, Instance>& instanceByID) : Instance(i) {
        if (type == "Plane") {
            trans = toPose( get<0, STEPentity*>(), instanceByID);
        }

        if (type == "Cylindrical_Surface") {
            trans = toPose( get<0, STEPentity*, double>(), instanceByID );
            double R = get<1, STEPentity*, double>();
        }
    }

    VRGeometryPtr build() {
        Matrix m = trans.asMatrix();
        //m.invert();
        auto geo = VRGeometry::create("face");
        /*if (type == "Plane") {
            polygon poly;
            for (auto b : bounds) {
                for(auto p : b.poSTEPentity*s) {
                    m.mult(p,p);
                    poly.addPoSTEPentity*(Vec2f(p[0], p[1]));
                }
            }
            poly = poly.getConvexHull();
            poly;
        }*/

        GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
        GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();

        for (auto b : bounds) {
            for (int i=0; i<b.points.size(); i+=2) {
                Pnt3f p1 = b.points[i];
                Pnt3f p2 = b.points[i+1];
                pos->addValue(p1);
                pos->addValue(p2);
                norms->addValue(Vec3f(0,1,0));
                norms->addValue(Vec3f(0,1,0));
                inds->addValue(pos->size()-2);
                inds->addValue(pos->size()-1);
            }
        }

        geo->setType(GL_LINES);
        geo->setPositions(pos);
        geo->setNormals(norms);
        geo->setIndices(inds);

        VRMaterialPtr mat = VRMaterial::create("face");
        mat->setLit(0);
        mat->setLineWidth(3);
        geo->setMaterial(mat);
        return geo;
    }
};

void VRSTEP::build() {
    blacklisted = 0;

    for( int i=0; i<instances->InstanceCount(); i++) {
        STEPentity* se = instances->GetApplication_instance(i);
        //if (se == 1840) break;
        string name = se->EntityName();
        //if (name != "Shape_Representation_Relationship") continue;
        //if (name != "Circle") continue;
        //if (se == 1943) traverseAggregate((STEPaggregate*)se, ENTITY_TYPE, 1);
        traverseEntity(se,1);
    }

    /*for (auto p : instancesByType["Axis2_Placement_3d"]) {
        auto pp = toPose(p.ID);
        cout << p.ID << " " << pp.toString() << endl;
    }*/

    for (auto BrepShape : instancesByType["Advanced_Brep_Shape_Representation"]) {

        string name = BrepShape.get<0, string, vector<STEPentity*> >();
        auto geo = VRTransform::create(name);

        for (auto i : BrepShape.get<1, string, vector<STEPentity*> >() ) {
            auto& Item = instanceByID[i];
            if (Item.type == "Manifold_Solid_Brep") {
                auto& Outer = instanceByID[ Item.get<0, STEPentity*>() ];
                for (auto j : Outer.get<0, vector<STEPentity*> >() ) {
                    auto& Face = instanceByID[j];
                    if (Face.type == "Advanced_Face") {
                        auto& s = instanceByID[ Face.get<1, vector<STEPentity*>, STEPentity*, bool>() ];
                        Surface surface(s, instanceByID);
                        bool same_sense = Face.get<2, vector<STEPentity*>, STEPentity*, bool>();
                        for (auto k : Face.get<0, vector<STEPentity*>, STEPentity*, bool>() ) {
                            auto& b = instanceByID[k];
                            Bound bound(b, instanceByID);
                            surface.bounds.push_back(bound);
                        }
                        geo->addChild( surface.build() );
                    }
                }
            }
        }

        resObject[BrepShape.entity] = geo;
    }

    map<STEPentity*, STEPentity*> ABrepToSRep;
    for (auto ShapeRepRel : instancesByType["Shape_Representation_Relationship"]) {
        auto obj = ShapeRepRel.get<0, STEPentity*, STEPentity*>();
        auto rep = ShapeRepRel.get<1, STEPentity*, STEPentity*>();
        ABrepToSRep[rep] = obj;
    }

    // scene graph
    vector<Instance> assemblies;
    for (auto& i : instancesByType["Product_Definition_Relationship"]) assemblies.push_back(i);
    for (auto& i : instancesByType["Product_Definition_Usage"]) assemblies.push_back(i);
    for (auto& i : instancesByType["Assembly_Component_Usage"]) assemblies.push_back(i);
    for (auto& i : instancesByType["Next_Assembly_Usage_Occurrence"]) assemblies.push_back(i);

    for (auto& Assembly : assemblies) {
        auto& RelatingProduct = instanceByID[ Assembly.get<0, STEPentity*, STEPentity*>() ];
        auto& RelatedProduct = instanceByID[ Assembly.get<1, STEPentity*, STEPentity*>() ];
    }

    for (auto ShapeRep : instancesByType["Context_Dependent_Shape_Representation"]) {
        auto& Rep = instanceByID[ ShapeRep.get<0, STEPentity*, STEPentity*, STEPentity*>() ];
        auto& Shape1 = instanceByID[ Rep.get<0, STEPentity*, STEPentity*>() ];
        auto& Shape2 = instanceByID[ Rep.get<1, STEPentity*, STEPentity*>() ];

        auto& RepTrans = instanceByID[ ShapeRep.get<1, STEPentity*, STEPentity*, STEPentity*>() ];
        auto& ItemTrans = instanceByID[ RepTrans.get<0, STEPentity*>() ];
        auto pose1 = toPose( ItemTrans.get<0, STEPentity*, STEPentity*>(), instanceByID );
        auto pose2 = toPose( ItemTrans.get<1, STEPentity*, STEPentity*>(), instanceByID );

        if (ABrepToSRep.count(Shape1.entity)) {
            auto oe = ABrepToSRep[Shape1.entity];
            if (resObject.count(oe)) {
                /*Matrix m = pose1.asMatrix();
                m.invert();
                m.mult(pose2.asMatrix());
                resObject[oe]->setMatrix(m);*/
                resObject[oe]->setPose(pose2);
            }
        }
    }

    /*Entity 0x123150526300: Context_Dependent_Shape_Representation
        A: representation_relation : #1943
            Entity 0x12315051cee0 (C) : Representation_Relationship
            Entity 0x123150527000 (C) : Representation_Relationship_With_Transformation
                A: transformation_operator : #1939
                        Entity 0x123150523410: Item_Defined_Transformation
            Entity 0x1231505270e0 (C) : Shape_Representation_Relationship
        A: represented_product_relation : #1945
            Entity 0x1231505228c0: Product_Definition_Shape
                A: name : 'PDS_NAUO13' String: 'PDS_NAUO13'
                A: description : '' String: ''
                A: definition : #1944
                     Select entity not handled: Characterized_Product_Definition*/

    /*addType< tuple<STEPentity*, STEPentity*> >( "Context_Dependent_Shape_Representation", "a0e|a1e", false );
    addType< tuple<STEPentity*, STEPentity*> >( "Representation_Relationship", "a2e|a3e", false );
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Representation_Relationship_With_Transformation", "a2e|a3e|a4se", false );
    addType< tuple<STEPentity*, STEPentity*> >( "Item_Defined_Transformation", "a2e|a3e", false );

    addType< tuple<vector<STEPentity*>, STEPentity*> >( "Shape_Representation", "a1Ve|a2e", false );
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Representation_Relationship", "a2e|a3e", false );
*/
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

poSTEPentity*s -> Vec3f
vectors -> Vec3f
directions -> Vec3f
curves
surfaces

*/





