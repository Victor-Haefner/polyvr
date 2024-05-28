#include "VRSTEPExplorer.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"

#include <schema.h>

using namespace OSG;

VRSTEPExplorer::VRSTEPExplorer(string file) {
    /*treeview = VRGuiTreeExplorer::create("ssssp", "STEP file explorer (" + file + ")");
    treeview->setSelectCallback( VRFunction<VRGuiTreeExplorer*>::create( "step_explorer", bind(&VRSTEPExplorer::on_explorer_select, this, placeholders::_1) ) );*/
}

VRSTEPExplorer::~VRSTEPExplorer() {}

VRSTEPExplorerPtr VRSTEPExplorer::create(string file) { return VRSTEPExplorerPtr( new VRSTEPExplorer(file) ); }
VRSTEPExplorerPtr VRSTEPExplorer::ptr() { return static_pointer_cast<VRSTEPExplorer>(shared_from_this()); }

bool VRSTEPExplorer::doIgnore(VRSTEP::Node* node) {
    // simple stuff
    if (node->a_val == "'NONE'" || node->a_val == "''") {
        if (node->a_name == "name") return true;
        if (node->a_name == "description") return true;
    }

    if (node->a_val == "NONE") {
        if (node->a_name == "prefix") return true;
    }

    if (node->type == "AGGREGATE") {
        if (node->childrenV.size() == 0) return true;
    }

    if (node->a_name == "make_or_buy") return true;

    // advanced stuff, may be interesting in some instances
    if (node->entity) {
        const char* ename = node->entity->EntityName();
        string name = ename ? ename : "";
        if (name == "Geometric_Representation_Context") return true; // specifies the space dimensions
        if (name == "Global_Uncertainty_Assigned_Context") return true; // specifies length unit
        //if (name == "Global_Unit_Assigned_Context") return true; // specifies length unit
        if (name == "Representation_Context") return true; // specifies workspace
        if (name == "Product_Definition_Context") return true; // specifies MCAD
        if (name == "Product_Context") return true; // specifies MCAD
        if (name == "Application_Protocol_Definition") return true; // specifies automotive
    }

    return false;
}

void VRSTEPExplorer::on_explorer_select(VRGuiTreeExplorer* e) {
#ifndef WITHOUT_IMGUI
    /*auto row = e->getSelected();
    auto id = e->get<const char*>(row, 1);
    VRSTEP::Node* node = (VRSTEP::Node*)e->get<void*>(row, 4);

    string info = "ID: #" + string(id?id:"");
    info += "\n type: " + node->type;
    info += "\n name: " + node->a_name;
    info += "\n data: " + node->a_val;

    if (node->entity) {
        const char* ename = node->entity->EntityName();
        info += "\n entity: " + toString((void*)node->entity);
        info += "\n    ID: " + toString(node->entity->STEPfile_id);
        info += "\n    name: " + string(ename?ename:"-");
        info += "\n    complex: " + string(node->entity->IsComplex() ? "yes" : "no");
    }

    if (node->aggregate) {
        info += "\n aggregate: " + toString((void*)node->aggregate);
        info += "\n    size: ";// + node->aggregate->EntryCount();
    }

    if (node->select) {
        const char* stype = node->select->UnderlyingTypeName().c_str();
        info += "\n select: " + toString((void*)node->select);
        info += "\n    type: " + string(stype?stype:"");
    }

    e->setInfo(info);*/
#endif
}

void VRSTEPExplorer::traverse(VRSTEPPtr sPtr, VRSTEP::Node* node, bool doFilter) {
    stepPtr = sPtr;
    explore(node, 0, doFilter);
}

void VRSTEPExplorer::translate(string& name) {
    if (name == "Context_Dependent_Shape_Representation") name = "Assembly";
    if (name == "Mechanical_Design_Geometric_Presentation_Representation") name = "Face";
    if (name == "Presentation_Layer_Assignment") name = "Material";
    if (name == "Product_Related_Product_Category") name = "PartInfo";
    if (name == "Shape_Representation_Relationship") name = "Part";
    if (name == "Axis2_Placement_3d") name = "Pose";

    if (name == "Shape_Definition_Representation") name = "Component";
    if (name == "Application_Protocol_Definition") name = "PartInfo2";
}

void VRSTEPExplorer::explore(VRSTEP::Node* node, int parent, bool doFilter) {
    if (!node) return;
    if (doFilter)
        if (doIgnore(node)) return;


    int ID = -1;
    string name = node->a_name;
    string type = node->type;
    string data = node->a_val;
    if (data == "NONE") data = "";
    if (name == "NONE") name = "";

    if (node->entity) {
        const char* ename = node->entity->EntityName();
        name = ename ? ename : "";
        type += (node->entity->IsComplex() ? " (C)" : "");
        ID = node->entity->STEPfile_id;
    }

    else if (node->aggregate) { ID = 0; name = ""; }
    else if (node->select) { ID = 0; /*node->select->STEPfile_id;*/ }
    else if (parent) { ID = 0; }

    if (doFilter) translate(name);

#ifndef WITHOUT_IMGUI
    static size_t c = 0; c++;
    string sID = "#"+toString(ID);
    //if (ID >= 0 /*&& c < 30000*/) parent = treeview->add( parent, 5, type.c_str(), sID.c_str(), name.c_str(), data.c_str(), node);
#endif

    for (auto n : node->childrenV) explore(n, parent, doFilter);
}
