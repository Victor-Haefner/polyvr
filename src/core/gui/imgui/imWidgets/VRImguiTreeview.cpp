#include "VRImguiTreeview.h"
#include "VRImguiInput.h"
#include "core/gui/VRGuiManager.h"

#include <iostream>

ImTreeview::ImTreeview(string ID) : ID("##"+ID), root("", ID, "", 0) {}

void ImTreeview::render() {
    root.render(0);
}

void ImTreeview::Node::renderButton() {
    if (ImGui::Button(label.c_str())) {
        isSelected = true;
        uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}});
    }
}

void ImTreeview::Node::renderEditable() {
    if (!isSelected) renderButton();
    else {
        if (!input) input = new ImInput(ID+"_input", "", label, ImGuiInputTextFlags_EnterReturnsTrue);
        if (input->render(-1)) {
            isSelected = false;
            uiSignal("treeview_rename", {{"treeview",tvID}, {"node",ID}, {"name",input->value}});
            uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}});
        }
    }
}

bool ImTreeview::Node::render(int lvl) {
    if (label == "") { // root nodes
        for (auto& child : children) child.render(lvl+1);
        return true;
    }

    if (options & IM_TV_NODE_EDITABLE) renderEditable();
    else renderButton();

    bool open = false;
    if (children.size() > 0) {
        ImGui::SameLine();
        open = ImGui::CollapsingHeader(("##"+ID).c_str(), 0);
        if (open) {
            ImGui::Indent(8*lvl);
            for (auto& child : children) child.render(lvl+1);
            ImGui::Unindent(8*lvl);
        }
    }
    return open;
}

ImTreeview::Node::Node(string ID, string tvID, string label, int options) : ID(ID), tvID(tvID), label(label), options(options) {}

ImTreeview::Node& ImTreeview::add(string ID, string label, int options, string parent) {
    Node* n = 0;
    if (nodes.count(parent)) n = &nodes[parent]->add(ID, label, options);
    else n = &root.add(ID, label, options);
    nodes[ID] = n;
    return *n;
}

ImTreeview::Node& ImTreeview::Node::add(string childID, string child, int options) {
    children.push_back(Node(childID, tvID, child, options));
    return children[children.size()-1];
}

void ImTreeview::clear() {
    root.children.clear();
    nodes.clear();
}
