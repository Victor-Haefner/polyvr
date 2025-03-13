#ifndef VRIMGUIUTILS_H_INCLUDED
#define VRIMGUIUTILS_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <vector>

//#include "imEditor/TextEditor.h"
#include <imgui.h>
#include "core/tools/VRToolsFwd.h"

using namespace std;

struct ImRectangle {
    float left = 0;
    float right = 1;
    float bottom = 0;
    float top = 1;
};

struct Surface {
    int x = 0;
    int y = 0;
    int width = 10;
    int height = 10;

    void compute(const Surface& parent, const ImRectangle& area);
};

struct ResizeEvent {
    ImVec2 size;
    ImVec2 pos;

    vector<char> changed();
};

typedef function<void(string, map<string, string>)> Signal;
typedef function<void(string, Surface)> ResizeSignal;

ptrFwd(ImWidget);

class ImWidget {
    public:
        string name;
        Signal signal;

        ImWidget(string n);
        virtual ~ImWidget();

        void render();
        virtual void begin() = 0;
        virtual void end();
};

class ImImage {
    public:
        string path;
        unsigned int glID = 0;
        int width = 0;
        int height = 0;

    public:
        ImImage();
        ~ImImage();

        void read(string path);
        void render(int w, int h);
};

ImVec4 colorFromString(const string& c);
void pushGlowBorderStyle(int ID);
void popGlowBorderStyle();

float uiStrScale();
float uiStrWidth(const string& s);

void uiInitStore();
void uiCloseStore();
void uiStoreParameter(string name, string value);
string uiGetParameter(string name, string def);


#endif // VRIMGUIUTILS_H_INCLUDED
