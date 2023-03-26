#ifndef VRIMGUIMANAGER_H_INCLUDED
#define VRIMGUIMANAGER_H_INCLUDED

#include <map>
#include <string>
#include <functional>

#include "VRImguiEditor.h"

using namespace std;

class Editor {
    private:
        typedef map<string, string> Options;
        typedef function< bool(Options) > Callback;
        typedef function< bool(Surface) > ResizeCallback;
        map<string, vector<Callback>> callbacks;
        map<string, vector<ResizeCallback>> resizeCallbacks;

        Imgui imgui;

        bool verbose = false;

        void runMainLoop();

    public:
        Editor();
        ~Editor();

        void setupCallbacks();
        void initImgui();

        void addCallback(string name, Callback callback);
        void addResizeCallback(string name, ResizeCallback callback);
        bool trigger(string name, Options options = {});
        bool triggerResize(string name, Surface surface);



        void setVerbose(bool b);
};

#endif // VRIMGUIMANAGER_H_INCLUDED
