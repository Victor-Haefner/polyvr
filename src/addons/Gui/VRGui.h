#ifndef VRGUI_H_INCLUDED
#define VRGUI_H_INCLUDED

#include <vector>
#include <OpenSG/OSGConfig.h>

#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRMaterialFwd.h"
#include "core/gui/VRGuiFwd.h"

struct VRGuiInternals;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRDevice;

class VRGui {
    private:
        VRGuiInternals* internals = 0;
        int width = 800;
        int height = 800;

        string site;
        VRMaterialWeakPtr mat;
        VRObjectWeakPtr obj;
        int resolution = 1024;
        float aspect = 1;
        bool init = false;
        bool focus = false;
        bool ctrlUsed = false;
        bool doMouse = true;
        bool doKeyboard = true;
        int mX = -1;
        int mY = -1;

        VRUpdateCbPtr update_callback;
        map<VRDevice*, VRDeviceCbPtr> mouse_dev_callback;
        map<VRDevice*, VRUpdateCbPtr> mouse_move_callback;
        //VRDeviceCbPtr mouse_dev_callback;
        VRDeviceCbPtr keyboard_dev_callback;

        void global_initiate();
        void initiate();
        void update();

        bool mouse(int lb, int rb, int wu, int wd, VRDeviceWeakPtr dev);
        void mouse_move(VRDeviceWeakPtr dev);
        bool keyboard(VRDeviceWeakPtr dev);

        VRGui();
    public:
        ~VRGui();
        static shared_ptr<VRGui> create();

        void setResolution(float a);
        void setAspectRatio(float a);

        void setMaterial(VRMaterialPtr mat);
        void addMouse(VRDevicePtr dev, VRObjectPtr obj, int lb, int rb, int wu, int wd);
        void addKeyboard(VRDevicePtr dev);
        void toggleInput(bool keyboard, bool mouse);

        void open(string site);
        string getSite();
        void resize();

        static vector< shared_ptr<VRGui> > getInstances();
        static void reloadScripts(string path);
};

typedef shared_ptr<VRGui> VRGuiPtr;

OSG_END_NAMESPACE;

#endif // VRGUI_H_INCLUDED
