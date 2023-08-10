#ifndef VRIMGUISCENESCRIPTING_H_INCLUDED
#define VRIMGUISCENESCRIPTING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imEditor/TextEditor.h"

using namespace std;

class ImInput;

struct ImScriptEntry {
    string name;
    string fg = "#000000";
    string bg = "#FFFFFF";
    float perf = 0;
    ImScriptEntry() {}
    ImScriptEntry(string name);
};

struct ImScriptGroup {
    vector<ImScriptEntry> scripts;
    string name;
    ImScriptGroup() {}
    ImScriptGroup(string name);
};

class ImScriptList {
    public:
        bool doPerf = false;
        float width = 50;

    private:
        map<string, ImScriptGroup> groups;
        vector<string> groupsList;
        string selected;
        ImInput* input = 0;

        void clear();
        void addGroup(string name, string ID);
        void addScript(string name, string groupID, float time);
        void setColor(string name, string fg, string bg);
        void renderScriptEntry(ImScriptEntry& script);
        void renderGroupEntry(string& group);

    public:
        ImScriptList();
        void render();
        void computeMinWidth();
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
        void editorCommand(string cmd);
        void focusOn(string line, string column);
        void handleShiftTab(int state);

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
