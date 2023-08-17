#ifndef VRIMGUIEDITOR_H_INCLUDED
#define VRIMGUIEDITOR_H_INCLUDED

#include <functional>
#include <string>
#include <map>
#include <vector>

#include "VRImguiUtils.h"
#include "imWidgets/VRImguiInput.h"
#include "imWidgets/VRImguiTreeview.h"
#include <imgui.h>
#include <core/utils/VRFwdDeclTemplate.h>
#include <core/gui/VRGuiSignals.h>

using namespace std;

class ImSection : public ImWidget {
    public:
        ImRectangle layout;
        Surface surface;
        Surface parentSurface;
        ResizeEvent resizer;
        ImGuiWindowFlags flags = 0;

        ImSection(string n, ImRectangle r);
        void resize(const Surface& parent);
        void updateLayout(const Surface& newSize);

        void begin() override;
        void end() override;
};

class ImToolbar : public ImSection {
    public:
        ImToolbar(ImRectangle r);
        void begin() override;
};

class ImSidePanel : public ImSection {
    public:
        ImWidgetPtr appManager;
        ImWidgetPtr setupManager;
        ImWidgetPtr sceneEditor;

        string selected = "Apps";

        ImSidePanel(ImRectangle r);
        void begin() override;

        void openTabs(string tab1, string tab2);
};

class ImConsolesSection : public ImSection {
    public:
        ImWidgetPtr consoles;

        ImConsolesSection(ImRectangle r);
        void begin() override;
};

class ImDialog : public ImSection {
    public:
        ImDialog(string n);

        void renderFileDialog(string signal);
};

class ImNotifyDialog : public ImDialog {
    public:
        string message1;
        string message2;
        string signal;

        ImNotifyDialog();

        void open(string msg1, string msg2, string sig);
        void begin() override;
};

class ImAboutDialog : public ImDialog {
    public:
        string version;
        vector<string> authors;

        ImAboutDialog();

        void begin() override;
};

class ImFileDialog : public ImDialog {
    public:
        bool internalOpened = false;
        string sig = "ui_open_file";
        string filters = "*";
        string title = "File";
        string startDir = "~/";
        string startFile = "";
        string options = "";
        ImInput scaleInput;
        ImInput presetInput;

        ImFileDialog();
        void begin() override;
};

class ImRecorderDialog : public ImDialog {
    public:
        ImRecorderDialog();
        void begin() override;
};

class ImDocDialog : public ImDialog {
    public:
        ImInput filter;
        ImTreeview tree;
        string text;

        ImDocDialog();
        void begin() override;

        void treeClear();
        void treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col);
};

class ImSearchDialog : public ImDialog {
    public:
        ImInput searchField;
        ImInput replaceField;
        int searchScope = 0;
        string text;

        ImSearchDialog();
        void begin() override;
};

class ImProfDialog : public ImDialog {
    public:
        ImProfDialog();
        void begin() override;
};

class ImTemplateDialog : public ImDialog {
    public:
        bool initiated = false;
        ImInput filter;
        ImTreeview tree;
        string script;
        string selected;

        ImTemplateDialog();
        void begin() override;

        void clear();
        void add(string ID, string label, string parent);
        void selectTemplate(string node);
};

class ImImportDialog : public ImDialog {
    public:
        ImImportDialog();
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

        ImNotifyDialog notifyDialog = ImNotifyDialog();
        ImAboutDialog aboutDialog = ImAboutDialog();
        ImFileDialog fileDialog = ImFileDialog();
        ImRecorderDialog recorderDialog = ImRecorderDialog();
        ImDocDialog docDialog = ImDocDialog();
        ImSearchDialog searchDialog = ImSearchDialog();
        ImProfDialog profDialog = ImProfDialog();
        ImImportDialog importDialog = ImImportDialog();
        ImTemplateDialog templateDialog = ImTemplateDialog();

        void resolveResize(const string& name, const ResizeEvent& resizer);
        void handleRelayedKey(int key, int state, bool special);

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
