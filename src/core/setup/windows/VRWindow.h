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
typedef boost::function<void (VRWindow*, int, int, int, int, int)> VRWindowCallback; // params: device, button, state, mouse x, mouse y

class VRWindow : public VRName {
    protected:
        bool active = false;
        bool content = false;
        int type = -1;
        WindowRecPtr _win;
        RenderActionRefPtr ract;
        vector<VRView*> views;

        VRMouse* mouse = 0;
        VRKeyboard* keyboard = 0;
        int width = 640;
        int height = 480;

        int thread_id = -1;
        void update(VRThread* t);

    public:
        VRWindow();
        virtual ~VRWindow();
        bool hasType(int i);
        void resize(int w, int h);

        void setAction(RenderActionRefPtr ract);

        static unsigned int active_window_count;

        bool isActive();
        void setActive(bool b);

        bool hasContent();
        void setContent(bool b);

        void setMouse(VRMouse* m);
        VRMouse* getMouse();

        void setKeyboard(VRKeyboard* m);
        VRKeyboard* getKeyboard();

        WindowRecPtr getOSGWindow();
        void addView(VRView* view);
        void remView(VRView* view);
        vector<VRView*> getViews();

        virtual void render();
        virtual void save(xmlpp::Element* node);
        virtual void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;


#endif // VRWINDOW_H_INCLUDED
