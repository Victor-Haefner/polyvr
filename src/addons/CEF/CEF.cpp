#include "CEF.h"
#include "CEFWindowsKey.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRLogger.h"

using namespace std;
using namespace OSG;

vector<CEF*> instances;

CEF::CEF() {
    VRFunction<int>* fkt = new VRFunction<int>("webkit_update", boost::bind(&CEF::update, this));
    VRSceneManager::getCurrent()->addUpdateFkt(fkt);
    image = Image::create();
    instances.push_back(this);
}

CEF::~CEF() {
    browser = 0;
    CefShutdown();

    instances.erase( remove(instances.begin(), instances.end(), this), instances.end() );
}

void CEF::initiate() {
    init = true;
    cout << "CEF init " << endl;
    CefSettings settings;
#ifndef _WIN32
    string bsp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/CefSubProcess";
#else
	string bsp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/CefSubProcessWin.exe";
#endif
    string ldp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/locales";
    string rdp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef";
    string lfp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/wblog.log";
    CefString(&settings.browser_subprocess_path).FromASCII(bsp.c_str());
    CefString(&settings.locales_dir_path).FromASCII(ldp.c_str());
    CefString(&settings.resources_dir_path).FromASCII(rdp.c_str());
    CefString(&settings.log_file).FromASCII(lfp.c_str());
    settings.no_sandbox = true;

    CefMainArgs args;
    CefInitialize(args, settings, 0, 0);

    CefWindowInfo win;
    CefBrowserSettings browser_settings;

    /* NOT COMPILING?
    go to polyvr/dependencies
    > sudo git pull origin master
    > sudo gdebi -n ubuntu_14.04/libcef-dev.deb
    */

    win.SetAsWindowless(0, true);
    //win.SetTransparentPainting(true);
    browser = CefBrowserHost::CreateBrowserSync(win, this, "www.google.de", browser_settings, 0);
}

void CEF::setMaterial(VRMaterial* mat) { if (mat == 0) return; this->mat = mat; mat->setTexture(image); }
void CEF::update() { if (init) CefDoMessageLoopWork(); }
CefRefPtr<CefRenderHandler> CEF::GetRenderHandler() { return this; }
string CEF::getSite() { return site; }
void CEF::reload() { open(site); }

void CEF::open(string site) {
    if (!init) initiate();
    this->site = site;
    browser->GetMainFrame()->LoadURL(site);
}

void CEF::resize() {
    height = width/aspect;
    if (init) browser->GetHost()->WasResized();
    if (init) reload();
}

void CEF::reloadScripts(string path) {
    for (auto i : instances) {
        string s = i->getSite();
        stringstream ss(s); vector<string> res; while (getline(ss, s, '/')) res.push_back(s); // split by ':'
        if (res.size() == 0) continue;
        if (res[res.size()-1] == path) {
            i->resize();
            i->reload();
        }
    }
}

void CEF::setResolution(float a) { width = a; resize(); }
void CEF::setAspectRatio(float a) { aspect = a; resize(); }

// inherited CEF callbacks:

bool CEF::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect = CefRect(0, 0, width, height);
    return true;
}

void CEF::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) {
    image->set(Image::OSG_BGRA_PF, width, height, 1, 0, 1, 0.0, (const uint8_t*)buffer, Image::OSG_UINT8_IMAGEDATA, true, 1);
}

void CEF::addMouse(VRDevice* dev, VRObject* obj, int lb, int rb, int wu, int wd) {
    if (dev == 0 || obj == 0) return;
    this->obj = obj;

    dev->addSignal(lb, 0)->add( new VRFunction<VRDevice*>( "CEF::LMBP", boost::bind(&CEF::mouse, this, 0, 0, _1 ) ) );
    dev->addSignal(lb, 1)->add( new VRFunction<VRDevice*>( "CEF::LMBR", boost::bind(&CEF::mouse, this, 0, 1, _1 ) ) );
    dev->addSignal(rb, 0)->add( new VRFunction<VRDevice*>( "CEF::RMBP", boost::bind(&CEF::mouse, this, 2, 0, _1 ) ) );
    dev->addSignal(rb, 1)->add( new VRFunction<VRDevice*>( "CEF::RMBR", boost::bind(&CEF::mouse, this, 2, 1, _1 ) ) );
    dev->addSignal(wu, 1)->add( new VRFunction<VRDevice*>( "CEF::WU", boost::bind(&CEF::mouse, this, 3, 0, _1 ) ) );
    dev->addSignal(wd, 1)->add( new VRFunction<VRDevice*>( "CEF::WD", boost::bind(&CEF::mouse, this, 4, 0, _1 ) ) );

    VRFunction<int>* fkt = new VRFunction<int>( "CEF::MM", boost::bind(&CEF::mouse_move, this, dev, _1) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt);
}

