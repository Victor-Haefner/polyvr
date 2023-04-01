#ifndef VRIMGUISCENESCRIPTING_H_INCLUDED
#define VRIMGUISCENESCRIPTING_H_INCLUDED

#include "VRImguiUtils.h"
#include "imEditor/TextEditor.h"

using namespace std;

class ImScriptList {
    public:
        ImScriptList();
        void render();
};

class ImScriptEditor {
    private:
        TextEditor imEditor;

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
