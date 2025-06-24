#include "VRImguiEditor.h"

#include <math.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>

#ifdef _WIN32
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>
#else
#include <backends/imgui_impl_glut.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#include "core/gui/VRGuiSignals.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/toString.h"
#include "core/tools/VRProjectManager.h"

#include "VRImguiApps.h"
#include "VRImguiConsoles.h"
#include "VRImguiSetup.h"
#include "VRImguiScene.h"
#include "VRImguiProfiler.h"
#include "imFileDialog/ImGuiFileDialog.h"
#include "../clipboard/clip.h"
#include "../VRGuiManager.h"

void updateGlutCursor() {
    auto mc = ImGui::GetMouseCursor();
    if (mc == ImGuiMouseCursor_None) glutSetCursor(GLUT_CURSOR_NONE);
    if (mc == ImGuiMouseCursor_Arrow) glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
    if (mc == ImGuiMouseCursor_TextInput) glutSetCursor(GLUT_CURSOR_TEXT);
    if (mc == ImGuiMouseCursor_ResizeAll) glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
    if (mc == ImGuiMouseCursor_ResizeNS) glutSetCursor(GLUT_CURSOR_UP_DOWN);
    if (mc == ImGuiMouseCursor_ResizeEW) glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
    if (mc == ImGuiMouseCursor_ResizeNESW) glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
    if (mc == ImGuiMouseCursor_ResizeNWSE) glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
    if (mc == ImGuiMouseCursor_Hand) glutSetCursor(GLUT_CURSOR_INFO);
    if (mc == ImGuiMouseCursor_NotAllowed) glutSetCursor(GLUT_CURSOR_DESTROY);
}

ImGuiContext* mainContext = 0;
ImGuiContext* popupContext = 0;

ImSection::ImSection(string n, ImRectangle r) : ImWidget(n), layout(r) {
    resize({0,0,800,800});

    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoScrollbar;
    //flags |= ImGuiWindowFlags_MenuBar;
    flags |= ImGuiWindowFlags_NoMove;
    //flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoNav;
    //flags |= ImGuiWindowFlags_NoBackground;
    //flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    //flags |= ImGuiWindowFlags_UnsavedDocument;
}

void ImSection::begin() {
    //cout << " place widget " << surface << endl;
    ImGui::SetNextWindowPos(ImVec2(surface.x, surface.y)); // ImGuiCond_FirstUseEver
    ImGui::SetNextWindowSize(ImVec2(surface.width, surface.height));
    ImGui::Begin(name.c_str(), NULL, flags);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigWindowsResizeFromEdges = true;
    io.MouseDragThreshold = 30;

    for (char edge: resizer.changed()) {
        string sedge = string( 1, edge );
        signal("uiSectionResize", {{"name",name},{"edge",sedge}} );
    }
}

void ImSection::end() {
    ImGui::End();
}

void ImSection::updateLayout(const Surface& newSize) {
    //cout << " updateLayout " << newSize.y + newSize.height << "/800?   " << layout << ", parentSurface: " << parentSurface;
    layout.left  = float(newSize.x - parentSurface.x) / parentSurface.width;
    layout.right = float(newSize.x + newSize.width - parentSurface.x) / parentSurface.width;
    layout.top    = 1.0 - float(newSize.y - parentSurface.y) / parentSurface.height;
    layout.bottom = 1.0 - float(newSize.y + newSize.height - parentSurface.y) / parentSurface.height;
    surface.compute(parentSurface, layout);
    //cout << ", new size: " << newSize << " -> " << layout << endl;
}

void ImSection::resize(const Surface& parent) {
    parentSurface = parent;
    surface.compute(parent, layout);
    resizer.pos = ImVec2(surface.x, surface.y);
    resizer.size = ImVec2(surface.width, surface.height);
}

ImToolbar::ImToolbar(ImRectangle r) : ImSection("Toolbar", r) {}

ImSidePanel::ImSidePanel(ImRectangle r) : ImSection("SidePanel", r) {
    appManager = ImWidgetPtr(new ImAppManager());
    setupManager = ImWidgetPtr(new ImSetupManager());
    sceneEditor = ImWidgetPtr(new ImSceneEditor());

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("openUiTabs", [&](OSG::VRGuiSignals::Options o){ selected = o["tab1"]; return true; } );
}

//uiSignal("openUiTabs", {{"tab1", "Scene"}, {"tab2", "Scripting"}});
void ImSidePanel::openTabs(string tab1, string tab2) {
    ;
}

