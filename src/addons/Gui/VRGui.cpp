#ifdef _WIN32
#include <GL/glew.h>
#endif

#include "VRGui.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGImage.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/utils/VRLogger.h"
#include "core/utils/system/VRSystem.h"

#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

#include <imgui.h>
#ifdef _WIN32
#include <imgui_impl_opengl3.h>
#else
#include <backends/imgui_impl_opengl3.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

using namespace OSG;

vector< weak_ptr<VRGui> > instances;
bool VRGui_gl_init = false;

ImGuiContext* uiContext = 0;
GLuint fbo, render_buf;

class VRGui_client {
    private:
        VRTexturePtr image = 0;
        int width = 1024;
        int height = 1024;

    public:
        VRGui_client();
        ~VRGui_client();

        void OnPaint(const void* buffer, int width, int height);
        VRTexturePtr getImage();
        void resize(int resolution, float aspect);
};

struct VRGuiInternals {
    VRGui_client client;
};

VRGui_client::VRGui_client() {
    VRTextureGenerator g;
    g.setSize(Vec3i(2,2,1), false);
    g.drawFill(Color4f(0,0,0,1));
    image = g.compose();
}

VRGui_client::~VRGui_client() {
    cout << "~VRGui_client" << endl;
}

void VRGui_client::OnPaint(const void* buffer, int width, int height) {
    if (!image) return;
    auto img = image->getImage();
    if (img) {
        img->set(Image::OSG_BGRA_PF, width, height, 1, 0, 1, 0.0, (const uint8_t*)buffer, Image::OSG_UINT8_IMAGEDATA, true, 1);
    }
}

OSG::VRTexturePtr VRGui_client::getImage() { return image; }

void VRGui_client::resize(int resolution, float aspect) {
    width = resolution;
    height = width/aspect;
}


VRGui::VRGui() {
    global_initiate();
    internals = new VRGuiInternals();
    update_callback = VRUpdateCb::create("webkit_update", bind(&VRGui::update, this));
    auto scene = VRScene::getCurrent();
    if (scene) scene->addUpdateFkt(update_callback);
}

VRGui::~VRGui() {
    if (internals) delete internals;
    glDeleteFramebuffers(1,&fbo);
    glDeleteRenderbuffers(1,&render_buf);
}

VRGuiPtr VRGui::create() {
    auto gui = VRGuiPtr(new VRGui());
    instances.push_back(gui);
    return gui;
}

void VRGui::global_initiate() {
    static bool global_init = false;
    if (global_init) return;
    global_init = true;
    //ImGui::Initialize();
}

void VRGui::initiate() {
    init = true;


        // Offscreen rendering setup
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a texture to render to
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Create a render buffer for depth and stencil attachment (optional)
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Check fbo completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!" << std::endl;
    }

    auto outerCtx = ImGui::GetCurrentContext();
    uiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(uiContext);
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();
    ImGui::SetCurrentContext(outerCtx);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

