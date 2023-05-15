#ifndef VRIMGUIEDITOR_H_INCLUDED
#define VRIMGUIEDITOR_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <vector>

#include "VRImguiUtils.h"
#include <imgui.h>
#include <core/utils/VRFwdDeclTemplate.h>
#include <core/gui/VRGuiSignals.h>

using namespace std;

class ImSection : public ImWidget {
    public:
        Rectangle layout;
        Surface surface;
        Surface parentSurface;
        ResizeEvent resizer;
        ImGuiWindowFlags flags = 0;

        ImSection(string n, Rectangle r);
        void resize(const Surface& parent);
        void updateLayout(const Surface& newSize);

        void begin() override;
        void end() override;
};

class ImToolbar : public ImSection {
    public:
        ImToolbar(Rectangle r);
        void begin() override;
};

class ImSidePanel : public ImSection {
    public:
        ImWidgetPtr appManager;
        ImWidgetPtr setupManager;
        ImWidgetPtr sceneEditor;

        ImSidePanel(Rectangle r);
        void begin() override;
};

class ImConsolesSection : public ImSection {
    public:
        ImWidgetPtr consoles;

        ImConsolesSection(Rectangle r);
        void begin() override;
};

class ImDialog : public ImSection {
    public:
        ImDialog(string n);

        void renderFileDialog(string signal);
};

class ImAboutDialog : public ImDialog {
    public:
        string version;
        vector<string> authors;

        ImAboutDialog();

        void begin() override;
};

class ImNewDialog : public ImDialog {
    public:
        ImNewDialog();
        void begin() override;
};

class ImOpenDialog : public ImDialog {
    public:
        ImOpenDialog();
        void begin() override;
};

class ImSaveasDialog : public ImDialog {
    public:
        ImSaveasDialog();
        void begin() override;
};

class ImRecorderDialog : public ImDialog {
    public:
        ImRecorderDialog();
        void begin() override;
};

class VRImguiEditor {
    private:
        Signal signal;
        ResizeSignal resizeSignal;

        ImToolbar toolbar = ImToolbar({0,1,0.95,1});
        ImSidePanel sidePanel = ImSidePanel({0,0.3,0,0.95});
        ImConsolesSection consoles = ImConsolesSection({0.3,1.0,0,0.3});
        ImSection glArea = ImSection("glArea", {0.3,1,0.3,0.95});

        ImAboutDialog aboutDialog = ImAboutDialog();
        ImNewDialog newDialog = ImNewDialog();
        ImOpenDialog openDialog = ImOpenDialog();
        ImSaveasDialog saveasDialog = ImSaveasDialog();
        ImRecorderDialog recorderDialog = ImRecorderDialog();

        void resolveResize(const string& name, const ResizeEvent& resizer);

    public:
        void init(Signal signal, ResizeSignal resizeSignal);
        void initPopup();
        void close();

        void render();
        void renderPopup(OSG::VRGuiSignals::Options o);
        void renderGLArea();
        void resizeUI(const Surface& parent);
        void resizePopup(const Surface& parent);
        void resize(const Surface& parent);
        void onSectionResize(map<string,string> options);
};

#endif // VRIMGUIEDITOR_H_INCLUDED