void ImSidePanel::begin() {
    auto setCurrentTab = [&](string t) {
        if (t != currentTab) {
            currentTab = t;
            uiSignal("ui_change_main_tab", {{"tab",currentTab}});
            //cout << "  setCurrentTab " << currentTab << " -> " << t << endl;
        }
    };

    ImSection::begin();
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

    ImGuiTabItemFlags flags1, flags2, flags3;
    flags1 = flags2 = flags3 = 0;

    if (selected != "") {
        if (selected == "Apps") flags1 = ImGuiTabItemFlags_SetSelected;
        if (selected == "Setup") flags2 = ImGuiTabItemFlags_SetSelected;
        if (selected == "Scene") flags3 = ImGuiTabItemFlags_SetSelected;
        selected = "";
    }

    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Apps", NULL, flags1)) {
            appManager->render();
            ImGui::EndTabItem();
            setCurrentTab("Apps");
        }

        if (ImGui::BeginTabItem("Setup", NULL, flags2)) {
            setupManager->render();
            ImGui::EndTabItem();
            setCurrentTab("Setup");
        }

        if (ImGui::BeginTabItem("Scene", NULL, flags3)) {
            sceneEditor->render();
            ImGui::EndTabItem();
            setCurrentTab("Scene");
        }
        ImGui::EndTabBar();
    }
}

ImConsolesSection::ImConsolesSection(ImRectangle r) : ImSection("Consoles", r) {
    consoles = ImWidgetPtr(new ImConsoles());
}

void ImToolbar::begin() {
    ImSection::begin();

    if (ImGui::Button("New")) {
        string filters = "PolyVR Project (.pvr .pvc){.pvr,.pvc,.xml}";
        uiSignal("set_file_dialog_signal", {{"signal","ui_new_file"}});
        uiSignal("set_file_dialog_filter", {{"filter",filters}});
        uiSignal("set_file_dialog_setup", {{"title","Choose File"}, {"dir","."}, {"file","myApp\.pvr"}});
        uiSignal("ui_toggle_popup", {{"name","file"},{"title","Choose Filename"}, {"width","600"}, {"height","500"}});
    }

    ImGui::SameLine();
    if (ImGui::Button("Open")) {
        string filters = "PolyVR Project (.pvr .pvc){.pvr,.pvc,.xml}";
        filters += ",Mesh Model (.dae .wrl .obj .3ds .ply){.dae,.wrl,.obj,.3ds,.3DS,.ply}";
        filters += ",CAD Model (.step .ifc .dxf){.STEP,.STP,.step,.stp,.ifc,.dxf}";
        filters += ",Pointcloud (.e57 .xyz){.e57,.xyz}";
        filters += ",Geo Data (.hgt .tiff .pdf .shp){.hgt,.tif,.tiff,.pdf,.shp}";
        uiSignal("set_file_dialog_signal", {{"signal","ui_open_file"}});
        uiSignal("set_file_dialog_filter", {{"filter",filters}});
        uiSignal("set_file_dialog_setup", {{"title","Open File"}, {"dir","."}, {"file",""}});
        uiSignal("ui_toggle_popup", {{"name","file"},{"title","Choose File"}, {"width","600"}, {"height","500"}});
    }

    ImGui::SameLine(); if (ImGui::Button("Save")) uiSignal("toolbar_save");

    ImGui::SameLine();
    if (ImGui::Button("Save..")) {
        string filters = "PolyVR Project (.pvr .pvc){.pvr,.pvc,.xml}";
        uiSignal("set_file_dialog_signal", {{"signal","ui_saveas_file"}});
        uiSignal("set_file_dialog_filter", {{"filter",filters}});
        uiSignal("set_file_dialog_setup", {{"title","Save As.."}, {"dir","."}, {"file","myApp.pvr"}});
        uiSignal("ui_toggle_popup", {{"name","file"},{"title","Save As.."}, {"width","600"}, {"height","500"}});
    }

    ImGui::SameLine(); if (ImGui::Button("Export")) uiSignal("toolbar_export");
    ImGui::SameLine(); if (ImGui::Button("Close")) uiSignal("toolbar_close");
    ImGui::SameLine(); if (ImGui::Button("Exit")) uiSignal("toolbar_exit");
    ImGui::SameLine(); if (ImGui::Button("About")) uiSignal("ui_toggle_popup", {{"name","about"},{"title","About"}, {"width","400"}, {"height","500"}});
    ImGui::SameLine(); if (ImGui::Button("Profiler")) uiSignal("ui_toggle_popup", {{"name","profiler"},{"title","Profiler"}, {"width","600"}, {"height","500"}});
    ImGui::SameLine(); if (ImGui::Button("Recorder")) uiSignal("ui_toggle_popup", {{"name","recorder"},{"title","Recorder"}, {"width","400"}, {"height","200"}});
    ImGui::SameLine(); if (ImGui::Checkbox("Fotomode", &fotomode)) uiSignal("ui_toggle_fotomode", {{"active",toString(fotomode)}});
}

