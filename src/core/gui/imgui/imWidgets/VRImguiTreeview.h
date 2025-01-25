#ifndef VRIMGUITREEVIEW_H_INCLUDED
#define VRIMGUITREEVIEW_H_INCLUDED

#include <imgui.h>
#include <string>
#include <vector>
#include <map>

using namespace std;

enum IM_TV_OPT {
    IM_TV_NODE_EDITABLE = 1
};

class ImInput;

class ImTreeview {
    public:
        struct Node {
            string ID;
            string tvID;
            string label;
            ImInput* input = 0;
            int options;
            bool isSelected = false;
            bool isDragged = false;
            vector<Node*> children;
            vector<pair<string, string>> menu;
            ImGuiTreeNodeFlags nodeFlags = 0;

            Node() {}
            Node(string ID, string tvID, string label, int options);
            Node* add(string childID, string child, int options);
            void setMenu(vector<pair<string, string>> menu);
            bool render(int lvl = 0);
            void renderButton();
            void renderEditable();
            void renderMenu();
        };

    public:
        string ID;
        string selected;
        Node root;
        map<string, Node*> nodes;
        ImGuiTreeNodeFlags nodeFlags = 0;

        void handleSelection(string node);

    public:
        ImTreeview(string ID);

        void setNodeFlags(ImGuiTreeNodeFlags flags);
        Node* add(string ID, string label, int options, string parent = "");
        void rename(string ID, string label);
        void render();
        void clear();
        void expandAll();
};

#endif // VRIMGUITREEVIEW_H_INCLUDED
