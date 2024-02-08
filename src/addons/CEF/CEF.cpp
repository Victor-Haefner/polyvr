#include "CEF.h"

#ifdef PLOG
#undef PLOG
#endif

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_load_handler.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGTextureEnvChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGImage.h>

#include "CEFWindowsKey.h" // call after OpenSG includes because of strange windows boost issue

#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/utils/VRLogger.h"
#include "core/utils/system/VRSystem.h"

#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

using namespace OSG;

vector< weak_ptr<CEF> > instances;
bool cef_gl_init = false;



class CEF_handler : public CefRenderHandler, public CefLoadHandler, public CefContextMenuHandler, public CefDialogHandler, public CefDisplayHandler {
    private:
        VRTexturePtr image = 0;
        int width = 1024;
        int height = 1024;
        bool imgSetOnce = false;

    public:
        bool updateRect = false;
        Boundingbox bb;

    public:
        CEF_handler();
        ~CEF_handler();

#ifdef _WIN32
        void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
#else
        bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
#endif

        void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model);
        bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags);

        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) override;
        VRTexturePtr getImage();
        void resize(int resolution, float aspect);

        void OnLoadEnd( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, int httpStatusCode ) override;
        void OnLoadError( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl ) override;
        void OnLoadStart( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type ) override;

#ifdef _WIN32
        bool OnFileDialog( CefRefPtr< CefBrowser > browser, CefDialogHandler::FileDialogMode mode, const CefString& title, const CefString& default_file_path, const std::vector< CefString >& accept_filters, CefRefPtr< CefFileDialogCallback > callback) override;
#else
        bool OnFileDialog( CefRefPtr< CefBrowser > browser, CefDialogHandler::FileDialogMode mode, const CefString& title, const CefString& default_file_path, const std::vector< CefString >& accept_filters, int selected_accept_filter, CefRefPtr< CefFileDialogCallback > callback ) override;
#endif


        void on_link_clicked(string source, int line, string s);
        bool OnConsoleMessage( CefRefPtr< CefBrowser > browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line ) override;

        IMPLEMENT_REFCOUNTING(CEF_handler);
};

class CEF_client : public CefClient {
    private:
        CefRefPtr<CEF_handler> handler;

    public:
        CEF_client();
        ~CEF_client();

        CefRefPtr<CEF_handler> getHandler();
        CefRefPtr<CefRenderHandler> GetRenderHandler() override;
        CefRefPtr<CefLoadHandler> GetLoadHandler() override;
        CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
        CefRefPtr<CefDialogHandler> GetDialogHandler() override;
        CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;

        IMPLEMENT_REFCOUNTING(CEF_client);
};

struct CEFInternals {
    CefRefPtr<CefBrowser> browser;
    CefRefPtr<CEF_client> client;
};

CEF_handler::CEF_handler() {
    VRTextureGenerator g;
    g.setSize(Vec3i(2,2,1), false);
    g.drawFill(Color4f(0,0,0,1));
    image = g.compose();
}

CEF_handler::~CEF_handler() {
    cout << "~CEF_handler\n";
    image.reset();
}

#ifdef _WIN32
void CEF_handler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect = CefRect(0, 0, max(8,width), max(8,height)); // never give an empty rectangle!!
}
#else
bool CEF_handler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect = CefRect(0, 0, max(8, width), max(8, height)); // never give an empty rectangle!!
    return true;
}
#endif

//Disable context menu
//Define below two functions to essentially do nothing, overwriting defaults
void CEF_handler::OnBeforeContextMenu( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) {
    //CEF_REQUIRE_UI_THREAD();
    model->Clear();
}

bool CEF_handler::OnContextMenuCommand( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags) {
    //CEF_REQUIRE_UI_THREAD();
    //MessageBox(browser->GetHost()->GetWindowHandle(), L"The requested action is not supported", L"Unsupported Action", MB_OK | MB_ICONINFORMATION);
    return false;
}

void CEF_handler::on_link_clicked(string source, int line, string s) {
    auto data = splitString(source, '/');
    if (data.size() == 0) return;
    string script = data[data.size()-1];
#ifndef WITHOUT_GTK
    VRGuiManager::get()->focusScript(script, line, 0);
#endif
}