void VRGui::setMaterial(VRMaterialPtr mat) {
    if (!mat) return;
    this->mat = mat;
    mat->setTexture(internals->client.getImage());
    mat->setTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

string VRGui::getSite() { return site; }

void VRGui::update() {
    if (!init) return;

    auto outerCtx = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(uiContext);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // TODO: render imgui to a buffer

    ImGui::SetCurrentContext(uiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;
    if (io.DisplaySize.x < 0 || io.DisplaySize.y < 0) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(0);

    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    //glViewport(0, 0, width, height);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //drawCube();


    vector<uint8_t> buffer(width*height*4);
    glReadBuffer(GL_BACK);
    glReadPixels(0,0,width,height,GL_BGRA,GL_UNSIGNED_BYTE,&buffer[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui::SetCurrentContext(outerCtx);

    internals->client.OnPaint(&buffer[0], width, height);
}

void VRGui::open(string site) {
    if (!init) initiate();
    this->site = site;
    //if (internals->browser) internals->browser->GetMainFrame()->LoadURL(site);
}

void VRGui::resize() {
    internals->client.resize(resolution, aspect);
}

vector<VRGuiPtr> VRGui::getInstances() {
    vector<VRGuiPtr> res;
    for (auto i : instances) {
        auto VRGui = i.lock();
        if (!VRGui) continue;
        res.push_back(VRGui);
    }
    return res;
}

void VRGui::reloadScripts(string path) {
    for (auto i : instances) {
        auto VRGui = i.lock();
        if (!VRGui) continue;
        string s = VRGui->getSite();
        stringstream ss(s); vector<string> res; while (getline(ss, s, '/')) res.push_back(s); // split by ':'
        if (res.size() == 0) continue;
        if (res[res.size()-1] == path) {
            VRGui->resize();
        }
    }
}

void VRGui::setResolution(float a) { resolution = a; resize(); }
void VRGui::setAspectRatio(float a) { aspect = a; resize(); }

// dev callbacks:

void VRGui::addMouse(VRDevicePtr dev, VRObjectPtr obj, int lb, int mb, int rb, int wu, int wd) {
    if (!obj) obj = this->obj.lock();
    if (dev == 0 || obj == 0) return;
    this->obj = obj;

    auto k = dev.get();
    if (!mouse_dev_callback.count(k)) mouse_dev_callback[k] = VRFunction<VRDeviceWeakPtr, bool>::create( "VRGui::MOUSE", bind(&VRGui::mouse, this, lb,mb,rb,wu,wd,_1 ) );
    dev->newSignal(-1,0)->add(mouse_dev_callback[k], -1);
    dev->newSignal(-1,1)->add(mouse_dev_callback[k], -1);

    if (!mouse_move_callback.count(k)) mouse_move_callback[k] = VRUpdateCb::create( "VRGui::MM", bind(&VRGui::mouse_move, this, dev) );
    auto scene = VRScene::getCurrent();
    if (scene) scene->addUpdateFkt(mouse_move_callback[k]);
}

void VRGui::addKeyboard(VRDevicePtr dev) {
    if (dev == 0) return;
    if (!keyboard_dev_callback) keyboard_dev_callback = VRFunction<VRDeviceWeakPtr, bool>::create( "VRGui::KR", bind(&VRGui::keyboard, this, _1 ) );
    dev->newSignal(-1, 0)->add( keyboard_dev_callback );
    dev->newSignal(-1, 1)->add( keyboard_dev_callback );
}

void VRGui::mouse_move(VRDeviceWeakPtr d) {
    if (!focus) return;
    auto dev = d.lock();
    if (!dev) return;
    auto geo = obj.lock();
    if (!geo) return;
    VRIntersectionPtr ins = dev->intersect(geo);

    if (!ins->hit) return;
    if (ins->object.lock() != geo) return;

    /*VRGuiMouseEvent me;
    me.x = ins->texel[0]*resolution;
    me.y = ins->texel[1]*(resolution/aspect);
    if (!internals->browser) return;
    auto host = internals->browser->GetHost();
    if (!host) return;
    if (me.x != mX || me.y != mY) {
        host->SendMouseMoveEvent(me, false);
        mX = me.x;
        mY = me.y;
    }*/
}

void VRGui::toggleInput(bool m, bool k) {
    doMouse = m;
    doKeyboard = k;
}

bool VRGui::mouse(int lb, int mb, int rb, int wu, int wd, VRDeviceWeakPtr d) {
    if (!doMouse) return true;
    //cout << "VRGui::mouse " << lb << " " << rb << " " << wu << " " << wd << endl;
    auto dev = d.lock();
    if (!dev) return true;
    int b = dev->key();
    bool down = dev->getState();

    if (b == lb) b = 0;
    else if (b == mb) b = 1;
    else if (b == rb) b = 2;
    else if (b == wu) b = 3;
    else if (b == wd) b = 4;
    else return true;

    auto geo = obj.lock();
    if (!geo) return true;

    auto ins = dev->intersect(geo);
    auto iobj = ins->object.lock();

    if (VRLog::tag("net")) {
        string o = "NONE";
        if (iobj) o = iobj->getName();
        stringstream ss;
        ss << "VRGui::mouse " << this << " dev " << dev->getName();
        ss << " hit " << ins->hit << " " << o << ", trg " << geo->getName();
        ss << " b: " << b << " s: " << down;
        ss << " texel: " << ins->texel;
        ss << endl;
        VRLog::log("net", ss.str());
    }

    bool blockSignals = false;

    /*if (!internals->browser) return true;
    auto host = internals->browser->GetHost();
    if (!host) return true;
    if (!ins->hit) { host->SendFocusEvent(false); focus = false; return true; }
    if (iobj != geo) { host->SendFocusEvent(false); focus = false; return true; }
    host->SendFocusEvent(true); focus = true;

    int width = resolution;
    int height = resolution/aspect;

    VRGuiMouseEvent me;
    me.x = ins->texel[0]*width;
    me.y = ins->texel[1]*height;


    if (b < 3) {
        VRGui_mouse_button_type_t mbt;
        if (b == 0) mbt = MBT_LEFT;
        if (b == 1) mbt = MBT_MIDDLE;
        if (b == 2) mbt = MBT_RIGHT;
        //cout << "VRGui::mouse " << me.x << " " << me.y << " " << !down << endl;
        host->SendMouseClickEvent(me, mbt, !down, 1);
    }

    if (b == 3 || b == 4) {
        int d = b==3 ? -1 : 1;
        host->SendMouseWheelEvent(me, d*width*0.05, d*height*0.05);
        blockSignals = true; // only for scrolling
    }*/

    return !blockSignals;
}

bool VRGui::keyboard(VRDeviceWeakPtr d) {
    if (!doKeyboard) return true;
    auto dev = d.lock();
    if (!dev) return true;
    if (!focus) return true;
    if (dev->getType() != "keyboard") return true;
    //bool down = dev->getState();
    VRKeyboardPtr keyboard = dynamic_pointer_cast<VRKeyboard>(dev);
    if (!keyboard) return true;
    auto event = keyboard->getEvent();
    /*if (!internals->browser) return true;
    auto host = internals->browser->GetHost();
    if (!host) return true;

    cout << "VRGui::keyboard " << event.keyval << " " << ctrlUsed << " " << keyboard->ctrlDown() << endl;

    if (keyboard->ctrlDown() && event.state == 1) {
        if (event.keyval == 'a') { internals->browser->GetFocusedFrame()->SelectAll(); ctrlUsed = true; }
        if (event.keyval == 'c') { internals->browser->GetFocusedFrame()->Copy(); ctrlUsed = true; }
        if (event.keyval == 'v') { internals->browser->GetFocusedFrame()->Paste(); ctrlUsed = true; }
        return false;
    }

    if (!keyboard->ctrlDown() && event.state == 1) ctrlUsed = false;

    if (ctrlUsed && !keyboard->ctrlDown() && event.state == 0 ) {
        ctrlUsed = false;
        return false; // ignore next key up event when ctrl was used for a shortcut above!
    }

    VRGuiKeyEvent kev;
    kev.modifiers = GetVRGuiStateModifiers(keyboard->shiftDown(), keyboard->lockDown(), keyboard->ctrlDown(), keyboard->altDown(), false, false, false);
    if (event.keyval >= 1100 && event.keyval <= 1111) kev.modifiers |= EVENTFLAG_IS_KEY_PAD;
    if (kev.modifiers & EVENTFLAG_ALT_DOWN) kev.is_system_key = true;

    KeyboardCode windows_key_code = GdkEventToWindowsKeyCode(event.keyval);
    kev.windows_key_code = GetWindowsKeyCodeWithoutLocation(windows_key_code);

    kev.native_key_code = event.keyval;

    if (windows_key_code == VKEY_RETURN) kev.unmodified_character = '\r';
    else kev.unmodified_character = static_cast<int>(event.keyval);

    if (kev.modifiers & EVENTFLAG_CONTROL_DOWN) kev.character = GetControlCharacter(windows_key_code, kev.modifiers & EVENTFLAG_SHIFT_DOWN);
    else kev.character = kev.unmodified_character;

    if (event.state == 1) {
        //cout << " VRGui::keyboard press " << event.keyval << " " << kev.native_key_code << " " << kev.character << endl;
        kev.type = KEYEVENT_RAWKEYDOWN; host->SendKeyEvent(kev);
    } else {
        //cout << " VRGui::keyboard release " << event.keyval << " " << kev.native_key_code << " " << kev.character << endl;
        kev.type = KEYEVENT_KEYUP; host->SendKeyEvent(kev);
        kev.type = KEYEVENT_CHAR; host->SendKeyEvent(kev);
    }*/
    return false;
}



