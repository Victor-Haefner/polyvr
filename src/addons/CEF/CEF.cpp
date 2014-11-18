#include "CEF.h"

#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoProperties.h>

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/VROptions.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/material/VRMaterial.h"

using namespace std;
using namespace OSG;

vector<CEF*> instances;

CEF::CEF() {
    CefSettings settings;
    string bsp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/CefSubProcess";
    string ldp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/locales";
    string rdp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef";
    string lfp = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/cef/wblog.log";
    CefString(&settings.browser_subprocess_path).FromASCII(bsp.c_str());
    CefString(&settings.locales_dir_path).FromASCII(ldp.c_str());
    CefString(&settings.resources_dir_path).FromASCII(rdp.c_str());
    CefString(&settings.log_file).FromASCII(lfp.c_str());
    settings.no_sandbox = true;

    CefMainArgs args(VROptions::get()->argc, VROptions::get()->argv);
    CefInitialize(args, settings, 0, 0);

    CefWindowInfo win;
    CefBrowserSettings browser_settings;

    win.SetAsOffScreen(0);
    browser = CefBrowserHost::CreateBrowserSync(win, this, "www.google.de", browser_settings, 0);

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

void CEF::setMaterial(VRMaterial* mat) { if (mat == 0) return; this->mat = mat; mat->setTexture(image); }
void CEF::update() { CefDoMessageLoopWork(); }
void CEF::open(string site) { this->site = site; browser->GetMainFrame()->LoadURL(site); }
CefRefPtr<CefRenderHandler> CEF::GetRenderHandler() { return this; }
string CEF::getSite() { return site; }

void CEF::reload() {
    height = width/aspect;
    cout << "reload " << width << " " << height << endl;
    open(site);
}

void CEF::reload(string path) {
    for (uint i=0; i<instances.size(); i++) {
        string s = instances[i]->getSite();
        stringstream ss(s); vector<string> res; while (getline(ss, s, '/')) res.push_back(s); // split by ':'
        if (res[res.size()-1] == path) instances[i]->reload();
    }
}

void CEF::setResolution(float a) { width = a; reload(); }
void CEF::setAspectRatio(float a) { aspect = a; reload(); }

bool CEF::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect = CefRect(0, 0, width, height);
    return true;
}

void CEF::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) {
    image->set(Image::OSG_BGRA_PF, width, height, 1, 0, 1, 0.0, (const uint8_t*)buffer, Image::OSG_UINT8_IMAGEDATA, true, 1);
}

void CEF::addMouse(VRDevice* dev, VRObject* obj, int lb, int rb, int wu, int wd) {
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
    dev->addSignal(-1, 0)->add( new VRFunction<VRDevice*>( "CEF::KP", boost::bind(&CEF::keyboard, this, 0, _1 ) ) );
    //dev->addSignal(-1, 1)->add( new VRFunction<VRDevice*>( "CEF::KR", boost::bind(&CEF::keyboard, this, 1, _1 ) ) );
}

void CEF::mouse_move(VRDevice* dev, int i) {
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
    browser->GetHost()->SendFocusEvent();*/

    VRIntersection ins = dev->intersect(obj);

    /*string o = "NONE";
    if (ins.object) o = ins.object->getName();
    cout << "CEF::mouse " << this;
    cout << " hit " << ins.hit << " " << o << ", trg " << obj->getName();
    cout << " b: " << b << " state: " << down;
    cout << " texel: " << ins.texel;
    cout << endl;*/

    if (!ins.hit) return;
    if (ins.object != obj) return;

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

    if (b == 3 or b == 4) {
        int d = b==3 ? -1 : 1;
        browser->GetHost()->SendMouseWheelEvent(me, d*width*0.05, d*height*0.05);
    }
}

void CEF::keyboard(bool down, VRDevice* dev) {
    char k = dev->key();

    CefKeyEvent ke;
    ke.type = KEYEVENT_CHAR;
    ke.modifiers = EVENTFLAG_NONE;
    ke.windows_key_code = k;
    ke.native_key_code = k;
    //ke.is_system_key = k;
    ke.unmodified_character = k;
    //ke.focus_on_editable_field = ;
    ke.character = k;

    cout << "KEY " << dev->key() << endl;
    browser->GetHost()->SendKeyEvent(ke);
}
