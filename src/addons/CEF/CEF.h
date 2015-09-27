#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGImage.h>
#include <memory>
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;

namespace OSG{
class VRMaterial; class VRDevice;
}

class CEF : public CefClient, public CefRenderHandler {
    private:
        string site;
        OSG::VRMaterialPtr mat = 0;
        OSG::VRObjectPtr obj = 0;
        OSG::ImageRecPtr image = 0;
        int width = 1024;
        int height = 1024;
        float aspect = 1;
        bool init = false;
        bool focus = false;

        VRUpdatePtr update_callback;
        VRUpdatePtr mouse_move_callback;
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

        void setMaterial(OSG::VRMaterialPtr mat);
        void addMouse(OSG::VRDevice* dev, OSG::VRObjectPtr obj, int lb, int rb, int wu, int wd);
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
