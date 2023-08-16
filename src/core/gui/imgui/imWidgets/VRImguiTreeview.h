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
            vector<Node*> children;

            Node() {}
            Node(string ID, string tvID, string label, int options);
            Node* add(string childID, string child, int options);
            bool render(int lvl = 0);
            void renderButton();
            void renderEditable();
        };

    public:
        string ID;
        string selected;
        Node root;
        map<string, Node*> nodes;

        void handleSelection(string node);

    public:
        ImTreeview(string ID);

        Node* add(string ID, string label, int options, string parent = "");
        void rename(string ID, string label);
        void render();
        void clear();
};

#endif // VRIMGUITREEVIEW_H_INCLUDED