bool CEF_handler::OnConsoleMessage( CefRefPtr< CefBrowser > browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line ) {
#ifndef WITHOUT_GTK
    VRConsoleWidget::get( "Console" )->addStyle( "blueLink", "#1133ff", "#ffffff", false, false, true, false );

    auto link = VRFunction<string>::create("cef_link", bind(&CEF_handler::on_link_clicked, this, source, line, _1) );

    string msg = message;
    string src = source;
    VRConsoleWidget::get( "Console" )->write( src + " (" + toString(line) + ")", "blueLink", link );
    VRConsoleWidget::get( "Console" )->write( ": " + msg + "\n" );
    return true;
#else
    return false;
#endif
}

UInt64 setSubData(Image* img, Int32 x0, Int32 y0, Int32 z0, Int32 srcW, Int32 srcH, Int32 srcD, const UInt8 *src ) {
    UChar8* dest = img->editData();
    if (!src || !dest) return 0;

    UInt32 xMax = x0 + srcW;
    UInt32 yMax = y0 + srcH;
    UInt32 zMax = z0 + srcD;

    UInt64 changedPixels = 0;
    UInt64 lineSize = 0;
    UInt64 dataPtr = 0;
    for (UInt32 z = z0; z < zMax; z++) {
        for (UInt32 y = y0; y < yMax; y++) {
            lineSize = (xMax - x0) * img->getBpp();
            dataPtr  = ((z * img->getHeight() + y) * img->getWidth() + x0) * img->getBpp();
            memcpy (&dest[dataPtr], &src[dataPtr], size_t(lineSize));
            changedPixels += lineSize;
        }
    }

    return changedPixels;
}

void CEF_handler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height) {
    //cout << "CEF_handler::OnPaint " << image << endl;
    if (!image) return;
    auto img = image->getImage();
    if (!img) return;

    //cout << " CEF_handler::OnPaint set " << img << endl;
    if (!imgSetOnce || width != img->getWidth() || height != img->getHeight()) {
        img->set(Image::OSG_BGRA_PF, width, height, 1, 0, 1, 0.0, (const uint8_t*)buffer, Image::OSG_UINT8_IMAGEDATA, true, 1);
        imgSetOnce = true;
        updateRect = false;
        //cout << "changed all pixels!" << endl;
    } else {
        bb.clear();
        UInt64 changedPixels = 0;
        for (const CefRect& r : dirtyRects) {
            changedPixels += setSubData(img, r.x, r.y, 0, r.width, r.height, 1, (const uint8_t*)buffer);
            bb.update(Vec3d(r.x, r.y, 0));
            bb.update(Vec3d(r.x+r.width, r.y+r.height, 1));
        }
        updateRect = true;
        //cout << " changed pixels: " << changedPixels << ", or " << double(changedPixels)/(width*height) << "%" << endl;
    }
}

OSG::VRTexturePtr CEF_handler::getImage() { return image; }

void CEF_handler::resize(int resolution, float aspect) {
    width = resolution;
    height = width/aspect;
    //cout << "CEF_handler::resize" << endl;
}

void CEF_handler::OnLoadEnd( CefRefPtr< CefBrowser > browser, CefRefPtr< CefFrame > frame, int httpStatusCode ) {
    if (!frame->IsMain()) return;
    //cout << "CEF_handler::OnLoadEnd" << endl;
}

void CEF_handler::OnLoadError( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl ) {
    if (!frame->IsMain()) return;
    cout << "CEF_handler::OnLoadError, failed to load '" << failedUrl.ToString() << "' with '" << errorText.ToString() << "'" << endl;
}

void CEF_handler::OnLoadStart( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type ) {
    if (!frame->IsMain()) return;
    //cout << "CEF_handler::OnLoadStart" << endl;
}



CEF_client::CEF_client() {
    handler = new CEF_handler();
}

CEF_client::~CEF_client() {
    cout << "~CEF_client\n";
}

CefRefPtr<CefRenderHandler> CEF_client::GetRenderHandler() { return handler; }
CefRefPtr<CefLoadHandler> CEF_client::GetLoadHandler() { return handler; }
CefRefPtr<CEF_handler> CEF_client::getHandler() { return handler; }
CefRefPtr<CefContextMenuHandler> CEF_client::GetContextMenuHandler() { return handler; }
CefRefPtr<CefDialogHandler> CEF_client::GetDialogHandler() { return handler; }
CefRefPtr<CefDisplayHandler> CEF_client::GetDisplayHandler() { return handler; }

CEF::CEF() {
    global_initiate();
    internals = new CEFInternals();
    internals->client = new CEF_client();
    update_callback = VRUpdateCb::create("webkit_update", bind(&CEF::update, this));
    auto scene = VRScene::getCurrent();
    if (scene) scene->addUpdateFkt(update_callback);
}

