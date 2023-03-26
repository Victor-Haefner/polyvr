#include "VRImguiEditor.h"

#include <math.h>
#include <iostream>
#include <GL/glew.h>

#include <backends/imgui_impl_glut.h>
#include <backends/imgui_impl_opengl2.h>

ostream& operator<<(ostream& os, const ResizeEvent& s) {
    os << "[" << s.pos.x << ", " << s.pos.y << ", " << s.size.x << ", " << s.size.y << "]";
    return os;
}

ostream& operator<<(ostream& os, const ImVec2& s) {
    os << "[" << s.x << ", " << s.y << "]";
    return os;
}

ostream& operator<<(ostream& os, const Rectangle& s) {
    os << "[" << s.left << ", " << s.right << ", " << s.top << ", " << s.bottom << "]";
    return os;
}

ostream& operator<<(ostream& os, const Surface& s) {
    os << "[" << s.x << ", " << s.y << ", " << s.width << ", " << s.height << "]";
    return os;
}

void Surface::compute(const Surface& parent, const Rectangle& area) {
    width  = round( parent.width * (area.right - area.left) );
    height = round( parent.height * (area.top - area.bottom) );
    width = max(width, 10);
    height = max(height, 10);
    x = round( parent.width * area.left );
    y = round( parent.height * (1.0 - area.top) );
    //cout << " compute surface " << width << ", " << height << ", " << x << ", " << y << endl;
}

void Widget::updateLayout(const Surface& newSize) {
    //cout << " updateLayout " << newSize.y + newSize.height << "/800?   " << layout << ", parentSurface: " << parentSurface;
    layout.left  = float(newSize.x - parentSurface.x) / parentSurface.width;
    layout.right = float(newSize.x + newSize.width - parentSurface.x) / parentSurface.width;
    layout.top    = 1.0 - float(newSize.y - parentSurface.y) / parentSurface.height;
    layout.bottom = 1.0 - float(newSize.y + newSize.height - parentSurface.y) / parentSurface.height;
    surface.compute(parentSurface, layout);
    //cout << ", new size: " << newSize << " -> " << layout << endl;
}

void Imgui::init(Signal signal, ResizeSignal resizeSignal) {
    this->signal = signal;
    this->resizeSignal = resizeSignal;

    cout << "Imgui::init" << endl;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL2_Init();
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGLUT_Init();
    ImGui_ImplGLUT_InstallFuncs();

    toolbar.signal = signal;
    sidePannel.signal = signal;
    consoles.signal = signal;
    glArea.signal = signal;
}

void Imgui::close() {
    cout << "Imgui::close" << endl;
    ImGui_ImplOpenGL2_Shutdown();
    ImGui::DestroyContext();
}

void Imgui::resolveResize(const string& name, const ResizeEvent& resizer) {
    //cout << "     resolveResize " << name << ", " << resizer << endl;
    if (name == "SidePannel") {
        sidePannel.updateLayout({ resizer.pos.x, resizer.pos.y, resizer.size.x, resizer.size.y });
        consoles.layout.left = sidePannel.layout.right;
        consoles.resize(consoles.parentSurface);
        toolbar.layout.bottom = sidePannel.layout.top;
        toolbar.resize(toolbar.parentSurface);
        glArea.layout.left = sidePannel.layout.right;
        glArea.layout.top = sidePannel.layout.top;
        glArea.resize(glArea.parentSurface);
        resizeSignal("glAreaResize", glArea.surface);
    }

    if (name == "Consoles") {
        consoles.updateLayout({ resizer.pos.x, resizer.pos.y, resizer.size.x, resizer.size.y });
        sidePannel.layout.right = consoles.layout.left;
        sidePannel.resize(sidePannel.parentSurface);
        glArea.layout.left = consoles.layout.left;
        glArea.layout.bottom = consoles.layout.top;
        glArea.resize(glArea.parentSurface);
        resizeSignal("glAreaResize", glArea.surface);
    }
}

void Imgui::renderSidePannel() {
    sidePannel.begin();
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Apps")) {
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

    sidePannel.end();
}

void Imgui::renderToolbar() {
    toolbar.begin();
    if (ImGui::Button("New"));
    ImGui::SameLine();
    if (ImGui::Button("Open"));
    ImGui::SameLine();
    if (ImGui::Button("Save"));
    ImGui::SameLine();
    if (ImGui::Button("Save.."));
    ImGui::SameLine();
    if (ImGui::Button("Close"));
    ImGui::SameLine();
    if (ImGui::Button("Exit"));
    ImGui::SameLine();
    if (ImGui::Button("About"));
    ImGui::SameLine();
    if (ImGui::Button("Stats"));
    toolbar.end();
}

void Imgui::renderConsoles() {
    consoles.begin();
    consoles.end();
}

void Imgui::resizeUI(const Surface& parent) {
    toolbar.resize(parent);
    sidePannel.resize(parent);
    consoles.resize(parent);
    glArea.resize(parent);
    if (resizeSignal) resizeSignal("glAreaResize", glArea.surface);
}

void Imgui::resizeGL(const Surface& parent) { // on resize
    glAreaWindow = parent;
}

void Imgui::onWidgetResize(map<string,string> options) {
    string name = options["name"];
    char edge = options["edge"][0];
    if (name == "Toolbar" && edge == 'B') resolveResize(name, toolbar.resizer);
    if (name == "SidePannel" && (edge == 'T' || edge == 'R')) resolveResize(name, sidePannel.resizer);
    if (name == "Consoles" && (edge == 'T' || edge == 'L')) resolveResize(name, consoles.resizer);
}

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void Imgui::render() {
    ImGuiIO& io = ImGui::GetIO();
    cout << "  Imgui::render " << io.DisplaySize.x << ", " << io.DisplaySize.y << endl;
    if (io.DisplaySize.x < 0 || io.DisplaySize.y < 0) return;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    renderToolbar();
    renderSidePannel();
    renderConsoles();

    //ImGui::ShowDemoWindow(0);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);

    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    //cout << " Imgui::render " << (GLsizei)io.DisplaySize.x << ", " << (GLsizei)io.DisplaySize.y << endl;
}

