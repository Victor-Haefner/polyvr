#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGImage.h>


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

        CefRefPtr<CefBrowser> browser;
        CefRefPtr<CefRenderHandler> GetRenderHandler();

        void update();
        bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height);

        void mouse(int b, bool down, OSG::VRDevice* dev);
        void mouse_move(OSG::VRDevice* dev, int i);
        void keyboard(bool down, OSG::VRDevice* dev);

        IMPLEMENT_REFCOUNTING(CEF);

    public:
        CEF();
        ~CEF();

        void setResolution(float a);
        void setAspectRatio(float a);

        void setMaterial(OSG::VRMaterial* mat);
        void addMouse(OSG::VRDevice* dev, OSG::VRObject* obj, int lb, int rb, int wu, int wd);
        void addKeyboard(OSG::VRDevice* dev);

        void open(string site);
        string getSite();
        void reload();

        static void reload(string path);
};

#endif // CAVEKEEPER_H_INCLUDED