CEF::~CEF() {
    cout << "CEF destroyed " << internals->client->HasOneRef() << " " << internals->browser->HasOneRef() << endl;
    internals->browser->GetHost()->CloseBrowser(false);
    if (internals) delete internals;
}

void CEF::shutdown() { if (!cef_gl_init) return; cout << "CEF shutdown\n"; CefShutdown(); }

CEFPtr CEF::create() {
    auto cef = CEFPtr(new CEF());
    instances.push_back(cef);
    return cef;
}

void CEF::global_initiate() {
    static bool global_init = false;
    if (global_init) return;
    global_init = true;

    cout << "Global CEF init\n";
    cef_gl_init = true;
    CefSettings settings;

#ifdef _WIN32
    string path = "/ressources/cefWin";
#elif defined(CEF18)
    string path = "/ressources/cef18";
#else
    string path = "/ressources/cef";
#endif

    auto checkPath = [](string name, string path) {
        cout << "CEF::global_initiate, " << name << " path: " << path << endl;
        if (exists(path)) cout << " found " << name << " path" << endl;
        else cout << " Warning! did not find " << name << " path!" << endl;
    };

#ifdef _WIN32
    string bsp = VRSceneManager::get()->getOriginalWorkdir() + path + "/CefSubProcessWin.exe";
#else
    string bsp = VRSceneManager::get()->getOriginalWorkdir() + path + "/CefSubProcess";
#endif

    checkPath("subprocess", bsp);

    string ldp = VRSceneManager::get()->getOriginalWorkdir() + path + "/locales";
    string rdp = VRSceneManager::get()->getOriginalWorkdir() + path;
    string lfp = VRSceneManager::get()->getOriginalWorkdir() + path + "/wblog.log";

    checkPath("locales", ldp);
    checkPath("ressources", rdp);
    checkPath("log", lfp);

    CefString(&settings.browser_subprocess_path).FromASCII(bsp.c_str());
    CefString(&settings.locales_dir_path).FromASCII(ldp.c_str());
    CefString(&settings.resources_dir_path).FromASCII(rdp.c_str());
    CefString(&settings.log_file).FromASCII(lfp.c_str());
    settings.no_sandbox = true;
#ifdef _WIN32
    settings.windowless_rendering_enabled = true;
    //settings.log_severity = LOGSEVERITY_VERBOSE;
#endif

#ifdef _WIN32
    CefMainArgs args;
	//args.set(const struct_type* src, struct_type* target, bool copy); // TODO: set parameters as defined below
#else
    vector<const char *> cmdArgs;
    cmdArgs.push_back("--enable-media-stream=1");
    cmdArgs.push_back("--use-fake-ui-for-media-stream=1");
    CefMainArgs args(cmdArgs.size(), (char**)cmdArgs.data());
#endif
    CefInitialize(args, settings, nullptr, 0);
}

void CEF::initiate() {
    init = true;
    CefWindowInfo win;
    CefBrowserSettings browser_settings;
#ifdef _WIN32
    win.SetAsWindowless(0);
    win.shared_texture_enabled = false;
#elif defined(CEF18)
    win.SetAsWindowless(0);
#else
    win.SetAsWindowless(0, true);
#endif

#ifdef _WIN32
    //requestContext = CefRequestContext::CreateContext(handler.get());
    internals->browser = CefBrowserHost::CreateBrowserSync(win, internals->client, "", browser_settings, nullptr, nullptr);
    internals->browser->GetHost()->WasResized();
#else
    internals->browser = CefBrowserHost::CreateBrowserSync(win, internals->client, "", browser_settings, 0);
#endif
}

