#ifndef VRIMGUICOLORPICKER_H_INCLUDED
#define VRIMGUICOLORPICKER_H_INCLUDED

#include <imgui.h>
#include <string>
#include <vector>

using namespace std;

class ImColorPicker {
    public:
        string ID;
        string label;
        ImVec4 color;

    public:
        ImColorPicker(string ID, string label);

        bool render();
        void signal(string s);

        void set(string s);
        string get();
};

#endif // VRIMGUICOLORPICKER_H_INCLUDED