void ImConsolesSection::begin() {
    ImSection::begin();
    consoles->render();
}


void VRImguiEditor::resizeUI(const Surface& parent) {
    ImGui::SetCurrentContext(mainContext);
    toolbar.resize(parent);
    sidePanel.resize(parent);
    consoles.resize(parent);
    glArea.resize(parent);
    if (resizeSignal) resizeSignal("glAreaResize", glArea.surface);
}

void VRImguiEditor::resizePopup(const Surface& parent) {
    ImGui::SetCurrentContext(popupContext);
    notifyDialog.resize(parent);
    aboutDialog.resize(parent);
    fileDialog.resize(parent);
    recorderDialog.resize(parent);
    docDialog.resize(parent);
    searchDialog.resize(parent);
    profDialog.resize(parent);
    importDialog.resize(parent);
    templateDialog.resize(parent);
    ImGui_ImplGLUT_ReshapeFunc(parent.width, parent.height);
}

void VRImguiEditor::onSectionResize(map<string,string> options) {
    ImGui::SetCurrentContext(mainContext);
    string name = options["name"];
    char edge = options["edge"][0];
    if (name == "Toolbar" && edge == 'B') resolveResize(name, toolbar.resizer);
    if (name == "SidePanel" && (edge == 'T' || edge == 'R')) resolveResize(name, sidePanel.resizer);
    if (name == "Consoles" && (edge == 'T' || edge == 'L')) resolveResize(name, consoles.resizer);
}

void handleMouseWheel(int b, int s) {
    ImGuiIO& io = ImGui::GetIO();
    if (s == 1) {
        if (b == 3) io.MouseWheel += 1;
        if (b == 4) io.MouseWheel -= 1;
    }
}

void handleSpecial(int b, int s) { // TODO: for some reason the imgui state is inverted..
    //cout << "handleSpecial " << b << ", " << s << endl;
    ImGuiIO& io = ImGui::GetIO();
    if (b == 112) io.KeyShift = s;
    if (b == 113) io.KeyShift = s;
    if (b == 114) io.KeyCtrl = s;
    if (b == 115) io.KeyCtrl = s;
    if (b == 116) io.KeyAlt = s;
}

void checkSpecials() {
    ImGuiIO& io = ImGui::GetIO();
    auto mods = glutGetModifiers();
    io.KeyCtrl = (mods & GLUT_ACTIVE_CTRL);
    io.KeyShift = (mods & GLUT_ACTIVE_SHIFT);
    io.KeyAlt = (mods & GLUT_ACTIVE_ALT);
}

struct Utf8Handler {
    vector<unsigned char> parts;
    bool active = false;

    bool checkByte(unsigned char c) { // TODO: maybe handle continuation bytes?
        if ((c & 0xC0) == 0xC0 && (c & 0x20) == 0) { // start byte
            if (!active) parts.clear();
            parts.push_back(c);
            active = true;
            return true;
        }

        if (active) {
            parts.push_back(c);
            active = false;
            return true;
        }

        return false;
    }

    string str() {
        return string(parts.begin(), parts.end());
    }
};

void ImGui_ImplGLUT_KeyboardUpFunc_main(unsigned char c, int x, int y) {
    static Utf8Handler utf8Handler;
    if (utf8Handler.checkByte(c)) {
        if (utf8Handler.active) return;
    }

    //printf("imgui key up %i\n", c);
    uiSignal("relayedImguiKeySignal", {{"key",toString((int)c)},{"state",toString(0)}});
    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplGLUT_KeyboardUpFunc(c,x,y);
}

