#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGImage.h>
#include <memory>
#include "core/utils/VRFunctionFwd.h"

using namespace std;

namespace OSG{ class VRMaterial; class VRDevice; class VRObject; }

class CEF : public CefClient, public CefRenderHandler {
    private:
        string site;
        OSG::VRMaterial* mat = 0;
        OSG::VRObject* obj = 0;
        OSG::ImageRecPtr image = 0;
        int width = 1024;
        int height = 1024;
        float aspect = 1;
        bool init = false;
        bool focus = false;

        shared_ptr<VRFunction<int> > update_callback;
        shared_ptr<VRFunction<int> > mouse_move_callback;
        shared_ptr<VRFunction<OSG::VRDevice*> > mouse_dev_callback;
        shared_ptr<VRFunction<OSG::VRDevice*> > keyboard_dev_callback;

        CefRefPtr<CefBrowser> browser;
        CefRefPtr<CefRenderHandler> GetRenderHandler();

        void initiate();
        void update();
        bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height);

        void mouse(int lb, int rb, int wu, int wd, OSG::VRDevice* dev);
        void mouse_move(OSG::VRDevice* dev, int i);
        void keyboard(OSG::VRDevice* dev);

        IMPLEMENT_REFCOUNTING(CEF);

        CEF();
    public:
        ~CEF();
        static shared_ptr<CEF> create();

        void setResolution(float a);
        void setAspectRatio(float a);

        void setMaterial(OSG::VRMaterial* mat);
        void addMouse(OSG::VRDevice* dev, OSG::VRObject* obj, int lb, int rb, int wu, int wd);
        void addKeyboard(OSG::VRDevice* dev);

        void open(string site);
        void reload();
        string getSite();
        void resize();

        static void reloadScripts(string path);
};

typedef shared_ptr<CEF> CEFPtr;

#endif // CAVEKEEPER_H_INCLUDED
