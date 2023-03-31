#include "VRImguiEditor.h"

#include <math.h>
#include <iostream>
#include <GL/glew.h>

#include <backends/imgui_impl_glut.h>
#include <backends/imgui_impl_opengl3.h>
//(#include <imgui_internal.h>

#include <core/gui/VRGuiSignals.h>
#include <core/gui/VRGuiManager.h>
#include <core/utils/toString.h>

#include "VRImguiApps.h"

ImSection::ImSection(string n, Rectangle r) : ImWidget(n), layout(r) {
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

ImToolbar::ImToolbar(Rectangle r) : ImSection("Toolbar", r) {}

ImSidePanel::ImSidePanel(Rectangle r) : ImSection("SidePanel", r) {
    auto appMgr = new ImAppManager();
    appManager = ImWidgetPtr(appMgr);
}

void ImSidePanel::begin() {
    ImSection::begin();
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Apps")) {
            appManager->render();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Setup")) {
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scene")) {
            //editor.Render("Editor");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

ImConsoles::ImConsoles(Rectangle r) : ImSection("Consoles", r) {}

void ImToolbar::begin() {
    ImSection::begin();
    if (ImGui::Button("New")) uiSignal("toolbar_new");
    ImGui::SameLine(); if (ImGui::Button("Open")) uiSignal("toolbar_open");
    ImGui::SameLine(); if (ImGui::Button("Save")) uiSignal("toolbar_save");
    ImGui::SameLine(); if (ImGui::Button("Save..")) uiSignal("toolbar_saveas");
    ImGui::SameLine(); if (ImGui::Button("Close")) uiSignal("toolbar_close");
    ImGui::SameLine(); if (ImGui::Button("Exit")) uiSignal("toolbar_exit");
    ImGui::SameLine(); if (ImGui::Button("About")) uiSignal("toolbar_about");
    ImGui::SameLine(); if (ImGui::Button("Profiler")) uiSignal("toolbar_profiler");
    ImGui::SameLine(); if (ImGui::Button("Fullscreen")) uiSignal("toolbar_fullscreen");
}

void ImConsoles::begin() {
    ImSection::begin();
}


void VRImguiEditor::resizeUI(const Surface& parent) {
    toolbar.resize(parent);
    sidePanel.resize(parent);
    consoles.resize(parent);
    glArea.resize(parent);
    if (resizeSignal) resizeSignal("glAreaResize", glArea.surface);
}

void VRImguiEditor::onSectionResize(map<string,string> options) {
    string name = options["name"];
    char edge = options["edge"][0];
    if (name == "Toolbar" && edge == 'B') resolveResize(name, toolbar.resizer);
    if (name == "SidePanel" && (edge == 'T' || edge == 'R')) resolveResize(name, sidePanel.resizer);
    if (name == "Consoles" && (edge == 'T' || edge == 'L')) resolveResize(name, consoles.resizer);
}

void VRImguiEditor::init(Signal signal, ResizeSignal resizeSignal) {
    this->signal = signal;
    this->resizeSignal = resizeSignal;

    cout << "Imgui::init" << endl;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs();

    toolbar.signal = signal;
    sidePanel.signal = signal;
    consoles.signal = signal;
    glArea.signal = signal;
}

void VRImguiEditor::close() {
    cout << "Imgui::close" << endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void VRImguiEditor::resolveResize(const string& name, const ResizeEvent& resizer) {
    //cout << "     resolveResize " << name << ", " << resizer << endl;
    if (name == "SidePanel") {
        sidePanel.updateLayout({ resizer.pos.x, resizer.pos.y, resizer.size.x, resizer.size.y });
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
        consoles.updateLayout({ resizer.pos.x, resizer.pos.y, resizer.size.x, resizer.size.y });
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
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x < 0 || io.DisplaySize.y < 0) return;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::GetStyle().TouchExtraPadding = ImVec2(6, 6); // make DnD of section borders easier

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