void ImGui_ImplGLUT_KeyboardFunc_main(unsigned char c, int x, int y) {
    static Utf8Handler utf8Handler;
    if (utf8Handler.checkByte(c)) {
        if (!utf8Handler.active) {
            //printf("imgui utf8 key down %s\n", utf8Handler.str().c_str());
            ImGui::SetCurrentContext(mainContext);
            checkSpecials();
            ImGui::GetIO().AddInputCharactersUTF8( utf8Handler.str().c_str() );
        }
        return;
    }

    //printf("imgui key down %i\n", c);
    if (c == 27) uiSignal("ui_close_popup");
    ImGui::SetCurrentContext(mainContext);
    checkSpecials();
    ImGui_ImplGLUT_KeyboardFunc(c, x, y);
}

void ImGui_ImplGLUT_SpecialUpFunc_main(int k, int x, int y) {
    //printf("imgui special up %i\n", k);
    uiSignal("relayedImguiSpecialKeySignal", {{"key",toString(k)},{"state",toString(0)}});
    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplGLUT_SpecialUpFunc(k,x,y);
    handleSpecial(k,0);
}

void ImGui_ImplGLUT_SpecialFunc_main(int k, int x, int y) { ImGui::SetCurrentContext(mainContext); checkSpecials(); ImGui_ImplGLUT_SpecialFunc(k,x,y); handleSpecial(k,1); }
void ImGui_ImplGLUT_ReshapeFunc_main(int x, int y) { ImGui::SetCurrentContext(mainContext); ImGui_ImplGLUT_ReshapeFunc(x,y); }
void ImGui_ImplGLUT_MotionFunc_main(int x, int y) { updateGlutCursor(); ImGui::SetCurrentContext(mainContext); ImGui_ImplGLUT_MotionFunc(x, y); }
void ImGui_ImplGLUT_MouseFunc_main(int b, int s, int x, int y) {
    ImGui::SetCurrentContext(mainContext);
    checkSpecials();
    ImGui_ImplGLUT_MouseFunc(b,s,x,y);
    handleMouseWheel(b,s);
    uiSignal("uiGrabFocus", {});
}

void ImGui_ImplGLUT_KeyboardFunc_popup(unsigned char c, int x, int y) {
    static Utf8Handler utf8Handler;
    if (utf8Handler.checkByte(c)) {
        if (!utf8Handler.active) {
            //printf("imgui utf8 key down %s\n", utf8Handler.str().c_str());
            ImGui::SetCurrentContext(popupContext);
            checkSpecials();
            ImGui::GetIO().AddInputCharactersUTF8( utf8Handler.str().c_str() );
        }
        return;
    }

    if (c == 27) uiSignal("ui_close_popup");
    ImGui::SetCurrentContext(popupContext);
    checkSpecials();
    ImGui_ImplGLUT_KeyboardFunc(c, x, y);
}

void ImGui_ImplGLUT_KeyboardUpFunc_popup(unsigned char c, int x, int y) {
    static Utf8Handler utf8Handler;
    if (utf8Handler.checkByte(c)) {
        if (utf8Handler.active) return;
    }

    ImGui::SetCurrentContext(popupContext);
    ImGui_ImplGLUT_KeyboardUpFunc(c,x,y);
}

void ImGui_ImplGLUT_SpecialFunc_popup(int k, int x, int y) { ImGui::SetCurrentContext(popupContext); checkSpecials(); ImGui_ImplGLUT_SpecialFunc(k,x,y); handleSpecial(k,1); }
void ImGui_ImplGLUT_SpecialUpFunc_popup(int k, int x, int y) { ImGui::SetCurrentContext(popupContext); ImGui_ImplGLUT_SpecialUpFunc(k,x,y); handleSpecial(k,0); }
void ImGui_ImplGLUT_MouseFunc_popup(int b, int s, int x, int y) { ImGui::SetCurrentContext(popupContext); checkSpecials(); ImGui_ImplGLUT_MouseFunc(b,s,x,y); handleMouseWheel(b,s); }
//void ImGui_ImplGLUT_ReshapeFunc_popup(int x, int y) { ImGui::SetCurrentContext(popupContext); ImGui_ImplGLUT_ReshapeFunc(x,y); }
void ImGui_ImplGLUT_MotionFunc_popup(int x, int y) { updateGlutCursor(); ImGui::SetCurrentContext(popupContext); ImGui_ImplGLUT_MotionFunc(x,y); }

