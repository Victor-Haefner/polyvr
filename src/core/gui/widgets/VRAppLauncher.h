#ifndef VRAPPLAUNCHER_H_INCLUDED
#define VRAPPLAUNCHER_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>

#include "core/utils/VRFwdDeclTemplate.h"
#include "core/utils/VRFunctionFwd.h"

class VRGuiContextMenu;

struct _GtkButton;
struct _GtkImage;
struct _GtkLabel;
struct _GtkFrame;

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRAppLauncher);
ptrFwd(VRAppPanel);
ptrFwd(VRAppManager);

class VRAppLauncher : public std::enable_shared_from_this<VRAppLauncher> {
    public:
        string path;
        string lastStarted;
        string pxm_path;
        string table;
        _GtkFrame* widget = 0;
        _GtkButton* butPlay = 0;
        _GtkButton* butOpts = 0;
        _GtkButton* butLock = 0;
        _GtkLabel* label = 0;
        _GtkLabel* timestamp = 0;
        _GtkImage* imgScene = 0;
        _GtkImage* imgPlay = 0;
        _GtkImage* imgLock = 0;
        _GtkImage* imgUnlock = 0;
        _GtkImage* imgOpts = 0;
        bool running = false;
        bool pixmap = false;
        bool favorite = true;
        bool write_protected = false;
        VRDeviceCbPtr uPixmap;
        VRAppPanelWeakPtr section;

    public:
        VRAppLauncher(VRAppPanelPtr s);
        ~VRAppLauncher();
        static VRAppLauncherPtr create(VRAppPanelPtr s);

        void setup(VRGuiContextMenu* menu, VRAppManager* mgr);
        void updatePixmap();

        void setState(int state);
        void toggle_lock();

        void show();
        void hide();
};

OSG_END_NAMESPACE;

#endif // VRAPPLAUNCHER_H_INCLUDED
