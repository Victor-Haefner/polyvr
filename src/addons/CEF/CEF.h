#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGImage.h>

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"

#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;

namespace OSG{ class VRDevice; }

class CEF_handler : public CefRenderHandler {
    private:
        OSG::ImageRecPtr image = 0;
        int width = 1024;
        int height = 1024;

    public:
        CEF_handler();

        bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height);
        OSG::ImageRecPtr getImage();
        void resize(int resolution, float aspect);

        IMPLEMENT_REFCOUNTING(CEF_handler);
};

class CEF_client : public CefClient {
    private:
        CefRefPtr<CEF_handler> handler;

    public:
        CEF_client();

        CefRefPtr<CEF_handler> getHandler();
        CefRefPtr<CefRenderHandler> GetRenderHandler();

        IMPLEMENT_REFCOUNTING(CEF_client);
};

class CEF {
    private:
        string site;
        OSG::VRMaterialWeakPtr mat;
        OSG::VRObjectWeakPtr obj;
        CefRefPtr<CEF_client> client;
        int resolution = 1024;
        float aspect = 1;
        bool init = false;
        bool focus = false;

        VRUpdatePtr update_callback;
        VRUpdatePtr mouse_move_callback;
        shared_ptr<VRFunction<OSG::VRDevice*> > mouse_dev_callback;
        shared_ptr<VRFunction<OSG::VRDevice*> > keyboard_dev_callback;

        CefRefPtr<CefBrowser> browser;

        void initiate();
        void update();

        void mouse(int lb, int rb, int wu, int wd, OSG::VRDevice* dev);
        void mouse_move(OSG::VRDevice* dev, int i);
        void keyboard(OSG::VRDevice* dev);

        CEF();
    public:
        ~CEF();
        static shared_ptr<CEF> create();

        void setResolution(float a);
        void setAspectRatio(float a);

        void setMaterial(OSG::VRMaterialPtr mat);
        void addMouse(OSG::VRDevice* dev, OSG::VRObjectWeakPtr obj, int lb, int rb, int wu, int wd);
        void addKeyboard(OSG::VRDevice* dev);

        void open(string site);
        void reload();
        string getSite();
        void resize();

        static void reloadScripts(string path);
        static void shutdown();
};

typedef shared_ptr<CEF> CEFPtr;

#endif // CAVEKEEPER_H_INCLUDED
