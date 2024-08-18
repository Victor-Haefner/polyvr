#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include <vector>
#include <OpenSG/OSGConfig.h>

#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRMaterialFwd.h"

struct CEFInternals;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRDevice;

class CEF {
    private:
        CEFInternals* internals = 0;

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

        map<int, bool> blockedSignals;

        VRUpdateCbPtr update_callback;
        map<VRDevice*, VRDeviceCbPtr> mouse_dev_callback;
        map<VRDevice*, VRUpdateCbPtr> mouse_move_callback;
        //VRDeviceCbPtr mouse_dev_callback;
        VRDeviceCbPtr keyboard_dev_callback;

        void global_initiate();
        void initiate();
        void update();

        bool mouse(int lb, int mb, int rb, int wu, int wd, VRDeviceWeakPtr dev);
        void mouse_move(VRDeviceWeakPtr dev);
        bool keyboard(VRDeviceWeakPtr dev);

        CEF();
    public:
        ~CEF();
        static shared_ptr<CEF> create();

        void setResolution(float a);
        void setAspectRatio(float a);


        void setMaterial(VRMaterialPtr mat);
        void addMouse(VRDevicePtr dev, VRObjectPtr obj, int lb, int mb, int rb, int wu, int wd);
        void addKeyboard(VRDevicePtr dev);
        void setBlockedSignal(int s, bool b);
        void toggleInput(bool keyboard, bool mouse);

        void open(string site);
        void reload();
        string getSite();
        void resize();

        static vector< shared_ptr<CEF> > getInstances();
        static void reloadScripts(string path);
        static void shutdown();
};

typedef shared_ptr<CEF> CEFPtr;

OSG_END_NAMESPACE;

#endif // CEF_H_INCLUDED