void ImGui_ImplGLUT_InstallFuncs_main() {
    glutReshapeFunc(ImGui_ImplGLUT_ReshapeFunc_main);
    glutMotionFunc(ImGui_ImplGLUT_MotionFunc_main);
    glutPassiveMotionFunc(ImGui_ImplGLUT_MotionFunc_main);
    glutMouseFunc(ImGui_ImplGLUT_MouseFunc_main);
    glutKeyboardFunc(ImGui_ImplGLUT_KeyboardFunc_main);
    glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc_main);
    glutSpecialFunc(ImGui_ImplGLUT_SpecialFunc_main);
    glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc_main);
}

void ImGui_ImplGLUT_InstallFuncs_popup() {
    //glutReshapeFunc(ImGui_ImplGLUT_ReshapeFunc_popup);
    glutMotionFunc(ImGui_ImplGLUT_MotionFunc_popup);
    glutPassiveMotionFunc(ImGui_ImplGLUT_MotionFunc_popup);
    glutMouseFunc(ImGui_ImplGLUT_MouseFunc_popup);
    glutKeyboardFunc(ImGui_ImplGLUT_KeyboardFunc_popup);
    glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc_popup);
    glutSpecialFunc(ImGui_ImplGLUT_SpecialFunc_popup);
    glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc_popup);
}

void IMGUISetClipboardText(void* user_data, const char* text) {
    //cout << "IMGUISetClipboardText " << text << endl;
    if (text) clip::set_text(text);
}

const char* IMGUIGetClipboardText(void* user_data) {
    //cout << "IMGUIGetClipboardText " << endl;
    if (clip::has(clip::text_format())) {
        static string value;
        clip::get_text(value);
        //cout << " IMGUIGetClipboardText " << value << endl;
        return value.c_str();
    }
    else return 0;
}

void VRImguiEditor::handleRelayedKey(int key, int state, bool special) {
    if (special) {
        if (state) ImGui_ImplGLUT_SpecialFunc_main(key, 0, 0);
        else ImGui_ImplGLUT_SpecialUpFunc_main(key, 0, 0);
    } else {
        unsigned char c = key;
        if (state) ImGui_ImplGLUT_KeyboardFunc_main(c, 0, 0);
        else ImGui_ImplGLUT_KeyboardUpFunc_main(c, 0, 0);
    }
}

void VRImguiEditor::init(Signal signal, ResizeSignal resizeSignal) {
    this->signal = signal;
    this->resizeSignal = resizeSignal;

    cout << "Imgui::init" << endl;
    IMGUI_CHECKVERSION();
    mainContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs_main();

    ImGuiIO& io = ImGui::GetIO();
    io.SetClipboardTextFn = IMGUISetClipboardText;
    io.GetClipboardTextFn = IMGUIGetClipboardText;

    toolbar.signal = signal;
    sidePanel.signal = signal;
    consoles.signal = signal;
    glArea.signal = signal;
    fileDialog.signal = signal;
    recorderDialog.signal = signal;
    docDialog.signal = signal;
    searchDialog.signal = signal;
    profDialog.signal = signal;
    importDialog.signal = signal;
    templateDialog.signal = signal;

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("relayedKeySignal", [&](OSG::VRGuiSignals::Options o){ handleRelayedKey(toInt(o["key"]), toInt(o["state"]), false); return true; } );
    mgr->addCallback("relayedSpecialKeySignal", [&](OSG::VRGuiSignals::Options o){ handleRelayedKey(toInt(o["key"]), toInt(o["state"]), true); return true; } );

    uiInitStore();
    toValue(uiGetParameter("fontScale", "1.0"), io.FontGlobalScale);

    string theme;
    toValue(uiGetParameter("uiTheme", "dark"), theme);
    if (theme == "dark") ImGui::StyleColorsDark();
    if (theme == "light") ImGui::StyleColorsLight();
    if (theme == "classic") ImGui::StyleColorsClassic();
}

void VRImguiEditor::initPopup() {
    popupContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(popupContext);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs_popup();

    ImGuiIO& io = ImGui::GetIO();
    io.SetClipboardTextFn = IMGUISetClipboardText;
    io.GetClipboardTextFn = IMGUIGetClipboardText;

    ImGui::SetCurrentContext(mainContext);
}

