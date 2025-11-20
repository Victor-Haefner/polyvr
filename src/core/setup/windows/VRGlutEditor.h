#ifndef VRGLUTEDITOR_H_INCLUDED
#define VRGLUTEDITOR_H_INCLUDED


#include "VRWindow.h"
#include <OpenSG/OSGGLUTWindow.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGlutEditor: public VRWindow {
    public:
        enum CONTEXT {
            TOP = 0,
            SCENE,
            IMGUI,
            POPUP,
        };

    private:
        GLUTWindowMTRecPtr win;
        int topWin = -1;
        int winGL = -1;
        int winUI = -1;
        int winPopup = -1;
        VRHeadMountedDisplayPtr hmd;
        bool fullscreen = false;
        bool maximized = false;
        int focusedWinID = -1;
        string popup;
        string iconPath;

        typedef function<void(string, map<string, string>)> Signal;
        typedef function<void(string, int, int, int, int)> ResizeSignal;
        Signal signal;
        ResizeSignal resizeSignal;

        void getsFocus(int wID);
        void handleRelayedKey(int key, int state, bool special);

    public:
        VRGlutEditor();
        ~VRGlutEditor();

        static VRGlutEditorPtr create();
        VRGlutEditorPtr ptr();

        static void initGlut();

        int getCurrentWinID();
        int getWinID(CONTEXT c);
        void setCurrentWinID(int i);

        void setTitle(string title) override;
        void setIcon(string iconpath) override;
        void saveSnapshot(string path);

        void onMain_Keyboard_special(int k);

        void openPopupWindow(string name, string title, int width, int height);
        void togglePopupWindow(string name, string title, int width, int height);
        void closePopupWindow();

        void render(bool fromThread = false) override;

        void save(XMLElementPtr node) override;
        void load(XMLElementPtr node) override;

        void onMouse(int b, int s, int x, int y);
        void onMotion(int x, int y);
        void onKeyboard(int k, int s, int x, int y);
        void onKeyboard_special(int k, int s, int x, int y);

        void forceGLResize(int w, int h);
        void enableVSync(bool b);
        void setFullscreen(bool b);
        void setMaximized(bool b);

        void resizeGLWindow(int x, int y, int w, int h);
        void on_resize_window(int w, int h);
        void on_close_window();
        void on_ui_display();
        void on_gl_display();
        void on_popup_display();
        void on_ui_resize(int w, int h);
        void on_gl_resize(int w, int h);
        void on_popup_resize(int w, int h);
        void on_popup_close();
};

OSG_END_NAMESPACE;

#endif // VRGLUTEDITOR_H_INCLUDED
