#ifndef VRIMGUISCENESCRIPTING_H_INCLUDED
#define VRIMGUISCENESCRIPTING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imEditor/TextEditor.h"

using namespace std;

struct ImScriptGroup {
    vector<string> scripts;
    string name;
    ImScriptGroup() {}
    ImScriptGroup(string name);
};

class ImScriptList {
    private:
        map<string, ImScriptGroup> groups;
        vector<string> groupsList;
        string selected;

        void clear();
        void addGroup(string name, string ID);
        void addScript(string name, string groupID);
        void renderScriptEntry(string& script);
        void renderGroupEntry(string& group);

    public:
        ImScriptList();
        void render();
};

class ImScriptEditor {
    private:
        TextEditor imEditor;
        int current_type = 0;
        int current_group = 0;
        map<string, string> groups;
        vector<string> groupList;
        vector<string> typeList;

        void setBuffer(string data);
        void getBuffer(int skipLines);

        void setParameters(string type, string group);

        void clear();
        void addGroup(string name, string ID);

    public:
        ImScriptEditor();
        void render();
};

class ImScripting {
    private:
        bool perf = false;
        bool pause = false;

    public:
        ImScriptList scriptlist;
        ImScriptEditor editor;

        ImScripting();
        void render();
};

#endif // VRIMGUISCENESCRIPTING_H_INCLUDED
