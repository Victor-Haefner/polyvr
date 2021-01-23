#include "VRSTEP.h"

#include <STEPfile.h>
#include <STEPcomplex.h>
#include <STEPaggregate.h>
#include <schema.h>

#include <thread>
#include <unistd.h>
#include <memory>
#include <algorithm>
#include "core/utils/system/VRSystem.h"

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/polygon.h"
#include "core/math/pose.h"
//#include "core/gui/VRGuiTreeExplorer.h"

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

//VRGuiTreeExplorerPtr explorer;

void VRSTEP::loadT(string file, STEPfilePtr sfile, bool* done) {
    if (exists(file)) file = canonical(file);
    sfile->ReadExchangeFile(file);
    registry->ResetSchemas();
    registry->ResetEntities();
    *done = true;
}

VRSTEP::VRSTEP() {
    registry = RegistryPtr( new Registry( SchemaInit ) ); // schema
    instMgr = InstMgrPtr( new InstMgr() ); // instances
    sfile = STEPfilePtr( new STEPfile( *registry, *instMgr, "", false ) ); // file

    addType< tuple<STEPentity*, double> >( "Circle", "a1se|a2f", "", false);
    addType< tuple<double, double, double> >("Direction", "a1A0f|a1A1f|a1A2f", "", false);
    addType< tuple<double, double, double> >("Cartesian_Point", "a1A0f|a1A1f|a1A2f", "", false);
    addType< tuple<STEPentity*, bool> >( "Oriented_Edge", "a3e|a4b", "", false);
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Edge_Curve", "a1e|a2e|a3e", "", false);
    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Axis2_Placement_3d", "a1e|a2e|a3e", "", false);
    addType< tuple<STEPentity*> >( "Manifold_Solid_Brep", "a1e", "", false);
    addType< tuple<STEPentity*> >( "Plane", "a1e", "", false);
    addType< tuple<STEPentity*, double> >( "Cylindrical_Surface", "a1e|a2f", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Line", "a1e|a2e", "", false);
    addType< tuple<STEPentity*, double> >( "Vector", "a1e|a2f", "", false);
    addType< tuple<STEPentity*> >( "Vertex_Point", "a1e", "", false);
    addType< tuple<STEPentity*, bool> >( "Face_Bound", "a1e|a2b", "", false);
    addType< tuple<STEPentity*, bool> >( "Face_Outer_Bound", "a1e|a2b", "", false);
    addType< tuple<vector<STEPentity*>, STEPentity*, bool> >( "Advanced_Face", "a1Ve|a2e|a3b", "", false);
    addType< tuple<vector<STEPentity*> > >( "Closed_Shell", "a1Ve", "", false);
    addType< tuple<vector<STEPentity*> > >( "Edge_Loop", "a1Ve", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Pcurve", "a1e|a2e", "", false);
    addType< tuple<vector<STEPentity*> > >( "Definitional_Representation", "a1Ve", "", false);

    addType< tuple<STEPentity*, vector<STEPentity*> > >( "Surface_Curve", "a1e|a2Ve", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "PCurve", "a1e|a2e", "", false);
    addType< tuple<STEPentity*> >( "Definitional_Representation", "a1e", "", false);
    addType< tuple<int, vector<STEPentity*>, bool> >( "B_Spline_Curve", "a1i|a2Ve|a4b", "n", false); // TODO
    addType< tuple<int, vector<STEPentity*>, bool, vector<double> > >( "Rational_B_Spline_Curve", "a1i|a2Ve|a4b|a6Vf", "c0a0i|c0a1Ve|c0a3b|a0Vf", false); // TODO
    addType< tuple<int, vector<STEPentity*>, bool, vector<int>, vector<double> > >( "B_Spline_Curve_With_Knots", "a1i|a2Ve|a4b|a6Vi|a7Vf", "c0a0i|c0a1Ve|c0a3b|c1a0Vi|c1a1Vf", false); //TODO

    addType< tuple<int, int, field<STEPentity*>, bool, bool, bool > >( "B_Spline_Surface", "a0i|a1i|a2Fe|a4b|a5b|a6b", "a0i|a1i|a2Fe|a4b|a5b|a6b", false);

    addType< tuple<int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> > >( "B_Spline_Surface_With_Knots",
        "a0i|a1i|a2Fe|a4b|a5b|a6b|a7Vi|a8Vi|a9Vf|a10Vf",
        "c0a0i|c0a1i|c0a2Fe|c0a4b|c0a5b|c0a6b|c1a0Vi|c1a1Vi|c1a2Vf|c1a3Vf", false);
    //addType< tuple<int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> > >( "B_Spline_Surface_With_Knots", "a0i|a1i|a2Fe|a4b|a5b|a6b|a7Vi|a8Vi|a9Vf|a10Vf", "c1a0i|c1a1i|c1a2Fe|c1a4b|c1a5b|c1a6b|c2a0Vi|c2a1Vi|c2a2Vf|c2a3Vf", false);

    addType< tuple< field<double> > >( "Rational_B_Spline_Surface", "a0Ff", "a0Ff", false);

    // geometry types
    addType< tuple<string, vector<STEPentity*> > >( "Advanced_Brep_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Compound_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Csg_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Curve_Swept_Solid_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Direction_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Edge_Based_Wireframe_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Face_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Faceted_Brep_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_2d_Wireframe_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_Surface_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Geometrically_Bounded_Wireframe_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Location_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Manifold_Subsurface_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Manifold_Surface_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Non_Manifold_Surface_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Path_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Planar_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Point_Placement_Shape_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Shape_Dimension_Representation", "a0S|a1Ve", "", false);
    addType< tuple<string, vector<STEPentity*> > >( "Shape_Representation_With_Parameters", "a0S|a1Ve", "", false);

    // assembly entities (scene graph)
    addType< tuple<string, STEPentity*, STEPentity*> >( "Product_Definition_Relationship", "a3e|a4e", "", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Product_Definition_Usage", "a3e|a4e", "", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Assembly_Component_Usage", "a3e|a4e", "", false);
    addType< tuple<string, STEPentity*, STEPentity*> >( "Next_Assembly_Usage_Occurrence", "a1S|a3e|a4e", "", false);

    addType< tuple<STEPentity*, STEPentity*, STEPentity*> >( "Context_Dependent_Shape_Representation", "a0ec0e|a0ec1e|a1e", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Representation_Relationship", "a2e|a3e", "a2e|a3e", false);
    addType< tuple<STEPentity*> >( "Representation_Relationship_With_Transformation", "a4se", "a0se", false); // a0se
    addType< tuple<STEPentity*, STEPentity*> >( "Item_Defined_Transformation", "a2e|a3e", "", false);

    addType< tuple<vector<STEPentity*>, STEPentity*> >( "Shape_Representation", "a1Ve|a2e", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Representation_Relationship", "a2e|a3e", "c0a2e|c0a3e", false);
    //addType< tuple<STEPentity*, STEPentity*> >( "Geometric_Representation_Context", "a2e|a3e" );

    addType< tuple<string, string> >( "Product", "a0S|a1S", "", false);
    addType< tuple<STEPentity*> >( "Property_Definition", "a2se", "", false);
    addType< tuple<STEPentity*> >( "Shape_Aspect", "a2e", "", false);
    addType< tuple<STEPentity*> >( "Product_Definition", "a2e", "", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Shape", "a2se", "", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Formation", "a2e", "", false);
    addType< tuple<STEPentity*> >( "Product_Definition_Formation_With_Specified_Source", "a2e", "", false);
    addType< tuple<STEPentity*, STEPentity*> >( "Shape_Definition_Representation", "a0se|a1e", "", false);

    // materials stuff
    addType< tuple<string, vector<STEPentity*>, STEPentity*> >( "Styled_Item", "a0S|a1Ve|a2e", "", false);
    addType< tuple<vector<SDAI_Select*> > >( "Presentation_Style_Assignment", "a0Vs", "", false);
    addType< tuple<STEPentity*> >( "Surface_Style_Usage", "a1se", "", false);
    addType< tuple<vector<SDAI_Select*> > >( "Surface_Side_Style", "a1Vs", "", false);
    addType< tuple<STEPentity*> >( "Surface_Style_Fill_Area", "a0e", "", false);
    addType< tuple<vector<SDAI_Select*> > >( "Fill_Area_Style", "a1Vs", "", false);
    addType< tuple<STEPentity*> >( "Fill_Area_Style_Colour", "a1e", "", false);
    addType< tuple<string> >( "Draughting_Pre_Defined_Colour", "a0S", "", false);
    addType< tuple<double, double, double> >( "Colour_Rgb", "a1f|a2f|a3f", "", false);

    {
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
    blacklist["Product_Category"] = 1;
    blacklist["Product_Context"] = 1;
    //blacklist["Product_Definition_Formation"] = 1;
    blacklist["Product_Definition_Context"] = 1;
    blacklist["Product_Related_Product_Category"] = 1;
    blacklist["Property_Definition_Representation"] = 1;
    blacklist["Representation"] = 1;
    blacklist["Representation_Context"] = 1;
    blacklist["Representation_Item"] = 1;
    blacklist["Si_Unit"] = 1;
    blacklist["Uncertainty_Measure_With_Unit"] = 1;
    }
}

void VRSTEP::on_explorer_select(VRGuiTreeExplorer* e) {
    /*auto row = e->getSelected();
    auto id = e->get<int>(row, 0);
    auto type = e->get<string>(row, 1);
    auto val = e->get<string>(row, 2);
    string info = toString(id) + "\n" + type + "\n" + val;
    e->setInfo(info);*/
}

template<class T> void VRSTEP::addType(string typeName, string path, string cpath, bool print) {
    Type type;
    type.print = print;
    type.path = path;
    type.cpath = cpath;
    type.cb = VRFunction<STEPentity*>::create("STEPtypeCb", bind( &VRSTEP::parse<T>, this, placeholders::_1, path, cpath, typeName ));
    types[typeName] = type;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, string& t, char c, string& type) {
    if (c == 'S') {
        if (a) if (auto r = a->String() ) { r->asStr(t); return true; }
        if (an) { t = ((StringNode*)an)->value.c_str(); return true; }
    }
    return false;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, int& t, char c, string& type) {
    if (c == 'i') {
        if (a) if (auto r = a->Integer() ) { t = *r; return true; }
        if (an) { t = ((IntNode*)an)->value; return true; }
    }
    return false;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, STEPentity*& t, char c, string& type) {
    if (c == 'e') {
        if (e) { t = e; return true; }
        if (an) { t = ((STEPentity*)((EntityNode*)an)->node); return true; }
    }
    return false;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, SDAI_Select*& t, char c, string& type) {
    if (c == 's') {
        if (an) { t = ((SDAI_Select*)((SelectNode*)an)->node); return true; }
    }
    return false;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, double& t, char c, string& type) {
    if (c == 'f') {
        if (a) if (auto r = a->Real() ) { t = *r; return true; }
        if (an) { t = ((RealNode*)an)->value; return true; }
    }
    return false;
}

bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, bool& t, char c, string& type) {
    if (c == 'b') {
        if (a) if (auto r = a->Boolean() ) { t = *r; return true; }
        if (an) { t = ((IntNode*)an)->value; return true; } // TODO, check this!
    }
    return false;
}

template<typename T> bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, vector<T>& vec, char t, string& type) {
    //cout << type << " " << e->StepFileId() << ": get V" << t << " values: ";
    for( ; an != NULL; an = an->NextNode() ) {
        T v;
        if (!getValue(0,0,an,v,t,type)) { /*cout << endl;*/ return false; }
        //cout << " " << v;
        vec.push_back(v);
    }
    //cout << endl;
    return true;
}

void VRSTEP::fieldPush(field<STEPentity*>& f, string v) {
    int ID = toInt(splitString(v, '#')[1]);
    auto e = instancesById[ID].entity;
    f.data.push_back(e);
}

void VRSTEP::fieldPush(field<double>& f, string v) {
    f.data.push_back( toFloat(v) );
}

template<typename T> bool VRSTEP::getValue(STEPentity* e, STEPattribute* a, SingleLinkNode* an, field<T>& f, char t, string& type) {
    //cout << type << " " << e->StepFileId() << ": get V" << t << " values: ";

    for( ; an != NULL; an = an->NextNode() ) {
        f.height++;
        string s;
        ((STEPnode*)an)->asStr(s); // Idea (workaround): parse the string..
        s = splitString(s, '(')[1];
        s = splitString(s, ')')[0];
        for (auto v : splitString(s, ',')) fieldPush(f, v);
    }

    f.width = f.data.size()/f.height;

    return true;
}

STEPentity* VRSTEP::getSelectEntity(SDAI_Select* s) {
    string ID;
    s->STEPwrite(ID);

    if (ID == "") return 0;

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
            if (stype == "Named_Unit") {
                auto v = (SdaiUnit*)s;
                if (v->IsDerived_unit()) { SdaiDerived_unit* o = *v; return o; }
                if (v->IsNamed_unit()) { SdaiNamed_unit* o = *v; return o; }
            }
            if (stype == "Surface_Style_Usage") {
                auto v = (SdaiPresentation_style_select*)s;
                if (v->IsApproximation_tolerance()) { SdaiApproximation_tolerance* o = *v; return o; }
                if (v->IsCurve_style()) { SdaiCurve_style* o = *v; return o; }
                if (v->IsExternally_defined_style()) { SdaiExternally_defined_style* o = *v; return o; }
                if (v->IsFill_area_style()) { SdaiFill_area_style* o = *v; return o; }
                //if (v->IsNull_style()) { * o = *v; return o; } // TODO?
                if (v->IsPoint_style()) { SdaiPoint_style* o = *v; return o; }
                if (v->IsPre_defined_presentation_style()) { SdaiPre_defined_presentation_style* o = *v; return o; }
                if (v->IsSurface_style_usage()) { SdaiSurface_style_usage* o = *v; return o; }
                if (v->IsSymbol_style()) { SdaiSymbol_style* o = *v; return o; }
                if (v->IsText_style()) { SdaiText_style* o = *v; return o; }
            }
            cout << " Select entity not handled: " << stype << endl; // TODO
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

string toString(STEPentity* e) {
    return toString(e->StepFileId());
}

string toString(SDAI_Select* s) {
    return " SDAI_Select - to be implemented! ";
}

template<typename T>
string toString(vector<T>& v) {
    string s = string("vector of size ") + toString( v.size() ) + " data:";
    for (auto t : v) s += " " + toString(t);
    return s;
}

template<typename T>
string toString(field<T>& f) {
    string s = string("field of size ") + toString( f.width ) + " x " + toString( f.height ) + " data:";
    for (auto t : f.data) s += " " + toString(t);
    return s;
}

template<typename T> bool VRSTEP::query(STEPentity* e, string path, T& t, string type) {
    auto toInt = [](char c) { return int(c-'0'); };

    auto warn = [&](int i, string w) {
        cout << "VRSTEP::query " << path << ":" << i << " of type " << type << " entity " << e->EntityName() << " " << e->StepFileId() << " warning: " << w << endl;
    };

    bool verbose = 0;//(e->StepFileId() == 248);

    int j = 1;
    STEPattribute* curAttr = 0;
    STEPaggregate* curAggr = 0;
    SDAI_Select* curSel = 0;
    SingleLinkNode* curAggrNode = 0;
    string attrStr;
    for (unsigned int i=0; i<path.size(); i+=j) {
        bool isLast = (i == path.size()-1);
        j = 1;
        auto c = path[i];

        if (c == 'n') { return false; } // nothing to do

        if (c == 'a') { // attribute
            j = 2;
            int ai = toInt(path[i+1]);
            if (e->AttributeCount() <= ai) { warn(i, string("attrib ") + path[i+1] + " is out of range!"); return false; }
            curAttr = &e->attributes[ai];
            attrStr = curAttr->asStr();
            curAggr = 0;
            if (verbose) cout << "ATTR of " << e->EntityName() << " ai: " << ai << " data: " << attrStr << endl;
        }

        if (c == 'A') { // aggregate
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
                if (curSel->ValueType() != ENTITY_TYPE) { warn(i, " is not an entity!"); return false; }
                e = getSelectEntity(curSel);
            }
        }

        if (c == 's') {
            if (!curAttr) continue;
            curSel = curAttr->Select();
            if (!curSel) { warn(i, " is not a Select!"); return false; }
            curAttr = 0;
            curAggrNode = 0;
        }

        if (c == 'V') { // vector
            if (!curAttr) continue;
            curAggr = curAttr->Aggregate();
            if (!curAggr) { warn(i, " is not an Aggregate Vector!"); return false; } // TODO
            char tc = path[i+1];
            //if (tc == 's') tc = path[i+2];
            bool b = getValue(e, curAttr, curAggr->GetHead(), t, tc, type);
            if (verbose) cout << "VAL vect " << e->EntityName() << " v " << toString(t) << " t " << tc << endl;
            return b;
        }

        if (c == 'F') { // field
            if (!curAttr) continue;
            curAggr = curAttr->Aggregate();
            if (!curAggr) { warn(i, " is not an Aggregate Field!"); return false; } // TODO
            bool b = getValue(e, curAttr, curAggr->GetHead(), t, path[i+1], type);
            if (verbose) cout << "VAL field " << e->EntityName() << " v " << toString(t) << " t " << path[i+1] << endl;
            return b;
        }

        if (c == 'c') {
            j = 2;
            if (!e->IsComplex()) continue;
            auto ce = ( (STEPcomplex*)e )->head;
            int ci = toInt(path[i+1]);
            for (int i=0; i<ci; i++) ce = ce->sc;
            e = ce;
            if (verbose) cout << "CPLX " << e->EntityName() << endl;
            curAggrNode = 0;
            curAttr = 0;
        }

        if (isLast) {
            auto b = getValue(e, curAttr, curAggrNode, t, c, type);
            if (verbose) cout << "VAL " << e->EntityName() << " v " << toString(t) << endl;
            return b;
        }
        //if (isLast) cout << " t " << t << endl;
    }
    return false;
}

// helper function to set tuple members
template<class T, size_t N> struct Setup {
    static void setup(T& t, STEPentity* e, vector<string>& paths, VRSTEP* step, string type) {
        Setup<T, N-1>::setup(t, e, paths, step, type);
        string p = paths[N-1];
        step->query(e, p, get<N-1>(t), type); // segfault
    }
};

template<class T> struct Setup<T, 1> {
    static void setup(T& t, STEPentity* e, vector<string>& paths, VRSTEP* step, string type) {
        step->query(e, paths[0], get<0>(t), type);
    }
};

template<class... Args> void setup(tuple<Args...>& t, STEPentity* e, string paths, VRSTEP* step, string type) {
    auto vpaths = splitString(paths, '|');
    if (vpaths.size() == 0) { cout << "ERROR: No paths for entity " << e->StepFileId() << " of type " << type << endl; return; }
    if (sizeof...(Args) != vpaths.size()) { cout << "ERROR: Not enough paths defined for entity " << e->StepFileId() << " of type " << type << endl; return; }
    Setup<decltype(t), sizeof...(Args)>::setup(t, e, vpaths, step, type);
}
// end helper function

template<class T> void VRSTEP::parse(STEPentity* e, string path, string cpath, string type) {
    if (!instances.count(e)) { cout << "AAARGH" << endl; return; }
    if (instances[e].data) return; // allready parsed
    if (e->IsComplex() && cpath == "") { cout << "ERROR: Missing cpath for entity " << e->StepFileId() << " of type " << type << endl; return; } // missing paths
    string paths = e->IsComplex() ? cpath : path;
    if (paths == "n") return;
    auto t = new T();
    setup(*t, e, paths, this, type);
    instances[e].data = t;
    instancesByType[type].push_back(instances[e]);
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

void VRSTEP::explore(VRSTEP::Node* node, int parent) {
    //int ID = -1;
    string name, type;

    if (node->entity) {
        name = string(node->entity->EntityName()) + (node->entity->IsComplex() ? " (C)" : "");
        //ID = node->entity->STEPfile_id;
        type = node->type;
    }

    else if (node->aggregate) {
        name = node->a_name;
        //ID = 0;//node->select->STEPfile_id;
        type = node->type;
    }

    else if (node->select) {
        name = node->a_name;
        //ID = 0;//node->select->STEPfile_id;
        type = node->type;
    }

    else if(parent) {
        name = node->a_name;
        type = node->a_val;
        //ID = 0;
    }

    // huh?
    //if (ID >= 0) parent = explorer->add( parent, 3, ID, name.c_str(), type.c_str() );

    for (auto n : node->childrenV) explore(n, parent);
}

vector<STEPentity*> VRSTEP::unfoldComplex(STEPentity* e) {
    vector<STEPentity*> res;
    if (e->IsComplex()) {
        auto c = ( (STEPcomplex*)e )->head;
        while(c) {
            res.push_back(c);
            c = c->sc;
        }
    }
    return res;
}

void VRSTEP::registerEntity(STEPentity* se, bool complexPass) {
    if (se->IsComplex() && !complexPass) {
        for (auto e : unfoldComplex(se)) registerEntity(e, 1);
        return;
    }

    string type = se->EntityName();
    if (!instances.count(se)) {
        Instance i;
        i.ID = se->STEPfile_id;
        i.entity = se;
        i.type = type;
        instancesById[i.ID] = i;
        //instancesByType[i.type].push_back(i);
        instances[se] = i;
    }
}

void VRSTEP::parseEntity(STEPentity* se, bool complexPass) {
    if (se->IsComplex() && !complexPass) {
        for (auto e : unfoldComplex(se)) parseEntity(e, 1);
        return;
    }

    string type = se->EntityName();
    if (types.count(type) && types[type].cb) (*types[type].cb)(se);
}

void VRSTEP::Node::addChild(Node* c) {
    if (children.count(c->key()) == 0) {
        childrenV.push_back(c);
        children[c->key()] = c;
        c->parents[key()] = this;
    }
}

STEPentity* VRSTEP::Node::key() {
    if (entity) return entity;
    if (select) return (STEPentity*)select;
    if (aggregate) return (STEPentity*)aggregate;
    return 0;
}

void VRSTEP::traverseEntity(STEPentity* se, int lvl, VRSTEP::Node* parent, bool complexPass) {
    if (se->IsComplex() && !complexPass) {
        for (auto e : unfoldComplex(se)) traverseEntity(e, lvl, parent, 1);
        return;
    }

    Node* n = 0;
    if (!nodes.count(se)) {
        n = new Node();
        n->entity = se;
        nodes[se] = n;
    } else n = nodes[se];

    if (n->parents.size() == 0 && parent->key() == 0) { // not attached, attach it to root
        parent->addChild(n);
    } else if (parent->key() != 0 ) { // not root, just attach it
        for (auto p : n->parents) {
            if (p.first == 0) { // root is one of the parents, remove it
                p.second->children.erase(se);
                n->parents.erase(0);
                p.second->childrenV.erase(remove(p.second->childrenV.begin(), p.second->childrenV.end(), n), p.second->childrenV.end());
                break;
            }
        }
        parent->addChild(n);
    }

    /*if (se->STEPfile_id == 465 || se->STEPfile_id == 466) {
        cout << se->STEPfile_id << " parent " << parent << " " << parent->entity << " " << parent->select << endl;
        if (parent->entity) cout << " parent entity " << parent->entity->STEPfile_id << endl;
    }*/

    if (!n->traversed) {
        STEPattribute* attr = 0;
        se->ResetAttributes();
        while ( (attr = se->NextAttribute()) != NULL ) {
            //if (se->STEPfile_id == 465) cout << "  a " << ( attr->Entity() && !attr->IsDerived()) << " " << bool(attr->Aggregate()) << " " << bool(attr->Select()) << " " << attr->Name() << " " << attr->asStr() << endl;
            if ( attr->Entity() && !attr->IsDerived()) { traverseEntity( attr->Entity(), lvl, n); }
            else if ( auto a = attr->Aggregate() ) { traverseAggregate(a, attr->BaseType(), attr, lvl, n); }
            else if ( auto s = attr->Select() ) { traverseSelect(s, lvl, n); }
            if (attr->BaseType() != ENTITY_TYPE && attr->BaseType() != SELECT_TYPE) { // numeric value
                auto a = new Node();
                a->a_name = attr->Name();
                a->a_val = attr->asStr();
                n->childrenV.push_back(a);
            }
        }
    }

    n->traversed = 1;
}

void VRSTEP::traverseSelect(SDAI_Select* s, int lvl, VRSTEP::Node* parent) {
    auto e = getSelectEntity(s);
    if (e) {
        Node* n = 0;
        if (!nodes.count((STEPentity*)s)) {
            n = new Node();
            n->select = s;
            n->type = "SELECT";
            //n->a_name = ID;
            nodes[(STEPentity*)s] = n;
        } else n = nodes[(STEPentity*)s];

        parent->addChild(n);
        traverseEntity(e, lvl, n);
    }
}

string primTypeAsString(int t) {
    switch(t) {
        case sdaiINTEGER: return "sdaiINTEGER 0x0001";
        case sdaiREAL: return "sdaiREAL 0x0002";
        case sdaiBOOLEAN: return "sdaiBOOLEAN 0x0004";
        case sdaiLOGICAL: return "sdaiLOGICAL 0x0008";
        case sdaiSTRING : return "sdaiSTRING 0x0010";
        case sdaiBINARY: return "sdaiBINARY 0x0020";
        case sdaiENUMERATION: return "sdaiENUMERATION 0x0040";
        case sdaiSELECT : return "sdaiSELECT 0x0080";
        case sdaiINSTANCE: return "sdaiINSTANCE 0x0100";
        case sdaiAGGR    : return "sdaiAGGR 0x0200";
        case sdaiNUMBER  : return "sdaiNUMBER 0x0400";
        case ARRAY_TYPE: return "ARRAY_TYPE";
        case BAG_TYPE: return "BAG_TYPE";
        case SET_TYPE: return "SET_TYPE";
        case LIST_TYPE: return "LIST_TYPE";
        case GENERIC_TYPE: return "GENERIC_TYPE";
        case REFERENCE_TYPE: return "REFERENCE_TYPE ";
        case UNKNOWN_TYPE: return "UNKNOWN_TYPE";
    }
    return "???";
}

void VRSTEP::traverseAggregate(STEPaggregate *sa, int atype, STEPattribute* attr, int lvl, VRSTEP::Node* parent) {
    string s;

    STEPentity* sse;
    //STEPaggregate* ssa;
    SelectNode* sen;
    SDAI_Select* sdsel;
    PrimitiveType etype = UNKNOWN_TYPE;
    PrimitiveType ebtype = UNKNOWN_TYPE;
    const EntityDescriptor* ssedesc = 0;
    auto btype = atype;
    int ID;

    if (attr) {
        const AttrDescriptor* adesc = attr->getADesc();
        atype = adesc->AggrElemType();
    }

    Node* n = 0;
    if (!nodes.count((STEPentity*)sa)) {
        n = new Node();
        n->aggregate = sa;
        n->type = "AGGREGATE";
        sa->asStr(n->a_name);
        nodes[(STEPentity*)sa] = n;
    } else n = nodes[(STEPentity*)sa];

    parent->addChild(n);

    auto switchEType = [&](STEPentity* e) {
        ssedesc = e->getEDesc();
        if (ssedesc) {
            etype = ssedesc->Type();
            switch (etype) {
                case SET_TYPE:
                case LIST_TYPE:
                    ebtype = ssedesc->BaseType();
                    traverseAggregate((STEPaggregate *)e, ebtype, 0, lvl, n); break;
                case ENTITY_TYPE: traverseEntity(e, lvl, n); break;
                case ARRAY_TYPE: cout << " handle ARRAY_TYPE\n"; break;
                case BAG_TYPE: cout << " handle BAG_TYPE\n"; break;
                case GENERIC_TYPE: cout << " handle GENERIC_TYPE\n"; break;
                case REFERENCE_TYPE: cout << " handle REFERENCE_TYPE\n"; break;
                case UNKNOWN_TYPE: cout << " handle UNKNOWN_TYPE\n"; break;

                default:
                    cout << "traverseAggregate: entity Type not handled:" << etype << endl;
                    //ebtype = ssedesc->BaseType();
                    //traverseAggregate((STEPaggregate *)sse, etype, lvl, n); break;
                    break;
            }
        }
    };

    for( STEPnode* sn = (STEPnode*)sa->GetHead(); sn != NULL; sn = (STEPnode*)sn->NextNode()) {
        switch (atype) {
            case ENTITY_TYPE: // 256
                sse = (STEPentity*)((EntityNode*)sn)->node;
                if (sse->IsComplex()) {
                    for (auto e : unfoldComplex(sse)) switchEType(e);
                } else switchEType(sse);

                break;
            case SELECT_TYPE: // 128
                sen = (SelectNode*)sn;
                sdsel = sen->node;
                traverseSelect(sdsel, lvl, n);
                break;

            case SET_TYPE:
            case LIST_TYPE: // 1028  // TODOOOO
                sn->asStr(s); // Idea (workaround): parse the string..
                s = splitString(s, '(')[1];
                s = splitString(s, ')')[0];
                for (auto v : splitString(s, ',')) {
                    switch(btype) {
                        case ENTITY_TYPE:
                            ID = toInt(splitString(v, '#')[1]);
                            traverseEntity(instancesById[ID].entity, lvl, n);
                            break;
                        case REAL_TYPE: // TODO
                        default:
                            cout << "Warning: unhandled LIST_TYPE base type " << btype << endl;
                    }
                }
                break;
            case INTEGER_TYPE: // 1
            case REAL_TYPE: // 2
            case BOOLEAN_TYPE: // 4
            case LOGICAL_TYPE: // 8
            case STRING_TYPE: // 16
            case BINARY_TYPE: // 32
            case NUMBER_TYPE: // 1024
                break;
            case ENUM_TYPE: // 64
            case AGGREGATE_TYPE: // 512
            default:
                cout << "aggregate Type not handled:" << atype << endl;
        }
    }
}

Vec3d toVec3d(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instances) {
    if (!instances.count(i)) { cout << "toVec3d FAILED with instance " << i << endl; return Vec3d(); }
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
        return Vec3d(y,x,-z)*L;
    }
    cout << "toVec3d FAILED with instance type " << I.type << endl;
    return Vec3d();
}

PosePtr toPose(STEPentity* i, map<STEPentity*, VRSTEP::Instance>& instances) {
    auto I = instances[i];
    if (I.type == "Axis2_Placement_3d") {
        Vec3d p = toVec3d( I.get<0, STEPentity*, STEPentity*, STEPentity*>(), instances);
        Vec3d d = toVec3d( I.get<1, STEPentity*, STEPentity*, STEPentity*>(), instances);
        Vec3d u = toVec3d( I.get<2, STEPentity*, STEPentity*, STEPentity*>(), instances);
        //d[2] *= -1;
        return Pose::create(p,d,u);
        //return Pose(p,d,u);
    }
    cout << "toPose FAILED with instance type " << I.type << endl;
    return Pose::create();
}

struct VRSTEP::Edge : public VRSTEP::Instance, public VRBRepEdge {
    void handleEdge(STEPentity* e, map<STEPentity*, Instance>& instances, bool cplx = 0) {
        auto EdgeGeo = instances[ e ];

        if (EdgeGeo.type == "Line") { type = EdgeGeo.type; return; }

        if (EdgeGeo.type == "Circle") {
            type = EdgeGeo.type;
            center = toPose( EdgeGeo.get<0, STEPentity*, double>(), instances );
            radius = EdgeGeo.get<1, STEPentity*, double>();
            return;
        }

        if (EdgeGeo.type == "B_Spline_Curve_With_Knots") {
            type = EdgeGeo.type;
            deg = EdgeGeo.get<0, int, vector<STEPentity*>, bool, vector<int>, vector<double> >();
            vector<STEPentity*> control_points = EdgeGeo.get<1, int, vector<STEPentity*>, bool, vector<int>, vector<double> >();
            vector<int> multiplicities = EdgeGeo.get<3, int, vector<STEPentity*>, bool, vector<int>, vector<double> >();
            vector<double> Knots = EdgeGeo.get<4, int, vector<STEPentity*>, bool, vector<int>, vector<double> >();
            if (control_points.size() <= 1) cout << "Warning: No control points of B_Spline_Curve_With_Knots" << endl;

            for (auto e : control_points) cpoints.push_back(toVec3d(e, instances));

            if (multiplicities.size() != Knots.size()) { cout << "B_Spline_Curve_With_Knots, multiplicities and knots do not match: " << multiplicities.size() << " " << Knots.size() << " id " << EdgeGeo.ID << endl; return; }

            // apply knot multiplicities
            for (unsigned int i=0; i<Knots.size(); i++) {
                for (int j=0; j<multiplicities[i]; j++) knots.push_back(Knots[i]);
            }
            if (knots.size() < cpoints.size() + deg + 1) { cout << "B_Spline_Curve_With_Knots, not enough knots: " << knots.size() << endl; return; }
            return;
        }

        if (EdgeGeo.type == "Rational_B_Spline_Curve") {
            weights = EdgeGeo.get<3, int, vector<STEPentity*>, bool, vector<double> >();
        }

        if (cplx) return; // TODO: is this right?

        // int, vector<STEPentity*>, bool
        if (EdgeGeo.type == "B_Spline_Curve" || EdgeGeo.type == "Rational_B_Spline_Curve") { // TODO
            //cout << "  edge type " << EdgeGeo.type << endl;
            //int deg = EdgeGeo.get<0, int, vector<STEPentity*>, bool>();
            vector<STEPentity*> control_points = EdgeGeo.get<1, int, vector<STEPentity*>, bool>();
            for (auto e : control_points) points.push_back(toVec3d(e, instances)); // TODO: correct??
            if (points.size() <= 1) cout << "Warning: No edge points of B_Spline_Curve" << endl;
            return;
        }

        cout << "Error: edge geo type not handled " << EdgeGeo.type << endl;
    }

    Edge(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        if (i.type == "Oriented_Edge") {
            auto& EdgeElement = instances[ i.get<0, STEPentity*, bool>() ];
            //bool edir = i.get<1, STEPentity*, bool>();
            if (EdgeElement.type == "Edge_Curve") {
                *EBeg = toVec3d( EdgeElement.get<0, STEPentity*, STEPentity*, STEPentity*>(), instances );
                *EEnd = toVec3d( EdgeElement.get<1, STEPentity*, STEPentity*, STEPentity*>(), instances );
                auto EdgeGeoI = EdgeElement.get<2, STEPentity*, STEPentity*, STEPentity*>();

                if (instances[EdgeGeoI].type == "Surface_Curve") {
                    EdgeGeoI = instances[EdgeGeoI].get<0, STEPentity*, vector<STEPentity*> >();
                }

                if (EdgeGeoI->IsComplex()) {
                    for (auto e : unfoldComplex(EdgeGeoI)) handleEdge(e, instances, 1);
                } else handleEdge(EdgeGeoI, instances);

                build(type);
                if (points.size() <= 1) cout << "Warning: No edge points!" << endl;
                return;

            } else cout << "Error: edge element type not handled " << EdgeElement.type << endl;
        } else cout << "Error: edge type not handled " << i.type << endl;
    }
};

struct VRSTEP::Bound : public VRSTEP::Instance, public VRBRepBound {
    Bound(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        BRepType = type;
        if (type != "Face_Outer_Bound") outer = false;
        if (type == "Face_Bound" || type == "Face_Outer_Bound") {
            auto& Loop = instances[ get<0, STEPentity*, bool>() ];
            //bool dir = get<1, STEPentity*, bool>();
            for (auto l : Loop.get<0, vector<STEPentity*> >() ) {
                Edge edge(instances[l], instances);
                if (edge.points.size() <= 1) {
                    //cout << "Warning2: No edge points " << &edge << endl;
                    continue;
                }
                edges.push_back(edge);
            }
        }

        if (edges.size() > 1) {
            if ( sameVec(edges[0].beg(), edges[1].beg()) || sameVec(edges[0].beg(), edges[1].end()) ) edges[0].swap(); // swap first edge

            for (unsigned int i=1; i<edges.size(); i++) {
                auto& e1 = edges[i-1];
                auto& e2 = edges[i];
                if ( sameVec(e2.end(), e1.end()) ) e2.swap();
            }
        } else {
            if (edges.size() == 0) { cout << "Warning: No bound edges" << endl; return; }
            if ( !sameVec(edges[0].beg(), edges[0].end()) ) { cout << "Warning: Single NOT closed edge!" << endl; return; }
        }

        /*if (edges.size() == 1 && edges[0].type == "Circle") {
            if (e.angles.size()) {
                for (auto& a : e.angles) angles.push_back(a);
            }
            return;
        }*/

        for (auto& e : edges) {
            for (auto& p : e.points) {
                /*cout << " " << p;
                if (points.size() > 0) cout << " " << sameVec(p, points[points.size()-1]) << " " << sameVec(p, points[0]);
                cout << endl;*/
                if (points.size() > 0) {
                    if (sameVec(p, points[points.size()-1])) continue; // same as last point
                    if (sameVec(p, points[0])) continue; // same as first point
                }
                points.push_back(p);
            }
        }
        if (points.size() == 0) cout << "Warning1: No bound points" << endl;
    }
};

struct VRSTEP::Surface : public VRSTEP::Instance, public VRBRepSurface {
    void handleSurface(STEPentity* e, map<STEPentity*, Instance>& instances) {
        bool cplx = e->IsComplex();
        string etype = e->EntityName();
        auto& inst = instances[e];

        if (etype == "Plane") trans = toPose( inst.get<0, STEPentity*>(), instances);
        if (etype == "Cylindrical_Surface") {
            trans = toPose( inst.get<0, STEPentity*, double>(), instances );
            R = inst.get<1, STEPentity*, double>();
        }

        // int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double>
        // degree_u, degree_v, control_points, u_closed, v_closed, self_intersect, u multiplicities, v multiplicities, u knots, v knots
        if (etype == "B_Spline_Surface_With_Knots") {
            type = etype;
            degu = inst.get<0, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            degv = inst.get<1, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            auto fcp = inst.get<2, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            //auto uclosed = inst.get<3, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            //auto vclosed = inst.get<4, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            //auto intersect = inst.get<5, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            auto u_multiplicities = inst.get<6, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >(); // segfault
            auto v_multiplicities = inst.get<7, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            auto u_knots = inst.get<8, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();
            auto v_knots = inst.get<9, int, int, field<STEPentity*>, bool, bool, bool, vector<int>, vector<int>, vector<double>, vector<double> >();

            // apply knot multiplicities
            if (u_knots.size() == u_multiplicities.size()) {
                for (unsigned int i=0; i<u_knots.size(); i++) {
                    for (int j=0; j<u_multiplicities[i]; j++) knotsu.push_back(u_knots[i]);
                }
            } else knotsu = u_knots;
            if (v_knots.size() == v_multiplicities.size()) {
                for (unsigned int i=0; i<v_knots.size(); i++) {
                    for (int j=0; j<v_multiplicities[i]; j++) knotsv.push_back(v_knots[i]);
                }
            } else knotsv = v_knots;

            cpoints.width = fcp.width;
            cpoints.height = fcp.height;
            for (auto e : fcp.data) {
                cpoints.data.push_back( toVec3d(e, instances) );
            }
        }

        if (etype == "Rational_B_Spline_Surface") {
            weights = inst.get<0, field<double> >();
        }

        if (cplx) return;

        // int, int, vector<STEPentity*>, bool, bool, bool
        // degree_u, degree_v, control_points, u_closed, v_closed, self_intersect
        if (etype == "B_Spline_Surface") {
            degu = inst.get<0, int, int, field<STEPentity*>, bool, bool, bool>();
            degv = inst.get<1, int, int, field<STEPentity*>, bool, bool, bool>();
            auto fcp = inst.get<2, int, int, field<STEPentity*>, bool, bool, bool>();
            cpoints.width = fcp.width;
            cpoints.height = fcp.height;
            for (auto e : fcp.data) {
                cpoints.data.push_back( toVec3d(e, instances) );
            }
        }
    }

    Surface(Instance& i, map<STEPentity*, Instance>& instances) : Instance(i) {
        if (i.entity->IsComplex()) {
            for (auto e : unfoldComplex(i.entity)) handleSurface(e, instances);
            return;
        } else handleSurface(i.entity, instances);
    }
};

void VRSTEP::buildGeometries() {
    cout << blueBeg << "VRSTEP::buildGeometries start\n" << colEnd;
    for (auto BrepShape : instancesByType["Advanced_Brep_Shape_Representation"]) {
        static int i=0; i++;
        //if (i != 5) continue;

        string name = BrepShape.get<0, string, vector<STEPentity*> >();
        auto geo = VRGeometry::create(name);

        cout << "VRSTEP::buildGeometries " << name << " ID: " << BrepShape.ID << endl;

        for (auto i : BrepShape.get<1, string, vector<STEPentity*> >() ) {
            auto& Item = instances[i];
            if (Item.type == "Manifold_Solid_Brep") {
                auto& Outer = instances[ Item.get<0, STEPentity*>() ];
                for (auto j : Outer.get<0, vector<STEPentity*> >() ) {
                    auto& Face = instances[j];
                    if (Face.type == "Advanced_Face") {
                        auto& s = instances[ Face.get<1, vector<STEPentity*>, STEPentity*, bool>() ];
                        Surface surface(s, instances);
                        //bool same_sense = Face.get<2, vector<STEPentity*>, STEPentity*, bool>();
                        for (auto k : Face.get<0, vector<STEPentity*>, STEPentity*, bool>() ) {
                            auto& b = instances[k];
                            Bound bound(b, instances);
                            surface.bounds.push_back(bound);
                        }
                        geo->merge( surface.build(surface.type) );
                        //geo->addChild( surface.build(surface.type) );
                    } else cout << "VRSTEP::buildGeometries Error 2 " << Face.type << " " << Face.ID << endl;
                }
                if (materials.count(Item.entity)) geo->setMaterial(materials[Item.entity]);
            } else if (Item.type == "Axis2_Placement_3d") { // ignore?
            } else cout << "VRSTEP::buildGeometries Error 1 " << Item.type << " " << Item.ID << endl;
        }

        resGeos[BrepShape.entity] = geo;
    }
    cout << "VRSTEP::buildGeometries  got " << resGeos.size() << " geometries" << endl;
    cout << blueBeg << "VRSTEP::buildGeometries finished\n" << colEnd;
}

VRSTEP::Instance& VRSTEP::getInstance(STEPentity* e) {
    static Instance invalid;
    if (e == 0) return invalid;
    if (!instances.count(e)) return invalid;
    return instances[e];
}


class VRSTEPProductStructure {
    public:
        struct nLink {
            string name;
            string parent;
            string child;
            PosePtr p;

            string toString() {
                string s = "link n: " + name + " p: " + parent + " c: " + child;
                return s;
            }
        };

        struct node {
            string obj;
            VRTransformPtr trans;
            vector<shared_ptr<nLink> > parents;
            vector<shared_ptr<nLink> > children;

            string toString() {
                string s = "o: " + obj;
                for (auto p : parents) s += "\n pi: " + p->toString();
                for (auto c : children) s += "\n ci: " + c->toString();
                return s;
            }
        };

        map<string, shared_ptr<node> > nodes;

        shared_ptr<node> addNode(string obj) {
            auto n = shared_ptr<node>( new node() );
            n->obj = obj;
            nodes[obj] = n;
            return n;
        }

        shared_ptr<nLink> addLink(string obj, string parent) {
            auto l = shared_ptr<nLink>(new nLink() );
            l->parent = parent;
            l->child = obj;
            nodes[obj]->parents.push_back( l );
            nodes[parent]->children.push_back( l );
            return l;
        }

        shared_ptr<node> getRoot() {
            for (auto ni : nodes) if (ni.second->parents.size() == 0) return ni.second;
            return 0;
        }

        VRTransformPtr construct() { return construct(getRoot()); }
        VRTransformPtr construct( shared_ptr<node> n ) {
            if (!n) return 0;
            vector<VRTransformPtr> childrenT;
            for (auto l : n->children) {
                auto c = construct( nodes[l->child] );
                c->setPose(l->p);
                c->setName(l->name);
                childrenT.push_back( c );
            }
            VRTransformPtr t = dynamic_pointer_cast<VRTransform>( n->trans->duplicate() );
            for (VRTransformPtr c : childrenT) t->addChild(c);
            return t;
        }
};

void VRSTEP::buildScenegraph() {
    cout << blueBeg << "VRSTEP::buildScenegraph start\n" << colEnd;

    // get geometries -------------------------------------
    map<STEPentity*, STEPentity*> SRepToGEO;
    for (auto ShapeRepRel : instancesByType["Shape_Representation_Relationship"]) {
        if (ShapeRepRel.entity->IsComplex()) continue;
        auto ABrep = ShapeRepRel.get<0, STEPentity*, STEPentity*>();
        auto SRep = ShapeRepRel.get<1, STEPentity*, STEPentity*>();
        if (!ABrep || !SRep) { cout << "VRSTEP::buildScenegraph Warning 1\n" ; continue; } // empty one
        SRepToGEO[SRep] = ABrep;
        cout << "Shape_Representation_Relationship: " << ShapeRepRel.ID << " " << ShapeRepRel.entity->IsComplex() << " srep " << SRep->EntityName() << " " << SRep->StepFileId() << " " << ABrep->EntityName() << " " << ABrep->StepFileId() << endl;
    }

    cout << "VRSTEP::buildScenegraph SRepToGEO " << SRepToGEO.size() << endl;
    for (auto o : SRepToGEO) cout << " SRep: " << o.first->StepFileId() << " ABrep: " << o.second->StepFileId() << endl;

    map<STEPentity*, STEPentity*> ProductToSRep;
    for (auto ShapeRepRel : instancesByType["Shape_Definition_Representation"]) {
        if (!ShapeRepRel) { cout << "VRSTEP::buildScenegraph Error 2 " << ShapeRepRel.ID << endl; continue; }
        auto& PDS = getInstance( ShapeRepRel.get<0, STEPentity*, STEPentity*>() );
        if (!PDS) { cout << "VRSTEP::buildScenegraph Error 3.1 " << ShapeRepRel.ID << endl; continue; }
        if (PDS.type == "Property_Definition") PDS = getInstance( PDS.get<0, STEPentity*>() );
        if (!PDS) { cout << "VRSTEP::buildScenegraph Error 3.2 " << ShapeRepRel.ID << endl; continue; }
        if (PDS.type == "Shape_Aspect") PDS = getInstance( PDS.get<0, STEPentity*>() );
        if (!PDS) { cout << "VRSTEP::buildScenegraph Error 3.3 " << ShapeRepRel.ID << endl; continue; }

        auto& PDef = getInstance( PDS.get<0, STEPentity*>() );
        if (!PDef) { cout << "VRSTEP::buildScenegraph Error 4 " << ShapeRepRel.ID << endl; continue; }
        auto& PDF = getInstance( PDef.get<0, STEPentity*>() );
        if (!PDF) { cout << "VRSTEP::buildScenegraph Error 5 " << ShapeRepRel.ID << endl; continue; }
        auto& Product = getInstance( PDF.get<0, STEPentity*>() );
        if (!Product) { cout << "VRSTEP::buildScenegraph Error 6 " << ShapeRepRel.ID << endl; continue; }
        auto SRep = ShapeRepRel.get<1, STEPentity*, STEPentity*>();
        ProductToSRep[Product.entity] = SRep;
    }

    cout << "VRSTEP::buildScenegraph ProductToSRep " << ProductToSRep.size() << endl;

    // get product definitions -------------------------------------
    resRoot->setName("STEPRoot");
    map<string, VRTransformPtr> objs;

    for (auto PDefShape : instancesByType["Product_Definition_Shape"]) {
        if (!PDefShape) { cout << "VRSTEP::buildScenegraph Error 7\n" ; continue; }
        auto& Def = getInstance( PDefShape.get<0, STEPentity*>() ); // Product_Definition
        if (!Def) { cout << "VRSTEP::buildScenegraph Error 8\n" ; continue; }

        if (Def.type == "Product_Definition") {
            auto& PDF = getInstance( Def.get<0, STEPentity*>() );
            if (!PDF) { cout << "VRSTEP::buildScenegraph Error 9\n" ; continue; }
            auto& Product = getInstance( PDF.get<0, STEPentity*>() );
            if (!Product) { cout << "VRSTEP::buildScenegraph Error 10\n" ; continue; }
            string name = Product.get<0, string, string>();

            STEPentity* brep = 0;
            if (ProductToSRep.count(Product.entity)) brep = ProductToSRep[Product.entity];
            else { cout << "VRSTEP::buildScenegraph Error: Product not found!"; continue; }

            string type = brep->EntityName();
            if (type == "Shape_Representation") {
                if (SRepToGEO.count(brep)) brep = SRepToGEO[brep];
            }

            cout << "VRSTEP::buildScenegraph geo " << name << " " << Product.ID << " " << PDF.ID << " brep " << brep->StepFileId() << " " << brep->EntityName() << " " << SRepToGEO.count(brep) << endl;

            VRTransformPtr o;
            if (resGeos.count(brep)/* && !SRepToGEO.count(brep)*/) o = resGeos[brep];
            else o = VRTransform::create(name);
            objs[name] = o;
        }
    }

    cout << "VRSTEP::buildScenegraph objs " << objs.size() << endl;

    // build scene graph and set transforms ----------------------------------------
    VRSTEPProductStructure product_structure;
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
        //cout << Relating.type << " " << Related.type << endl;
        auto& PDF1 = getInstance( Relating.get<0, STEPentity*>() );
        auto& PDF2 = getInstance( Related.get<0, STEPentity*>() );
        if (!PDF1 || !PDF2) { cout << "VRSTEP::buildScenegraph Error 19\n" ; continue; }
        auto& Product1 = getInstance( PDF1.get<0, STEPentity*>() );
        auto& Product2 = getInstance( PDF2.get<0, STEPentity*>() );
        if (!Product1 || !Product2) { cout << "VRSTEP::buildScenegraph Error 20\n" ; continue; }

        string parent = Product1.get<0, string, string>();
        string obj = Product2.get<0, string, string>();
        if (!objs.count(parent)) { cout << "VRSTEP::buildScenegraph Error 21 parent not found " << parent << endl ; continue; }
        if (!objs.count(obj)) { cout << "VRSTEP::buildScenegraph Error 22 object not found " << obj << endl ; continue; }

        if (!product_structure.nodes.count(parent)) {
            auto n = product_structure.addNode(parent);
            n->trans = objs[parent];
        }
        if (!product_structure.nodes.count(obj)) {
            auto n = product_structure.addNode(obj);
            if (resGeos.count(Shape1.entity)) n->trans = resGeos[Shape1.entity];
            else n->trans = objs[obj];
        }

        auto l = product_structure.addLink(obj, parent);
        l->name = name;
        l->p = pose2;
    }

    resRoot->addChild( product_structure.construct() );

    //cout << "VRSTEP::buildScenegraph objs " << objs.size() << endl;
    cout << blueBeg << "VRSTEP::buildScenegraph finished\n" << colEnd;
}

void VRSTEP::buildMaterials() {
    for (auto& Styled_Item : instancesByType["Styled_Item"]) {
        string name = Styled_Item.get<0, string, vector<STEPentity*>, STEPentity*>();
        auto geo = Styled_Item.get<2, string, vector<STEPentity*>, STEPentity*>();
        auto m = VRMaterial::create(name);
        materials[geo] = m;

        auto style_assigments = Styled_Item.get<1, string, vector<STEPentity*>, STEPentity*>();
        for (auto sa : style_assigments) {
            auto& style_assignment = getInstance(sa);
            vector<SDAI_Select*> styles = style_assignment.get<0, vector<SDAI_Select*> >();
            for (auto ss : styles) {
                auto& style = getInstance( getSelectEntity(ss) );
                if (style.type == "Surface_Style_Usage") {
                    auto& surface_side_style = getInstance( style.get<0, STEPentity*>() );
                    vector<SDAI_Select*> side_styles = surface_side_style.get<0, vector<SDAI_Select*> >();
                    for (auto ss : side_styles) {
                        auto& side_style = getInstance( getSelectEntity(ss) );
                        if (side_style.type == "Surface_Style_Fill_Area") {
                            auto& fill_area_style = getInstance( side_style.get<0, STEPentity*>() );
                            vector<SDAI_Select*> fill_styles = fill_area_style.get<0, vector<SDAI_Select*> >();
                            for (auto ss : fill_styles) {
                                auto& fill_style = getInstance( getSelectEntity(ss) );
                                if (fill_style.type == "Fill_Area_Style_Colour") {
                                    auto& color = getInstance( fill_style.get<0, STEPentity*>() );
                                    if (color.type == "Draughting_Pre_Defined_Colour") {
                                        string c = color.get<0, string>();
                                        c = splitString(c,'\'')[1];
                                        m->setDiffuse( VRMaterial::toColor(c) );
                                        continue;
                                    }
                                    if (color.type == "Colour_Rgb") {
                                        double r = color.get<0, double, double, double>();
                                        double g = color.get<1, double, double, double>();
                                        double b = color.get<2, double, double, double>();
                                        m->setDiffuse( Color3f(r,g,b) );
                                        continue;
                                    }
                                    cout << "Warning, buildMaterials: color type unknown - " << color.type << endl;
                                } else cout << "Warning, buildMaterials: fill_style type unknown - " << fill_style.type << endl;
                            }
                        } else cout << "Warning, buildMaterials: side_style type unknown - " << side_style.type << endl;
                    }
                } else cout << "Warning, buildMaterials: style type unknown - " << style.type << endl;
            }
        }
    }
}

void VRSTEP::build() {
    blacklisted = 0;

    int N = instMgr->InstanceCount();
    for( int i=0; i<N; i++ ) { // add all instances to dict
        STEPentity* se = instMgr->GetApplication_instance(i);
        registerEntity(se);
    }

    for( int i=0; i<N; i++ ) { // parse all instances
        STEPentity* se = instMgr->GetApplication_instance(i);
        parseEntity(se);
    }

    auto root = new VRSTEP::Node();
    for( int i=0; i<N; i++ ) {
        STEPentity* se = instMgr->GetApplication_instance(i);
        string name = se->EntityName();
        traverseEntity(se,0,root);
    }

    if (options == "explorer") {
        //explorer->setSelectCallback( VRFunction<VRGuiTreeExplorer*>::create( "step_explorer", bind(&VRSTEP::on_explorer_select, this, _1) ) );
        //explore(root);
    }

    buildMaterials();
    buildGeometries();
    buildScenegraph();

    cout << "build results:\n";
    cout << instances.size() << " STEP entities parsed\n";
    cout << blacklisted << " STEP blacklisted entities ignored\n";
    cout << resGeos.size() << " VR objects created\n";
}

void VRSTEP::load(string file, VRTransformPtr t, string opt) {
    options = opt;
    //if (options == "explorer") explorer = VRGuiTreeExplorer::create("iss", "STEP file explorer (" + file + ")");
    resRoot = t;
    open(file);
    build();
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

poSTEPentity*s -> Vec3d
vectors -> Vec3d
directions -> Vec3d
curves
surfaces

*/





