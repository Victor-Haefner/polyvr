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
        struct Trigger {
            string name;
            string trigger;
            string parameter;
            string device;
            string key;
            string state;
        };

        struct Argument {
            string name;
            string type;
            string value;
        };

        bool sensitive = false;
        TextEditor imEditor;
        int current_type = 0;
        int current_group = 0;
        map<string, string> groups;
        vector<string> groupList;
        vector<string> typeList;
        vector<string> argumentTypes;
        vector<string> triggerTypes;
        vector<string> device_types;
        vector<string> trigger_states;
        vector<Trigger> triggers;
        vector<Argument> arguments;

        void setBuffer(string data);
        void getBuffer(int skipLines);

        void setParameters(string type, string group);

        void clearGroups();
        void clearTrigsAndArgs();
        void addGroup(string name, string ID);
        void addTrigger(string name, string trigger, string parameter, string device, string key, string state);
        void addArgument(string name, string type, string value);

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