void CEF::setMaterial(VRMaterialPtr mat) {
    if (!mat) return;
    if (!internals->client->getHandler()) return;
    this->mat = mat;
    mat->setTexture(internals->client->getHandler()->getImage());
    mat->setTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

string CEF::getSite() { return site; }
void CEF::reload() {
    cout << "CEF::reload " << site << ", " << internals->browser << endl;
    if (internals->browser) {
#ifdef _WIN32
        internals->browser->GetMainFrame()->LoadURL(site); // Reload doesnt work on windows ??
#else
        internals->browser->Reload();
#endif
        //internals->browser->ReloadIgnoreCache(); // TODO: try this out
    }
    if (auto m = mat.lock()) m->updateDeferredShader();
}

void CEF::update() {
    auto handler = internals->client->getHandler();
    if (!init || !handler) return;

    auto img = handler->getImage();
    int dim1 = img->getImage()->getDimension();
    CefDoMessageLoopWork();
    int dim2 = img->getImage()->getDimension();

    if (auto m = mat.lock()) {
        if (dim1 != dim2) m->updateDeferredShader();
        if (handler->updateRect) {
            Vec3i m1 = Vec3i(handler->bb.min());
            Vec3i m2 = Vec3i(handler->bb.max()) - Vec3i(1,1,1);
            //cout << " " << m1 << " -> " << m2 << endl;
            m->updateTexture(m1, m2);
            handler->updateRect = false;
        }
    }
}

void CEF::open(string site) {
    if (!init) initiate();
    this->site = site;
    if (internals->browser) {
        internals->browser->GetMainFrame()->LoadURL(site);
        //bool b = internals->browser->IsLoading();
#ifdef _WIN32
        internals->browser->GetHost()->WasResized();
#endif
    }
}

void CEF::resize() {
    if (!internals->client->getHandler()) return;
    internals->client->getHandler()->resize(resolution, aspect);
    if (init && internals->browser) internals->browser->GetHost()->WasResized();
    if (init) reload();
}

vector<CEFPtr> CEF::getInstances() {
    vector<CEFPtr> res;
    for (auto i : instances) {
        auto cef = i.lock();
        if (!cef) continue;
        res.push_back(cef);
    }
    return res;
}

void CEF::reloadScripts(string path) {
    for (auto i : instances) {
        auto cef = i.lock();
        if (!cef) continue;
        string s = cef->getSite();
        auto res = splitString(s, '/');
        if (res.size() == 0) continue;
        if (res[res.size()-1] == path) {
            cef->resize();
            cef->reload();
        }
    }
}

void CEF::setResolution(float a) { resolution = a; resize(); }
void CEF::setAspectRatio(float a) { aspect = a; resize(); }

// dev callbacks:

void CEF::addMouse(VRDevicePtr dev, VRObjectPtr obj, int lb, int mb, int rb, int wu, int wd) {
    if (!obj) obj = this->obj.lock();
    if (dev == 0 || obj == 0) return;
    this->obj = obj;

    auto k = dev.get();
    if (!mouse_dev_callback.count(k)) mouse_dev_callback[k] = VRFunction<VRDeviceWeakPtr, bool>::create( "CEF::MOUSE", bind(&CEF::mouse, this, lb,mb,rb,wu,wd,_1 ) );
    dev->newSignal(-1,0)->add(mouse_dev_callback[k], -1);
    dev->newSignal(-1,1)->add(mouse_dev_callback[k], -1);

    if (!mouse_move_callback.count(k)) mouse_move_callback[k] = VRUpdateCb::create( "CEF::MM", bind(&CEF::mouse_move, this, dev) );
    auto scene = VRScene::getCurrent();
    if (scene) scene->addUpdateFkt(mouse_move_callback[k]);
}

void CEF::addKeyboard(VRDevicePtr dev) {
    if (dev == 0) return;
    if (!keyboard_dev_callback) keyboard_dev_callback = VRFunction<VRDeviceWeakPtr, bool>::create( "CEF::KR", bind(&CEF::keyboard, this, _1 ) );
    dev->newSignal(-1, 0)->add( keyboard_dev_callback );
    dev->newSignal(-1, 1)->add( keyboard_dev_callback );
}

void CEF::mouse_move(VRDeviceWeakPtr d) {
    if (!focus) return;
    auto dev = d.lock();
    if (!dev) return;
    auto geo = obj.lock();
    if (!geo) return;
    VRIntersectionPtr ins = dev->intersect(geo);

    if (!ins->hit) return;
    if (ins->object.lock() != geo) return;

    CefMouseEvent me;
    me.x = ins->texel[0]*resolution;
    me.y = ins->texel[1]*(resolution/aspect);
    if (!internals->browser) return;
    auto host = internals->browser->GetHost();
    if (!host) return;
    if (me.x != mX || me.y != mY) {
        host->SendMouseMoveEvent(me, false);
        mX = me.x;
        mY = me.y;
    }
}

void CEF::toggleInput(bool m, bool k) {
    doMouse = m;
    doKeyboard = k;
}

bool CEF::mouse(int lb, int mb, int rb, int wu, int wd, VRDeviceWeakPtr d) {
    if (!doMouse) return true;
    //cout << "CEF::mouse " << lb << " " << rb << " " << wu << " " << wd << endl;
    auto dev = d.lock();
    if (!dev) return true;
    int b = dev->key();
    bool down = dev->getState();
    //cout << " CEF::mouse b: " << b << " s: " << down << endl;

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
        ss << "CEF::mouse " << this << " dev " << dev->getName();
        ss << " hit " << ins->hit << " " << o << ", trg " << geo->getName();
        ss << " b: " << b << " s: " << down;
        ss << " texel: " << ins->texel;
        ss << endl;
        VRLog::log("net", ss.str());
    }

    if (!internals->browser) return true;
    auto host = internals->browser->GetHost();
    if (!host) return true;
#ifdef _WIN32
    if (!ins->hit) { host->SetFocus(false); focus = false; return true; }
    if (iobj != geo) { host->SetFocus(false); focus = false; return true; }
    host->SetFocus(true); focus = true;
#else
    //cout << " CEF::mouse ins->hit: " << ins->hit << ", iobj != geo: " << bool(iobj != geo) << endl;
    if (!ins->hit) { host->SendFocusEvent(false); focus = false; return true; }
    if (iobj != geo) { host->SendFocusEvent(false); focus = false; return true; }
    host->SendFocusEvent(true); focus = true;
#endif

    int width = resolution;
    int height = resolution/aspect;

    CefMouseEvent me;
    me.x = ins->texel[0]*width;
    me.y = ins->texel[1]*height;

    bool blockSignals = false;

    if (b < 3) {
        cef_mouse_button_type_t mbt;
        if (b == 0) mbt = MBT_LEFT;
        if (b == 1) mbt = MBT_MIDDLE;
        if (b == 2) mbt = MBT_RIGHT;
        //cout << "CEF::mouse " << me.x << " " << me.y << " " << !down << endl;
        host->SendMouseClickEvent(me, mbt, !down, 1);
    }

    if (b == 3 || b == 4) {
        int d = b==3 ? -1 : 1;
        host->SendMouseWheelEvent(me, d*width*0.05, d*height*0.05);
        blockSignals = true; // only for scrolling
    }

    return !blockSignals;
}

