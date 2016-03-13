#include "VRSTEP.h"

#include <STEPfile.h>
#include <STEPcomplex.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/polygon.h"

#include "VRBRepEdge.h"
#include "VRBRepBound.h"
#include "VRBRepSurface.h"

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
    if (boost::filesystem::exists(file)) file = boost::filesystem::canonical(file).string();
    sfile->ReadExchangeFile(file);
    registry->ResetSchemas();
    registry->ResetEntities();
    *done = true;
}

VRSTEP::VRSTEP() {
    registry = RegistryPtr( new Registry( SchemaInit ) ); // schema
    instMgr = InstMgrPtr( new InstMgr() ); // instances
    sfile = STEPfilePtr( new STEPfile( *registry, *instMgr, "", false ) ); // file

    addType< tuple<STEPentity*, double> >( "Circle", "a1se|a2f", false);
    addType< tuple<double, double, double> >("Direction", "a1A0f|a1A1f|a1A2f", false);
    addType< tuple<double, double, double> >("Cartesian_Point", "a1A0f|a1A1f|a1A2f", false);
    addType< tuple<STEPentity*, bool> >( "Oriented_Edge", "a3e|a4b", false);
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Edge_Curve", "a1e|a2e|a3e", false);
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Axis2_Placement_3d", "a1e|a2e|a3e", false);
    addType< tuple<STEPentity*> >( "Manifold_Solid_Brep", "a1e", false);
    addType< tuple<STEPentity*> >( "Plane", "a1e", false);
    addType< tuple<STEPentity*, double> >( "Cylindrical_Surface", "a1e|a2f", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Line", "a1e|a2e", false);
    addType< tuple<STEPentity*, double> >( "Vector", "a1e|a2f", false);
    addType< tuple<STEPentity*> >( "Vertex_Point", "a1e", false);
    addType< tuple<STEPentity*, bool> >( "Face_Bound", "a1e|a2b", false);
    addType< tuple<STEPentity*, bool> >( "Face_Outer_Bound", "a1e|a2b", false);
    addType< tuple<vector<STEPentity*>, STEPentity*, bool> >( "Advanced_Face", "a1Ve|a2e|a3b", false);
    addType< tuple<vector<STEPentity*> > >( "Closed_Shell", "a1Ve", false);
    addType< tuple<vector<STEPentity*> > >( "Edge_Loop", "a1Ve", false);
    addType< tuple<int, vector<STEPentity*>, bool, bool > >( "B_Spline_Curve", "a0i|a1Ve|a3b|a4b", false);
    addType< tuple<vector<int>, vector<double> > >( "B_Spline_Curve_With_Knots", "a0Vi|a1Vf", false);
    addType< tuple<vector<double> > >( "Rational_B_Spline_Curve", "a0Vf", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Pcurve", "a1e|a2e", false);
    addType< tuple<vector<STEPentity*> > >( "Definitional_Representation", "a1Ve", false);
    addType< tuple<STEPentity*, vector<STEPentity*> > >( "Surface_Curve", "a1e|a2Ve", false);

    addType< tuple<int, int, vector<STEPentity*>, bool, bool, bool > >( "B_Spline_Surface", "a0i|a1i|a2Ve|a4b|a5b|a6b", false);
    addType< tuple< vector<int>, vector<int>, vector<double>, vector<double> > >( "B_Spline_Surface_With_Knots", "a0Vi|a1Vi|a2Vf|a2Vf", false);
    addType< tuple< vector<double> > >( "Rational_B_Spline_Surface", "a0Vf", false);

    // geometry types
    addType< tuple<string, vector<STEPentity*> > >( "Advanced_Brep_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Compound_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Csg_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Curve_Swept_Solid_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Direction_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Edge_Based_Wireframe_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Face_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Faceted_Brep_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_2d_Wireframe_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_Surface_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_Wireframe_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Location_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Manifold_Subsurface_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Manifold_Surface_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Non_Manifold_Surface_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Path_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Planar_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Point_Placement_Shape_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Shape_Dimension_Representation", "a0S|a1Ve", false);
    addType< tuple<string, vector<STEPentity*> > >( "Shape_Representation_With_Parameters", "a0S|a1Ve", false);

    // assembly entities
    addType< tuple<string, STEPentity*, STEPentity*> >( "Product_Definition_Relationship", "a3e|a4e", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Product_Definition_Usage", "a3e|a4e", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Assembly_Component_Usage", "a3e|a4e", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Next_Assembly_Usage_Occurrence", "a1S|a3e|a4e", false);

    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Context_Dependent_Shape_Representation", "a0ec0e|a0ec1e|a1e", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Representation_Relationship", "a2e|a3e", false);
    addType< tuple<STEPentity*> >( "Representation_Relationship_With_Transformation", "a0se", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Item_Defined_Transformation", "a2e|a3e", false);

    addType< tuple<vector<STEPentity*>, STEPentity*> >( "Shape_Representation", "a1Ve|a2e", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Representation_Relationship", "a2e|a3e", false);
    //addType< tuple<STEPentity*, STEPentity*> >( "Geometric_Representation_Context", "a2e|a3e" );

    addType< tuple<string, string> >( "Product", "a0S|a1S", false);
    addType< tuple<STEPentity*> >( "Product_Definition", "a2e", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Shape", "a2se", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Formation", "a2e", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Formation_With_Specified_Source", "a2e", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Definition_Representation", "a0se|a1e", false);

    {
    blacklist["Colour_Rgb"] = 1;
    blacklist["Bounded_Curve"] = 1;
    blacklist["Curve"] = 1;
    blacklist["Geometric_Representation_Item"] = 1;
    blacklist["Parametric_Representation_Context"] = 1;
    blacklist["Surface"] = 1;
    blacklist["Bounded_Surface"] = 1;
    blacklist["Plane_Angle_Unit"] = 1;
    blacklist["Solid_Angle_Unit"] = 1;

    blacklist["Application_Context"] = 1;
    blacklist["Application_Protocol_Definition"] = 1;
    blacklist["Applied_Person_And_Organization_Assignment"] = 1;
    blacklist["Conversion_Based_Unit"] = 1;
    blacklist["Derived_Unit"] = 1;
    blacklist["Derived_Unit_Element"] = 1;
    blacklist["Dimensional_Exponents"] = 1;
    blacklist["Draughting_Pre_Defined_Colour"] = 1;
    blacklist["Fill_Area_Style"] = 1;
    blacklist["Fill_Area_Style_Colour"] = 1;
    blacklist["Geometric_Representation_Context"] = 1;
    blacklist["Global_Uncertainty_Assigned_Context"] = 1;
    blacklist["Global_Unit_Assigned_Context"] = 1;
    blacklist["Length_Unit"] = 1;
    blacklist["Measure_Representation_Item"] = 1;
    blacklist["Mechanical_Design_Geometric_Presentation_Representation"] = 1;
    blacklist["Named_Unit"] = 1;
    blacklist["Organization"] = 1;
    blacklist["Person"] = 1;
    blacklist["Person_And_Organization"] = 1;
    blacklist["Person_And_Organization_Role"] = 1;
    blacklist["Plane_Angle_Measure_With_Unit"] = 1;
    blacklist["Presentation_Layer_Assignment"] = 1;
    blacklist["Presentation_Style_Assignment"] = 1;
    blacklist["Product_Category"] = 1;
    blacklist["Product_Context"] = 1;
    //blacklist["Product_Definition_Formation"] = 1;
    blacklist["Product_Definition_Context"] = 1;
    blacklist["Product_Related_Product_Category"] = 1;
    blacklist["Property_Definition"] = 1;
    blacklist["Property_Definition_Representation"] = 1;
    blacklist["Representation"] = 1;
    blacklist["Representation_Context"] = 1;
    blacklist["Representation_Item"] = 1;
    blacklist["Si_Unit"] = 1;
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
        if (e) { t = e; return true; }
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

STEPentity* VRSTEP::getSelectEntity(SDAI_Select* s, string ID) {
    if (s->ValueType() == ENTITY_TYPE && ID[0] == '#') {
        int id = toInt(ID.substr(1));
        if (instancesById.count(id)) return instancesById[id].entity;
        else cout << "getSelectEntity ID " << id << " not found\n";
    }

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
            }
            if (stype == "Derived_Unit") {
                auto v = (SdaiUnit*)s;
                if (v->IsDerived_unit()) { SdaiDerived_unit* o = *v; return o; }
                if (v->IsNamed_unit()) { SdaiNamed_unit* o = *v; return o; }
            }
            if (stype == "Property_Definition") {
                auto v = (SdaiProperty_or_shape_select*)s;
                if (v->IsProperty_definition()) { SdaiProperty_definition* o = *v; return o; }
                if (v->IsShape_definition()) {
                    SdaiShape_definition* v2 = *v;
                    if (v2->IsProduct_definition_shape()) { SdaiProduct_definition_shape* o = *v2; return o; }
                    if (v2->IsShape_aspect()) { SdaiShape_aspect* o = *v2; return o; }
                    if (v2->IsShape_aspect_relationship()) { SdaiShape_aspect_relationship* o = *v2; return o; }
                }
            }
            if (stype == "Characterized_Product_Definition") {
                auto v = (SdaiCharacterized_definition*)s;
                if (v->IsCharacterized_object()) { SdaiCharacterized_object* o = *v; return o; }
                if (v->IsCharacterized_product_definition()) {
                    SdaiCharacterized_product_definition* v2 = *v;
                    if (v2->IsProduct_definition()) { SdaiProduct_definition* o = *v2; return o; }
                    if (v2->IsProduct_definition_relationship()) { SdaiProduct_definition_relationship* o = *v2; return o; }
                }
                if (v->IsShape_definition()) {
                    SdaiShape_definition* v2 = *v;
                    if (v2->IsProduct_definition_shape()) { SdaiProduct_definition_shape* o = *v2; return o; }
                    if (v2->IsShape_aspect()) { SdaiShape_aspect* o = *v2; return o; }
                    if (v2->IsShape_aspect_relationship()) { SdaiShape_aspect_relationship* o = *v2; return o; }
                }
            }
            if (stype == "Pcurve") {
                auto v = (SdaiPcurve_or_surface*)s;
                if (v->IsPcurve()) { SdaiPcurve* o = *v; return o; }
                if (v->IsSurface()) { SdaiSurface* o = *v; return o; }
            }
            cout << " Select entity not handled: " << stype << endl;
            break;
        case AGGREGATE_TYPE:
            cout << " Select aggregate not handled: " << stype << endl;
            break;
        case INTEGER_TYPE:
            cout << " Select integer not handled: " << stype << endl;
            break;
        case REAL_TYPE:
            cout << " Select real not handled: " << stype << endl;
            break;
        case BOOLEAN_TYPE:
            cout << " Select boolean not handled: " << stype << endl;
            break;
        case LOGICAL_TYPE:
            cout << " Select logical not handled: " << stype << endl;
            break;
        case STRING_TYPE:
            cout << " Select string not handled: " << stype << endl;
            break;
        case BINARY_TYPE:
            cout << " Select binary not handled: " << stype << endl;
            break;
        case ENUM_TYPE:
            cout << " Select enum not handled: " << stype << endl;
            break;
        case SELECT_TYPE:
            cout << " Select select not handled: " << stype << endl;
            break;
        case NUMBER_TYPE:
            cout << " Select number not handled: " << stype << endl;
            break;
        default:
            cout << "Select type not handled: " << s->ValueType() << endl;
    }
    return 0;
}

template<typename T> bool VRSTEP::query(STEPentity* e, string path, T& t) {
    auto toInt = [](char c) { return int(c-'0'); };

    int j = 1;
    STEPattribute* curAttr = 0;
    STEPaggregate* curAggr = 0;
    SDAI_Select* curSel = 0;
    SingleLinkNode* curAggrNode = 0;
    string attrStr;
    for (int i=0; i<path.size(); i+=j) {
        bool isLast = (i == path.size()-1);
        j = 1;
        auto c = path[i];

        if (c == 'a') {
            j = 2;
            int ai = toInt(path[i+1]);
            if (e->AttributeCount() <= ai) { cout << "VRSTEP::query " << ai << " is out of range!\n"; return false; }
            curAttr = &e->attributes[ai];
            attrStr = curAttr->asStr();
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
                e = getSelectEntity(curSel, attrStr);
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
    static void setup(T& t, STEPentity* e, vector<string>& paths, VRSTEP* step) {
        Setup<T, N-1>::setup(t, e, paths, step);
        step->query(e, paths[N-1], get<N-1>(t));
    }
};

template<class T> struct Setup<T, 1> {
    static void setup(T& t, STEPentity* e, vector<string>& paths, VRSTEP* step) {
        step->query(e, paths[0], get<0>(t));
    }
};

template<class... Args> void setup(tuple<Args...>& t, STEPentity* e, string paths, VRSTEP* step) {
    auto vpaths = splitString(paths, '|');
    Setup<decltype(t), sizeof...(Args)>::setup(t, e, vpaths, step);
}
// end helper function

template<class T> void VRSTEP::parse(STEPentity* e, string path, string type) {
    if (instances.count(e)) return;
    auto t = new T();
    setup(*t, e, path, this);
    Instance i;
    i.data = t;
    i.ID = e->STEPfile_id;
    i.entity = e;
    i.type = type;
    instancesById[i.ID] = i;
    instancesByType[type].push_back(i);
    instances[e] = i;
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

    /*bool red = (type == "Advanced_Brep_Shape_Representation" ||
        type == "Shape_Definition_Representation" ||
        type == "Shape_Representation_Relationship" ||
        type == "Shape_Representation" );

    bool green = (type == "Axis2_Placement_3d" ||
                  type == "Item_Defined_Transformation");

    bool blue = (type == "Product_Definition_Relationship" ||
        type == "Product_Definition_Usage" ||
        type == "Assembly_Component_Usage" ||
        type == "Next_Assembly_Usage_Occurrence");

    //if (red) cout << redBeg;
    //if (green) cout << greenBeg;
    //if (blue) cout << blueBeg;
    cout << indent(lvl) << "Entity " << se->STEPfile_id << (se->IsComplex() ? " (C) " : "") << ": " << string(se->EntityName()) << endl;
    //if (red || green || blue) cout << colEnd;*/

    if (!types.count(type) && !blacklist.count(type))
        cout << indent(lvl) << "Entity " << se->STEPfile_id << (se->IsComplex() ? " (C) " : "") << ": " << string(se->EntityName()) << endl;

    bool printAll = false;
    if (instances.count(se) && !types[type].print && !printAll) return;
    if (types.count(type) && types[type].cb) { (*types[type].cb)(se); if (!types[type].print && !printAll) return; }
    if (blacklist.count(type) && blacklist[type] && !printAll) { blacklisted++; return; }
    //if (blacklist.count(type)) { blacklisted++; return; }

    STEPattribute* attr;
    se->ResetAttributes();
    while ( (attr = se->NextAttribute()) != NULL ) {
        cout << indent(lvl+1) << "A: " << string(attr->Name()) << " : " << string(attr->asStr());
        if ( attr->Entity() && !attr->IsDerived()) { cout << endl; traverseEntity( attr->Entity(), lvl+2); }
        if ( auto a = attr->Aggregate() ) { cout << endl; traverseAggregate(a, attr->BaseType(), lvl+2); }
        if ( auto s = attr->Select() ) { cout << endl; traverseSelect(s, attr->asStr(), lvl+2); }
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

void VRSTEP::traverseSelect(SDAI_Select* s, string ID, int lvl) {
    cout << indent(lvl) << "resolve select\n";
    auto e = getSelectEntity(s, ID);
    if (e) traverseEntity(e, lvl+1);
}

void VRSTEP::traverseAggregate(STEPaggregate *sa, int atype, int lvl) {
    string s; sa->asStr(s);
    cout << indent(lvl) << "Aggregate: " << s << endl;

    STEPentity* sse;
    SelectNode* sen;
    SDAI_Select* sdsel;
    PrimitiveType etype, ebtype;
    const EntityDescriptor* ssedesc = 0;

    for( EntityNode* sn = (EntityNode*)sa->GetHead(); sn != NULL; sn = (EntityNode*)sn->NextNode()) {
        switch (atype) {
            case ENTITY_TYPE: // 100
                sse = (STEPentity*)sn->node;
                ssedesc = sse->getEDesc();
                if (ssedesc) {
                    etype = ssedesc->Type();
                    switch (etype) {
                        case SET_TYPE:
                        case LIST_TYPE:
                            ebtype = ssedesc->BaseType();
                            traverseAggregate((STEPaggregate *)sse, ebtype, lvl+2); break;
                        case ENTITY_TYPE: traverseEntity(sse, lvl+2); break;
                        default: cout << indent(lvl+1) << "entity Type not handled:" << etype << endl;
                    }
                }
                break;
            case SELECT_TYPE: // 80
                sen = (SelectNode*)sn;
                sdsel = sen->node;
                sen->asStr(s);
                traverseSelect(sdsel, s, lvl+2);
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

Vec3f toVec3f(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instances) {
    if (!instances.count(i)) { cout << "toVec3f FAILED with instance " << i << endl; return Vec3f(); }
    auto I = instances[i];
    double L = 1.0;
    bool isVec3f = false;

    if (I.type == "Vertex_Point") { I = instances[ I.get<0, STEPentity*>() ]; isVec3f = true; }

    if (I.type == "Vector") {
        L = I.get<1, STEPentity*, double>();
        I = instances[ I.get<0, STEPentity*, double>() ];
        isVec3f = true;
    }

    if (I.type == "Cartesian_Point" || I.type == "Direction") isVec3f = true;

    if (isVec3f) {
        auto x = I.get<0, double, double, double>(); if (abs(x) < 1e-14) x = 0;
        auto y = I.get<1, double, double, double>(); if (abs(y) < 1e-14) y = 0;
        auto z = I.get<2, double, double, double>(); if (abs(z) < 1e-14) z = 0;
        return Vec3f(y,x,-z)*L;
    }
    cout << "toVec3f FAILED with instance type " << I.type << endl;
    return Vec3f();
}

pose toPose(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instances) {
    auto I = instances[i];
    if (I.type == "Axis2_Placement_3d") {
        Vec3f p = toVec3f( I.get<0, STEPentity*, STEPentity*, STEPentity*>(), instances);
        Vec3f d = toVec3f( I.get<1, STEPentity*, STEPentity*, STEPentity*>(), instances);
        Vec3f u = toVec3f( I.get<2, STEPentity*, STEPentity*, STEPentity*>(), instances);
        //d[2] *= -1;
        return pose(p,d,u);
        //return pose(p,d,u);
    }
    cout << "toPose FAILED with instance type " << I.type << endl;
    return pose();
}

struct VRSTEP::Edge : public VRSTEP::Instance, public VRBRepEdge {
    Edge(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        type = i.type;
        if (type == "Oriented_Edge") {
            auto& EdgeElement = instances[ i.get<0, STEPentity*, bool>() ];
            bool edir = i.get<1, STEPentity*, bool>();
            if (EdgeElement.type == "Edge_Curve") {
                Vec3f EBeg = toVec3f( EdgeElement.get<0, STEPentity*, STEPentity*, STEPentity*>(), instances );
                Vec3f EEnd = toVec3f( EdgeElement.get<1, STEPentity*, STEPentity*, STEPentity*>(), instances );
                auto& EdgeGeo = instances[ EdgeElement.get<2, STEPentity*, STEPentity*, STEPentity*>() ];
                int Np = points.size();

                if (EdgeGeo.type == "Line") {
                    //Vec3f p = toVec3f( EdgeGeo.get<0, STEPentity*, STEPentity*>(), instances );
                    //Vec3f d = toVec3f( EdgeGeo.get<1, STEPentity*, STEPentity*>(), instances );
                    points.push_back(EBeg);
                    points.push_back(EEnd);
                }

                if (EdgeGeo.type == "Circle") {
                    pose c = toPose( EdgeGeo.get<0, STEPentity*, double>(), instances );
                    float r = EdgeGeo.get<1, STEPentity*, double>();
                    float _r = 1/r;
                    Matrix m = c.asMatrix();
                    Matrix mI = m; mI.invert();

                    float a1,a2; // get start and end angles
                    Vec3f c1,c2;
                    mI.mult(Pnt3f(EBeg), c1);
                    mI.mult(Pnt3f(EEnd), c2);
                    c1 *= _r; c2*= _r;
                    a1 = atan2(c1[1],c1[0]);
                    a2 = atan2(c2[1],c2[0]);

                    for (auto a : angleFrame(a1, a2)) {
                        Pnt3f p(r*cos(a),r*sin(a),0);
                        m.mult(p,p);
                        points.push_back(Vec3f(p));
                    }
                }
            }
        }
    }
};

struct VRSTEP::Bound : public VRSTEP::Instance, public VRBRepBound {
    Bound(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        if (type != "Face_Outer_Bound") outer = false;
        if (type == "Face_Bound" || type == "Face_Outer_Bound") {
            auto& Loop = instances[ get<0, STEPentity*, bool>() ];
            bool dir = get<1, STEPentity*, bool>();
            for (auto l : Loop.get<0, vector<STEPentity*> >() ) {
                Edge edge(instances[l], instances);
                edges.push_back(edge);
            }
        }

        if (edges.size() <= 1) return; // done

        if ( sameVec(edges[0].beg(), edges[1].beg()) || sameVec(edges[0].beg(), edges[1].end()) ) edges[0].swap(); // swap first edge

        for (int i=1; i<edges.size(); i++) {
            auto& e1 = edges[i-1];
            auto& e2 = edges[i];
            if ( sameVec(e2.end(), e1.end()) ) e2.swap();
        }

        for (auto& e : edges) {
            for (auto& p : e.points) {
                if (points.size() > 0) {
                    if (sameVec(p, points[points.size()-1])) continue; // same as last point
                    if (sameVec(p, points[0])) continue; // same as first point
                }
                points.push_back(p);
            }
        }
    }
};

struct VRSTEP::Surface : public VRSTEP::Instance, public VRBRepSurface {
    Surface(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        if (type == "Plane") trans = toPose( get<0, STEPentity*>(), instances);
        if (type == "Cylindrical_Surface") {
            trans = toPose( get<0, STEPentity*, double>(), instances );
            R = get<1, STEPentity*, double>();
        }
    }
};

void VRSTEP::buildGeometries() {
    for (auto BrepShape : instancesByType["Advanced_Brep_Shape_Representation"]) {

        string name = BrepShape.get<0, string, vector<STEPentity*> >();
        auto geo = VRGeometry::create(name);

        for (auto i : BrepShape.get<1, string, vector<STEPentity*> >() ) {
            auto& Item = instances[i];
            if (Item.type == "Manifold_Solid_Brep") {
                auto& Outer = instances[ Item.get<0, STEPentity*>() ];
                for (auto j : Outer.get<0, vector<STEPentity*> >() ) {
                    auto& Face = instances[j];
                    if (Face.type == "Advanced_Face") {
                        auto& s = instances[ Face.get<1, vector<STEPentity*>, STEPentity*, bool>() ];
                        Surface surface(s, instances);
                        bool same_sense = Face.get<2, vector<STEPentity*>, STEPentity*, bool>();
                        for (auto k : Face.get<0, vector<STEPentity*>, STEPentity*, bool>() ) {
                            auto& b = instances[k];
                            Bound bound(b, instances);
                            surface.bounds.push_back(bound);
                        }
                        //static int ii = 0; ii++;
                        //if (ii < 17) continue;
                        geo->merge( surface.build(surface.type) );
                        //geo->addChild( surface.build(surface.type) );
                        //if (ii >= 17) break;
                    }
                }
                //break;
            }
        }

        resGeos[BrepShape.entity] = geo;
        //break;
    }
}

VRSTEP::Instance& VRSTEP::getInstance(STEPentity* e) {
    static Instance invalid;
    if (e == 0) return invalid;
    if (!instances.count(e)) return invalid;
    return instances[e];
}

void VRSTEP::buildScenegraph() {
    cout << "VRSTEP::buildScenegraph start\n";

    // get geometries -------------------------------------
    map<STEPentity*, STEPentity*> SRepToABrep;
    for (auto ShapeRepRel : instancesByType["Shape_Representation_Relationship"]) {
        auto ABrep = ShapeRepRel.get<0, STEPentity*, STEPentity*>();
        auto SRep = ShapeRepRel.get<1, STEPentity*, STEPentity*>();
        cout << "AA " << ABrep << " " << SRep << endl;
        if (!ABrep || !SRep) { /*cout << "VRSTEP::buildScenegraph Warning 1\n" ;*/ continue; } // empty one
        SRepToABrep[SRep] = ABrep;
    }

    cout << "VRSTEP::buildScenegraph SRepToABrep " << SRepToABrep.size() << endl;

    map<STEPentity*, STEPentity*> ProductToSRep;
    for (auto ShapeRepRel : instancesByType["Shape_Definition_Representation"]) {
        if (!ShapeRepRel) { cout << "VRSTEP::buildScenegraph Error 2\n" ; continue; }
        auto& PDS = getInstance( ShapeRepRel.get<0, STEPentity*, STEPentity*>() );
        if (!PDS) { cout << "VRSTEP::buildScenegraph Error 3\n" ; continue; }
        auto& PDef = getInstance( PDS.get<0, STEPentity*>() );
        if (!PDef) { cout << "VRSTEP::buildScenegraph Error 4\n" ; continue; }
        auto& PDF = getInstance( PDef.get<0, STEPentity*>() );
        if (!PDF) { cout << "VRSTEP::buildScenegraph Error 5\n" ; continue; }
        auto& Product = getInstance( PDF.get<0, STEPentity*>() );
        if (!Product) { cout << "VRSTEP::buildScenegraph Error 6\n" ; continue; }
        auto SRep = ShapeRepRel.get<1, STEPentity*, STEPentity*>();
        ProductToSRep[Product.entity] = SRep;
    }

    cout << "VRSTEP::buildScenegraph ProductToSRep " << ProductToSRep.size() << endl;

    // get product definitions -------------------------------------
    resRoot = VRTransform::create("STEPRoot");
    VRObjectPtr root;
    map<string, VRTransformPtr> objs;

    for (auto PDefShape : instancesByType["Product_Definition_Shape"]) {
        if (!PDefShape) { cout << "VRSTEP::buildScenegraph Error 7\n" ; continue; }
        auto& Def = getInstance( PDefShape.get<0, STEPentity*>() );
        if (!Def) { cout << "VRSTEP::buildScenegraph Error 8\n" ; continue; }

        if (Def.type == "Product_Definition") {
            auto& PDF = getInstance( Def.get<0, STEPentity*>() );
            if (!PDF) { cout << "VRSTEP::buildScenegraph Error 9\n" ; continue; }
            auto& Product = getInstance( PDF.get<0, STEPentity*>() );
            if (!Product) { cout << "VRSTEP::buildScenegraph Error 10\n" ; continue; }
            string name = Product.get<0, string, string>();
            STEPentity *srep, *brep; srep = brep = 0;
            if (ProductToSRep.count(Product.entity)) srep = ProductToSRep[Product.entity];
            if (SRepToABrep.count(srep)) brep = SRepToABrep[srep];
            VRTransformPtr o;
            if (resGeos.count(brep)) o = resGeos[brep];
            else o = VRTransform::create(name);
            objs[name] = o;
            if (!root) root = o;
        }
    }

    cout << "VRSTEP::buildScenegraph objs " << objs.size() << endl;
    resRoot->addChild(root);

    // build scene graph and set transforms ----------------------------------------
    for (auto ShapeRep : instancesByType["Context_Dependent_Shape_Representation"]) {
        if (!ShapeRep) { cout << "VRSTEP::buildScenegraph Error 11\n" ; continue; }
        auto& Rep = getInstance( ShapeRep.get<0, STEPentity*, STEPentity*, STEPentity*>() );
        if (!Rep) { cout << "VRSTEP::buildScenegraph Error 12\n" ; continue; }
        auto& Shape1 = getInstance( Rep.get<0, STEPentity*, STEPentity*>() );
        auto& Shape2 = getInstance( Rep.get<1, STEPentity*, STEPentity*>() );
        if (!Shape1 || !Shape2) { cout << "VRSTEP::buildScenegraph Error 13\n" ; continue; }

        auto& RepTrans = getInstance( ShapeRep.get<1, STEPentity*, STEPentity*, STEPentity*>() );
        if (!RepTrans) { cout << "VRSTEP::buildScenegraph Error 14\n" ; continue; }
        auto& ItemTrans = getInstance( RepTrans.get<0, STEPentity*>() );
        if (!ItemTrans) { cout << "VRSTEP::buildScenegraph Error 15\n" ; continue; }
        auto pose1 = toPose( ItemTrans.get<0, STEPentity*, STEPentity*>(), instances );
        auto pose2 = toPose( ItemTrans.get<1, STEPentity*, STEPentity*>(), instances );

        auto& PDef = getInstance( ShapeRep.get<2, STEPentity*, STEPentity*, STEPentity*>() );
        if (!PDef) { cout << "VRSTEP::buildScenegraph Error 16\n" ; continue; }
        auto& Assembly = getInstance( PDef.get<0, STEPentity*>() );
        if (!Assembly) { cout << "VRSTEP::buildScenegraph Error 17\n" ; continue; }
        string name  = Assembly.get<0, string, STEPentity*, STEPentity*>();

        auto& Relating = getInstance( Assembly.get<1, string, STEPentity*, STEPentity*>() );
        auto& Related  = getInstance( Assembly.get<2, string, STEPentity*, STEPentity*>() );
        if (!Relating || !Related) { cout << "VRSTEP::buildScenegraph Error 18\n" ; continue; }
        auto& PDF1 = getInstance( Relating.get<0, STEPentity*>() );
        auto& PDF2 = getInstance( Related.get<0, STEPentity*>() );
        if (!PDF1 || !PDF2) { cout << "VRSTEP::buildScenegraph Error 19\n" ; continue; }
        auto& Product1 = getInstance( PDF1.get<0, STEPentity*>() );
        auto& Product2 = getInstance( PDF2.get<0, STEPentity*>() );
        if (!Product1 || !Product2) { cout << "VRSTEP::buildScenegraph Error 20\n" ; continue; }

        string parent = Product1.get<0, string, string>();
        string obj = Product2.get<0, string, string>();

        VRTransformPtr o = dynamic_pointer_cast<VRTransform>( objs[obj]->duplicate() );
        o->setName(name);
        objs[parent]->addChild(o);
        objs[name] = o;
        objs[name]->setPose(pose2);
    }

    cout << "VRSTEP::buildScenegraph objs " << objs.size() << endl;
    cout << "VRSTEP::buildScenegraph finished\n";
}

void VRSTEP::build() {
    blacklisted = 0;

    for( int i=0; i<instMgr->InstanceCount(); i++) {
        STEPentity* se = instMgr->GetApplication_instance(i);
        //if (se == 1840) break;
        string name = se->EntityName();
        //if (name != "Product_Definition_Shape") continue;
        //if (name != "Cartesian_Point" && name != "Direction") continue;
        //if (se == 1943) traverseAggregate((STEPaggregate*)se, ENTITY_TYPE, 1);
        traverseEntity(se,0);
    }

    buildGeometries();
    buildScenegraph();

    cout << "build results:\n";
    cout << instances.size() << " STEP entities parsed\n";
    cout << blacklisted << " STEP blacklisted entities ignored\n";
    cout << resGeos.size() << " VR objects created\n";
}

VRTransformPtr VRSTEP::load(string file) {
    open(file);
    build();
    return resRoot;
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