void CEF::addKeyboard(VRDevice* dev) {
    if (dev == 0) return;
    dev->addSignal(-1, 0)->add( new VRFunction<VRDevice*>( "CEF::KR", boost::bind(&CEF::keyboard, this, 0, _1 ) ) );
    dev->addSignal(-1, 1)->add( new VRFunction<VRDevice*>( "CEF::KP", boost::bind(&CEF::keyboard, this, 1, _1 ) ) );
}

void CEF::mouse_move(VRDevice* dev, int i) {
    if (dev == 0) return;
    VRIntersection ins = dev->intersect(obj);

    if (!ins.hit) return;
    if (ins.object != obj) return;

    CefMouseEvent me;
    me.x = ins.texel[0]*width;
    me.y = ins.texel[1]*height;
    browser->GetHost()->SendMouseMoveEvent(me, dev->b_state(dev->key()));
}


void CEF::mouse(int b, bool down, VRDevice* dev) {
    /*browser->GetHost()->SendCaptureLostEvent();
    */

    VRIntersection ins = dev->intersect(obj);

    if (VRLog::tag("net")) {
        string o = "NONE";
        if (ins.object) o = ins.object->getName();
        stringstream ss;
        ss << "CEF::mouse " << this << " dev " << dev->getName();
        ss << " hit " << ins.hit << " " << o << ", trg " << obj->getName();
        ss << " b: " << b << " s: " << down;
        ss << " texel: " << ins.texel;
        ss << endl;
        VRLog::log("net", ss.str());
    }


    if (!ins.hit) { browser->GetHost()->SendFocusEvent(false); focus = false; return; }
    if (ins.object != obj) { browser->GetHost()->SendFocusEvent(false); focus = false; return; }
    browser->GetHost()->SendFocusEvent(true); focus = true;

    CefMouseEvent me;
    me.x = ins.texel[0]*width;
    me.y = ins.texel[1]*height;

    if (b < 3) {
        cef_mouse_button_type_t mbt;
        if (b == 0) mbt = MBT_LEFT;
        if (b == 1) mbt = MBT_MIDDLE;
        if (b == 2) mbt = MBT_RIGHT;
        browser->GetHost()->SendMouseClickEvent(me, mbt, !down, 1);
    }

    if (b == 3 || b == 4) {
        int d = b==3 ? -1 : 1;
        browser->GetHost()->SendMouseWheelEvent(me, d*width*0.05, d*height*0.05);
    }
}

void CEF::keyboard(bool down, VRDevice* dev) {
    if (!focus) return;
    if (dev->getType() != "keyboard") return;
    VRKeyboard* kb = (VRKeyboard*)dev;
    auto event = kb->getGtkEvent();
    CefRefPtr<CefBrowserHost> host = browser->GetHost();

    CefKeyEvent kev;
    kev.modifiers = GetCefStateModifiers(event->state);
    if (event->keyval >= GDK_KP_Space && event->keyval <= GDK_KP_9) kev.modifiers |= EVENTFLAG_IS_KEY_PAD;
    if (kev.modifiers & EVENTFLAG_ALT_DOWN) kev.is_system_key = true;

    KeyboardCode windows_key_code = GdkEventToWindowsKeyCode(event);
    kev.windows_key_code = GetWindowsKeyCodeWithoutLocation(windows_key_code);

    kev.native_key_code = event->keyval;

    if (windows_key_code == VKEY_RETURN) kev.unmodified_character = '\r'; else
    kev.unmodified_character = static_cast<int>(gdk_keyval_to_unicode(event->keyval));

    if (kev.modifiers & EVENTFLAG_CONTROL_DOWN) kev.character = GetControlCharacter(windows_key_code, kev.modifiers & EVENTFLAG_SHIFT_DOWN); else
    kev.character = kev.unmodified_character;

    if (event->type == GDK_KEY_PRESS) {
        kev.type = KEYEVENT_RAWKEYDOWN; host->SendKeyEvent(kev);
    } else {
        kev.type = KEYEVENT_KEYUP; host->SendKeyEvent(kev);
        kev.type = KEYEVENT_CHAR; host->SendKeyEvent(kev);
    }
}
