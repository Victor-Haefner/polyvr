#ifndef VRIMGUIINPUT_H_INCLUDED
#define VRIMGUIINPUT_H_INCLUDED

#include <imgui.h>
#include <string>

using namespace std;

class ImInput {
    public:
        string ID;
        string label;
        string value;
        int flags = 0;

    public:
        ImInput(string ID, string label, string value, int flags = 0);

        bool render();
};

#endif // VRIMGUIINPUT_H_INCLUDED
