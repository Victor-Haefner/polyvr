#ifndef VRVIEWMANAGER_H_INCLUDED
#define VRVIEWMANAGER_H_INCLUDED

#include <OpenSG/OSGSField.h>
#include "core/objects/VRObjectFwd.h"
#include "core/setup/VRSetupFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Background; OSG_GEN_CONTAINERPTR(Background);

class VRViewManager {
    protected:
        map<int, VRViewPtr> views;
        VRTransformPtr anchor;

        bool checkView(int i);
        void setViewAnchor(VRTransformPtr a);

    public:
        VRViewManager();
        ~VRViewManager();

        int addView(string name);
        void removeView(int i);

        void setViewCamera(VRCameraPtr c, int i);
        void setViewRoot(VRObjectPtr root, int i);
        void setViewUser(VRTransformPtr user, int i);
        void setViewBackground(BackgroundMTRecPtr bg, int i = -1);

        void updateViews();
        void toggleViewStats(int i);
        void showViewStats(int i, bool b);
        void showViewportGeos(bool b);

        VRTransformPtr getViewUser(int i);
        VRViewPtr getView(int i);
        VRViewPtr getView(string name);
        vector<VRViewPtr> getViews();

        void setStereo(bool b);
        void setStereoEyeSeparation(float v);

        void resetViewports();

        void setFotoMode(bool b);
};

OSG_END_NAMESPACE;

#endif // VRVIEWMANAGER_H_INCLUDED