void VRImguiEditor::close() {
    cout << "Imgui::close" << endl;

    if (popupContext) {
        ImGui::SetCurrentContext(popupContext);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    if (mainContext) {
        ImGui::SetCurrentContext(mainContext);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
}

void VRImguiEditor::resolveResize(const string& name, const ResizeEvent& resizer) {
    ImGui::SetCurrentContext(mainContext);

    //cout << "     resolveResize " << name << ", " << resizer << endl;
    if (name == "SidePanel") {
        sidePanel.updateLayout({ int(resizer.pos.x), int(resizer.pos.y), int(resizer.size.x), int(resizer.size.y) });
        consoles.layout.left = sidePanel.layout.right;
        consoles.resize(consoles.parentSurface);
        toolbar.layout.bottom = sidePanel.layout.top;
        toolbar.resize(toolbar.parentSurface);
        glArea.layout.left = sidePanel.layout.right;
        glArea.layout.top = sidePanel.layout.top;
        glArea.resize(glArea.parentSurface);
        resizeSignal("glAreaResize", glArea.surface);
    }

    if (name == "Consoles") {
        consoles.updateLayout({ int(resizer.pos.x), int(resizer.pos.y), int(resizer.size.x), int(resizer.size.y) });
        sidePanel.layout.right = consoles.layout.left;
        sidePanel.resize(sidePanel.parentSurface);
        glArea.layout.left = consoles.layout.left;
        glArea.layout.bottom = consoles.layout.top;
        glArea.resize(glArea.parentSurface);
        resizeSignal("glAreaResize", glArea.surface);
    }
}

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void VRImguiEditor::render() {
    ImGui::SetCurrentContext(mainContext);
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x < 0 || io.DisplaySize.y < 0) return;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::GetStyle().TouchExtraPadding = ImVec2(3, 3); // make DnD of section borders easier


    toolbar.render();
    sidePanel.render();
    consoles.render();
    //ImGui::ShowDemoWindow(0);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void VRImguiEditor::renderPopup(OSG::VRGuiSignals::Options options) {
    float fontScale = 1.0;
    {
        ImGuiIO& io = ImGui::GetIO();
        fontScale = io.FontGlobalScale;
    }

    ImGui::SetCurrentContext(popupContext);
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x < 0 || io.DisplaySize.y < 0) return;
    io.FontGlobalScale = fontScale;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    string name = options["name"];
    if (name == "notify") notifyDialog.render();
    if (name == "about") aboutDialog.render();
    if (name == "file") fileDialog.render();
    if (name == "recorder") recorderDialog.render();
    if (name == "documentation") docDialog.render();
    if (name == "search") searchDialog.render();
    if (name == "profiler") profDialog.render();
    if (name == "import") importDialog.render();
    if (name == "template") templateDialog.render();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImDialog::ImDialog(string n) : ImSection(n, {0,1,0,1}) {
    flags |= ImGuiWindowFlags_NoResize;
}

ImAboutDialog::ImAboutDialog() : ImDialog("about") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("setAboutVersion", [&](OSG::VRGuiSignals::Options o){ version = o["version"]; return true; } );
    mgr->addCallback("addAboutAuthors", [&](OSG::VRGuiSignals::Options o){ authors.push_back(o["author"]); return true; } );
}

void centeredText(string txt) {
    ImGuiStyle& style = ImGui::GetStyle();
    float size = uiStrWidth(txt);
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - size) * 0.5;
    if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
    ImGui::Text(txt.c_str());
}

void ImAboutDialog::begin() {
    ImSection::begin();

    centeredText("PolyVR");
    ImGui::Spacing();
    centeredText("Version:");
    centeredText(version);
    ImGui::Spacing();
    centeredText("Authors:");
    for (auto& a : authors) centeredText(a);
}

ImNotifyDialog::ImNotifyDialog() : ImDialog("notify") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("askUser", [&](OSG::VRGuiSignals::Options o){ open(o["msg1"], o["msg2"], o["sig"]); return true; } );
}


ImSearchDialog::ImSearchDialog() : ImDialog("search")
                                    , searchField("scriptSearch", "Search:", "", ImGuiInputTextFlags_EnterReturnsTrue)
                                    , replaceField("scriptReplace", "Replace:", "", ImGuiInputTextFlags_EnterReturnsTrue) {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("ui_search_set", [&](OSG::VRGuiSignals::Options o){ searchField.value = o["string"]; return true; } );
}

ImRecorderDialog::ImRecorderDialog() : ImDialog("recorder") {}
ImImportDialog::ImImportDialog() : ImDialog("import") {}

ImProfDialog::ImProfDialog() : ImDialog("profiler") {
    profiler = ImWidgetPtr(new ImProfiler());
}

