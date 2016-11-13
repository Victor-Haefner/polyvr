#ifndef VRWINDOW_H_INCLUDED
#define VRWINDOW_H_INCLUDED

#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGRenderAction.h>


#include "../devices/VRMouse.h"
#include "../devices/VRKeyboard.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRWindow;
class VRThread;
typedef boost::function<void (VRWindowPtr, int, int, int, int, int)> VRWindowCallback; // params: device, button, state, mouse x, mouse y

class VRWindow : public std::enable_shared_from_this<VRWindow>, public VRName {
    protected:
        bool active = false;
        bool content = false;
        int type = -1;
        WindowRecPtr _win;
        RenderActionRefPtr ract;
        vector<VRViewWeakPtr> views;

        VRMousePtr mouse = 0;
        VRKeyboardPtr keyboard = 0;
        int width = 640;
        int height = 480;

        VRThreadCbPtr winThread;
        int thread_id = -1;
        void update( VRThreadWeakPtr t );

    public:
        VRWindow();
        virtual ~VRWindow();

        static VRWindowPtr create();
        VRWindowPtr ptr();

        bool hasType(int i);
        void resize(int w, int h);
        Vec2i getSize();

        void setAction(RenderActionRefPtr ract);

        static unsigned int active_window_count;

        bool isActive();
        void setActive(bool b);

        bool hasContent();
        void setContent(bool b);

        void setMouse(VRMousePtr m);
        VRMousePtr getMouse();

        void setKeyboard(VRKeyboardPtr m);
        VRKeyboardPtr getKeyboard();

        WindowRecPtr getOSGWindow();
        void addView(VRViewPtr view);
        void remView(VRViewPtr view);
        vector<VRViewPtr> getViews();

        virtual void render();
        virtual void save(xmlpp::Element* node);
        virtual void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;


#endif // VRWINDOW_H_INCLUDED
