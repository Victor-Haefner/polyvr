#include "VRImguiTreeview.h"
#include "VRImguiInput.h"
#include "../VRImguiUtils.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"

#include <iostream>
#include <algorithm>

ImTreeview::ImTreeview(string ID) : ID(ID), root("", ID, "", 0) {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("on_tv_node_rename", [&](OSG::VRGuiSignals::Options o){ if (o["treeview"] == this->ID) rename(o["node"], o["name"]); return true; } );

    selection.onDeselect = [&](string s) {
        if (nodes.count(s)) nodes[s]->isSelected = false;
    };
}

void ImTreeview::render() {
    root.render(selection, 0);
}

void ImTreeview::rename(string ID, string label) {
    if (nodes.count(ID)) nodes[ID]->label = label;
}

void ImTreeview::Node::setMenu(vector<pair<string, string>> m) { menu = m; }

void ImTreeview::Node::handleSelection(ImTreeview::Selection& selection, string node) {
    ImGuiIO& io = ImGui::GetIO();
    bool ShiftDown = io.KeyShift; // TODO, use it below!
    bool CtrlDown = io.KeyCtrl;

    if (CtrlDown) selection.add(node);
    else selection.set(node);
    isSelected = true;
}

void ImTreeview::Node::renderMenu() {
    if (menu.size() == 0) return;
    string idPopup = "menu##" + ID;
    if (ImGui::BeginPopupContextItem(idPopup.c_str())) {
        for (auto& m : menu) {
            string idOpt = m.first + "##opt_" + ID;
            if (ImGui::MenuItem(idOpt.c_str())) {
                uiSignal(m.second, {{"treeview",tvID}, {"node",label}, {"ID",ID}, {"option", m.first}});
            }
        }
        ImGui::EndPopup();
    }
}

void ImTreeview::Node::renderButton(ImTreeview::Selection& selection) {
    string idLbl = label + "##" + ID;

    if (isSelected) {
		ImGui::PushStyleColor(ImGuiCol_Text, colorFromString("#FFFFFF"));
		ImGui::PushStyleColor(ImGuiCol_Border, colorFromString("#66AAFF"));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3);
    }

    if (ImGui::Button(idLbl.c_str())) {
        //isSelected = true;
        //cout << "ImTreeview::Node::renderButton " << tvID << ", " << ID << ", " << label << ", N selected: " << selection.selected.size() << endl;
        handleSelection(selection, ID);
        string sel = toString(selection.selected);
        uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}, {"selection",sel}});
    }

    if (isSelected) {
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
    }

    renderMenu();
}

void ImTreeview::Node::renderEditable(ImTreeview::Selection& selection) {
    if (!isSelected) renderButton(selection);
    else {
        if (!input) input = new ImInput(ID+"_input", "", label, ImGuiInputTextFlags_EnterReturnsTrue);
        if (input->render(0)) {
            //cout << "ImTreeview::Node::renderEditable " << tvID << ", " << ID << endl;

            handleSelection(selection, ID);
            string sel = toString(selection.selected);
            uiSignal("treeview_rename", {{"treeview",tvID}, {"node",ID}, {"name",input->value}});
            uiSignal("treeview_select", {{"treeview",tvID}, {"node",ID}, {"selection",sel}});
        }
        renderMenu();
    }
}

bool ImTreeview::Node::render(ImTreeview::Selection& selection, int lvl) {
    if (label == "") { // root nodes
        for (auto& child : children) child->render(selection, lvl+1);
        return true;
    }

    if (options & IM_TV_NODE_EDITABLE) renderEditable(selection);
    else renderButton(selection);

    if (ImGui::BeginDragDropSource()) {
        isDragged = true;
        ImGui::SetDragDropPayload(tvID.c_str(), ID.c_str(), ID.size() + 1);
        renderButton(selection);
        ImGui::EndDragDropSource();
    } else isDragged = false;

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload( tvID.c_str() )) {
            string sID = string( static_cast<const char*>(payload->Data) );
            uiSignal("treeview_drop", {{"treeview",tvID}, {"source",sID}, {"target",ID}});
        }
        ImGui::EndDragDropTarget();
    }

    bool open = false;
    if (children.size() > 0) {
        ImGui::SameLine();
        open = ImGui::CollapsingHeader(("##"+ID).c_str(), nodeFlags);
        if (open) {
            ImGui::Indent(8);
            for (auto& child : children) child->render(selection, lvl+1);
            ImGui::Unindent(8);
        }
    }
    return open;
}

ImTreeview::Node::Node(string ID, string tvID, string label, int options) : ID(ID), tvID(tvID), label(label), options(options) {}

bool ImTreeview::Selection::has(string s) {
    return bool( find(selected.begin(), selected.end(), s) != selected.end() );
}

void ImTreeview::Selection::clear() {
    for (auto s : selected) onDeselect(s);
    selected.clear();
}

void ImTreeview::Selection::set(string s) {
    for (auto s : selected) onDeselect(s);
    selected = { s };
}

void ImTreeview::Selection::add(string s) {
    selected.push_back(s);
}

void ImTreeview::Selection::rem(string s) {
    selected.erase(find(selected.begin(), selected.end(), s));
    onDeselect(s);
}

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