ImTemplateDialog::ImTemplateDialog() : ImDialog("template"), filter("templSearch", "Search:", ""), tree("templTree") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("script_templates_clear", [&](OSG::VRGuiSignals::Options o){ clear(); return true; } );
    mgr->addCallback("on_script_template_tree_append", [&](OSG::VRGuiSignals::Options o){ add(o["ID"], o["label"], o["parent"]); return true; } );
    mgr->addCallback("treeview_select", [&](OSG::VRGuiSignals::Options o){ if(o["treeview"] == "templTree") selectTemplate(o["node"]); return true; } );
    mgr->addCallback("set_script_template", [&](OSG::VRGuiSignals::Options o){ script = o["script"]; return true; } );
}

void ImTemplateDialog::selectTemplate(string node) {
    uiSignal("select_script_template", {{"ID", node}});
    selected = node;
}

void ImTemplateDialog::clear() {
    tree.clear();
}

void ImTemplateDialog::add(string ID, string label, string parent) {
    tree.add(ID, label, 0, parent);
}

ImDocDialog::ImDocDialog() : ImDialog("documentation"), filter("docSearch", "Search:", ""), tree("docTree") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("on_change_doc_text", [&](OSG::VRGuiSignals::Options o){ text = o["text"]; return true; } );
    mgr->addCallback("on_doc_filter_tree_clear", [&](OSG::VRGuiSignals::Options o){ treeClear(); return true; } );
    mgr->addCallback("on_doc_filter_tree_append", [&](OSG::VRGuiSignals::Options o) {
            treeAppend(o["ID"], o["label"], o["parent"], o["type"], o["cla"], o["mod"], o["col"]);
        return true; } );
}

ImFileDialog::ImFileDialog() : ImDialog("file"), scaleInput("geoScale", "Scale:", "1.0"), presetInput("geoPreset", "Preset:", "OSG") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("set_file_dialog_filter", [&](OSG::VRGuiSignals::Options o){ filters = ".*,"+o["filter"]; return true; } );
    mgr->addCallback("set_file_dialog_signal", [&](OSG::VRGuiSignals::Options o){ sig = o["signal"]; return true; } );
    mgr->addCallback("set_file_dialog_setup", [&](OSG::VRGuiSignals::Options o){ title = o["title"]; startDir = o["dir"]; startFile = o["file"]; return true; } );
    mgr->addCallback("set_file_dialog_options", [&](OSG::VRGuiSignals::Options o){ options = o["options"]; return true; } );
    mgr->addCallback("ui_close_popup", [&](OSG::VRGuiSignals::Options o) { close(); return true; });
}

IGFD::FileDialog* imGuiFileDialogInstance = 0;


void ImFileDialog::close() {
    if (imGuiFileDialogInstance) {
        imGuiFileDialogInstance->Close();
        delete imGuiFileDialogInstance;
        imGuiFileDialogInstance = 0;
        internalOpened = false;
    }
}

void ImFileDialog::begin() {
    if (!internalOpened) {
        internalOpened = true;
        imGuiFileDialogInstance = new IGFD::FileDialog();
        imGuiFileDialogInstance->OpenDialog("ChooseFileDlgKey", title.c_str(), filters.c_str(), startDir.c_str(), startFile.c_str());
    }

    bool doGeoOpts = bool(options == "geoOpts");

    ImSection::begin();
    ImVec2 minSize = ImGui::GetWindowSize();
    if (doGeoOpts) minSize.y *= 0.9;
    ImGui::SetNextWindowPos(ImVec2(surface.x, surface.y));
    if (imGuiFileDialogInstance->Display("ChooseFileDlgKey", flags, minSize, minSize)) {
        if (imGuiFileDialogInstance->IsOk()) {
            std::string fileName = imGuiFileDialogInstance->GetFilePathName();
            std::string filePath = imGuiFileDialogInstance->GetCurrentPath();
            signal(sig, {{"fileName",fileName},{"filePath",filePath}});
        }

        signal("ui_close_popup", {});
    }

    if (doGeoOpts) {
        ImVec2 winSize = ImGui::GetWindowSize();
        winSize.y *= 0.1;
        ImGui::SetNextWindowPos(ImVec2(0, minSize.y));
        ImGui::SetNextWindowSize(winSize);
        //ImGui::Begin("##geoOpts", ImVec2(minSize.x,0), false, flags);
        ImGui::Begin("##geoOpts", 0, flags);
        if (scaleInput.render(-1)) uiSignal("on_change_import_scale", {{"scale",scaleInput.value}});
        if (presetInput.render(-1)) uiSignal("on_change_import_preset", {{"preset",presetInput.value}});
        //ImGui::SameLine();
        ImGui::End();
    }
}