bool CEF::keyboard(VRDeviceWeakPtr d) {
    if (!doKeyboard) return true;
    auto dev = d.lock();
    if (!dev) return true;
    if (!focus) return true;
    if (dev->getType() != "keyboard") return true;
    //bool down = dev->getState();
    VRKeyboardPtr keyboard = dynamic_pointer_cast<VRKeyboard>(dev);
    if (!keyboard) return true;
    auto event = keyboard->getEvent();
    if (!internals->browser) return true;
    auto host = internals->browser->GetHost();
    if (!host) return true;

    //cout << "CEF::keyboard " << event.keyval << " " << ctrlUsed << " " << keyboard->ctrlDown() << endl;

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

    CefKeyEvent kev;
    kev.modifiers = GetCefStateModifiers(keyboard->shiftDown(), keyboard->lockDown(), keyboard->ctrlDown(), keyboard->altDown(), false, false, false);
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
        //cout << " CEF::keyboard press " << event.keyval << " " << kev.native_key_code << " " << kev.character << endl;
        kev.type = KEYEVENT_RAWKEYDOWN; host->SendKeyEvent(kev);
    } else {
        //cout << " CEF::keyboard release " << event.keyval << " " << kev.native_key_code << " " << kev.character << endl;
        kev.type = KEYEVENT_KEYUP; host->SendKeyEvent(kev);
        kev.type = KEYEVENT_CHAR; host->SendKeyEvent(kev);
    }
    return false;
}

#ifdef _WIN32
bool CEF_handler::OnFileDialog(CefRefPtr< CefBrowser > browser, CefDialogHandler::FileDialogMode mode, const CefString& title, const CefString& default_file_path, const std::vector< CefString >& accept_filters, CefRefPtr< CefFileDialogCallback > callback) {
#else
bool CEF_handler::OnFileDialog( CefRefPtr< CefBrowser > browser, CefDialogHandler::FileDialogMode mode, const CefString& title, const CefString& default_file_path, const std::vector< CefString >& accept_filters, int selected_accept_filter, CefRefPtr< CefFileDialogCallback > callback ) {
#endif
    auto onAccept = [callback](){
/*#ifdef _WIN32
        callback->Continue( { VRGuiFile::getPath() } );
#else
        callback->Continue(0, { VRGuiFile::getPath() });
#endif*/
    };

    auto onCancel = [callback](){
        callback->Cancel();
    };

    // TODO
    //VRGuiFile::setCallbacks(onAccept, onCancel);
    //VRGuiFile::open("Open", 0, title);
    return true;
}

