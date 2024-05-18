#include "VRImguiTreeview.h"
#include "VRImguiInput.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>

ImTreeview::ImTreeview(string ID) : ID(ID), root("", ID, "", 0) {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("treeview_select", [&](OSG::VRGuiSignals::Options o){ if (o["treeview"] == this->ID) handleSelection(o["node"]); return true; } );
    mgr->addCallback("on_tv_node_rename", [&](OSG::VRGuiSignals::Options o){ if (o["treeview"] == this->ID) rename(o["node"], o["name"]); return true; } );
}

void ImTreeview::handleSelection(string node) {
    if (nodes.count(selected)) nodes[selected]->isSelected = false;
    selected = node;
    nodes[selected]->isSelected = true;
}

void ImTreeview::render() {
    root.render(0);
}

void ImTreeview::rename(string ID, string label) {
    if (nodes.count(ID)) nodes[ID]->label = label;
}

void ImTreeview::Node::renderButton() {
    string idLbl = label + "##" + ID;
    if (ImGui::Button(idLbl.c_str())) {
        //isSelected = true;
        //cout << "ImTreeview::Node::renderButton " << tvID << ", " << ID << ", " << label << endl;
        uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}});
    }
}

void ImTreeview::Node::renderEditable() {
    if (!isSelected) renderButton();
    else {
        if (!input) input = new ImInput(ID+"_input", "", label, ImGuiInputTextFlags_EnterReturnsTrue);
        if (input->render(0)) {
            //cout << "ImTreeview::Node::renderEditable " << tvID << ", " << ID << endl;
            uiSignal("treeview_rename", {{"treeview",tvID}, {"node",ID}, {"name",input->value}});
            uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}});
        }
    }
}

bool ImTreeview::Node::render(int lvl) {
    if (label == "") { // root nodes
        for (auto& child : children) child->render(lvl+1);
        return true;
    }

    if (options & IM_TV_NODE_EDITABLE) renderEditable();
    else renderButton();

    bool open = false;
    if (children.size() > 0) {
        ImGui::SameLine();
        open = ImGui::CollapsingHeader(("##"+ID).c_str(), nodeFlags);
        if (open) {
            ImGui::Indent(8);
            for (auto& child : children) child->render(lvl+1);
            ImGui::Unindent(8);
        }
    }
    return open;
}

ImTreeview::Node::Node(string ID, string tvID, string label, int options) : ID(ID), tvID(tvID), label(label), options(options) {}

void ImTreeview::setNodeFlags(ImGuiTreeNodeFlags flags) {
    nodeFlags = flags;
    root.nodeFlags = flags;
    for (auto n : nodes) n.second->nodeFlags = flags;
}

ImTreeview::Node* ImTreeview::add(string nID, string label, int options, string parent) {
    Node* n = 0;
    if (nodes.count(parent)) n = nodes[parent]->add(nID, label, options);
    else n = root.add(nID, label, options);
    nodes[nID] = n;
    return n;
}

ImTreeview::Node* ImTreeview::Node::add(string childID, string child, int options) {
    Node* n = new Node(childID, tvID, child, options);
    n->nodeFlags = nodeFlags;
    children.push_back(n);
    return n;
}

void ImTreeview::expandAll() { // TODO
    ;
}

void ImTreeview::clear() {
    root.children.clear();
    nodes.clear();
}
