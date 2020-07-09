#ifndef VRWINDOW_H_INCLUDED
#define VRWINDOW_H_INCLUDED

#include <OpenSG/OSGWindow.h>
#include <OpenSG/OSGRenderAction.h>

#include "core/setup/VRSetupFwd.h"
#include "core/utils/VRDeviceFwd.h"
#include "core/utils/VRName.h"
#include "core/utils/VRChangeList.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRWindow;
class VRThread;
typedef boost::function<void (VRWindowPtr, int, int, int, int, int)> VRWindowCallback; // params: device, button, state, mouse x, mouse y

class VRWindow : public std::enable_shared_from_this<VRWindow>, public VRName {
    protected:
        bool active = false;
        bool content = false;
        bool waitingAtBarrier = false;
        bool stopping = false;
        string msaa = "x4";
        int type = -1;
        WindowMTRecPtr _win;
        RenderActionRefPtr ract;
        vector<VRViewWeakPtr> views;
        VRChangeList changeListStats;

        VRMousePtr mouse = 0;
        VRMultiTouchPtr multitouch = 0;
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

        bool isWaiting();
        void stop();

        void setMouse(VRMousePtr m);
        void setMultitouch(VRMultiTouchPtr m);
        VRMousePtr getMouse();
        VRMultiTouchPtr getMultitouch();

        void setKeyboard(VRKeyboardPtr m);
        VRKeyboardPtr getKeyboard();

        WindowMTRecPtr getOSGWindow();
        void addView(VRViewPtr view);
        void remView(VRViewPtr view);
        vector<VRViewPtr> getViews();

        void setMSAA(string s);
        string getMSAA();

        virtual void sync(bool fromThread = false);
        virtual void render(bool fromThread = false);
        virtual void clear(Color3f c);
        virtual void save(XMLElementPtr node);
        virtual void load(XMLElementPtr node);
};

OSG_END_NAMESPACE;


#endif // VRWINDOW_H_INCLUDED