void ImRecorderDialog::begin() {
    ImSection::begin();

    centeredText("PolyVR");
    ImGui::Spacing();
    centeredText("Version:");
    //centeredText(version);
    ImGui::Spacing();
    centeredText("Authors:");
    //for (auto& a : authors) centeredText(a);
}

void ImDocDialog::treeClear() { tree.clear(); }

void ImDocDialog::treeAppend(string ID, string label, string parent, string type, string cla, string mod, string col) {
    //cout << "treeAppend, ID: " << ID << ", parent: " << parent << endl;
    //tree.add([parent].push_back({label, type, cla, mod, col});
    tree.add(ID, label, 0, parent);
}

void ImDocDialog::begin() {
    ImSection::begin();
    centeredText("Documentation");

    if (filter.render(-1)) uiSignal("on_change_doc_filter", {{"filter",filter.value}});

    //uiSignal("on_change_doc_selection", {{"obj","Geometry"},{"type","class"},{"class","Geometry"},{"mod","VR"}});

    auto region1 = ImGui::GetContentRegionAvail();
    auto region2 = ImGui::GetContentRegionAvail();
    region1.x *= 0.3;
    region2.x *= 0.7;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    // doc mod tree
    ImGui::BeginChild("docTree", region1, false, flags);
    tree.render();
    ImGui::EndChild();

    ImGui::SameLine();

    // doc text
    //ImGui::BeginChild("docText", region2, false, flags);
    ImGui::InputTextMultiline("docText", &text[0], text.size(), region2, ImGuiInputTextFlags_ReadOnly);
    //ImGui::Text(text.c_str());
    //ImGui::EndChild();

}

void ImSearchDialog::begin() {
    ImSection::begin();
    centeredText("Find");

    bool filterEnter = searchField.render(-1);
    bool replaceEnter = replaceField.render(-1);

    static string scope = "script";
    if (ImGui::RadioButton("Script", &searchScope, 0)) scope = "script";
    if (ImGui::RadioButton("Project", &searchScope, 1)) scope = "project";

    bool clickSearch = ImGui::Button("Search");

    if (filterEnter || replaceEnter || clickSearch) {
        uiSignal("ui_close_popup");
        uiSignal("on_run_script_search", {{"search",searchField.value}, {"scope",scope}, {"replace",replaceField.value}});
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) uiSignal("ui_close_popup");
}

void ImNotifyDialog::open(string msg1, string msg2, string sig) {
    message1 = msg1;
    message2 = msg2;
    signal = sig;
    uiSignal("ui_toggle_popup", {{"name","notify"},{"title","Notification"}, {"width","400"}, {"height","200"}});
}

void ImNotifyDialog::begin() {
    ImSection::begin();
    centeredText(message1);
    centeredText(message2);
    if (ImGui::Button("Ok")) {
        uiSignal("ui_close_popup");
        uiSignal(signal);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) uiSignal("ui_close_popup");
}

void ImProfDialog::begin() {
    ImSection::begin();
    centeredText("Profiler");
    profiler->render();
}

void ImImportDialog::begin() {
    ImSection::begin();
    centeredText("Import");
}

void ImTemplateDialog::begin() {
    ImSection::begin();
    centeredText("Script Templates");

    bool filterChanged = filter.render(-1);
    if (!initiated || filterChanged) {
        uiSignal("ui_request_script_templates", {{"filter", filter.value}});
        initiated = true;
    }

    auto region1 = ImGui::GetContentRegionAvail();
    auto region2 = ImGui::GetContentRegionAvail();
    region1.x *= 0.25;
    region2.x *= 0.75;
    region1.y -= 50;
    region2.y -= 50;

    ImGui::BeginChild("templatesTree", region1, false, ImGuiWindowFlags_None);
    tree.render();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::InputTextMultiline("templatesScriptView", &script[0], script.size(), region2, ImGuiInputTextFlags_ReadOnly);


    if (ImGui::Button("Ok")) {
        uiSignal("ui_close_popup");
        uiSignal("import_script_template", {{"ID", selected}});
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) uiSignal("ui_close_popup");
}







