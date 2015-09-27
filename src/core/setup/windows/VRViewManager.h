#ifndef VRVIEWMANAGER_H_INCLUDED
#define VRVIEWMANAGER_H_INCLUDED

#include <OpenSG/OSGFieldContainerFields.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRView;

class Background; OSG_GEN_CONTAINERPTR(Background);

class VRViewManager {
    protected:
        map<int, VRView*> views;
        map<int, VRView*>::iterator itr;
        VRTransformPtr anchor;

        bool checkView(int i);

        void setViewAnchor(VRTransformPtr a);

    public:
        VRViewManager();
        ~VRViewManager();

        //int addView(bool active_stereo = false, bool stereo = false, bool projection = false, Pnt3f screenLowerLeft = Pnt3f(0,0,0), Pnt3f screenLowerRight = Pnt3f(0,0,0), Pnt3f screenUpperRight = Pnt3f(0,0,0), Pnt3f screenUpperLeft = Pnt3f(0,0,0), bool swapeyes = false);
        int addView(string name);

        void setViewCamera(VRCameraPtr c, int i);

        void setViewRoot(VRObjectPtr root, int i);

        void setViewUser(VRTransformPtr user, int i);

        void setViewBackground(BackgroundRecPtr bg, int i = -1);

        void showViewStats(int i, bool b);
        void showViewportGeos(bool b);

        void removeView(int i);

        VRTransformPtr getViewUser(int i);

        VRView* getView(int i);

        void setStereo(bool b);
        void setStereoEyeSeparation(float v);

        void resetViewports();

        void setFotoMode(bool b);
        void setCallibrationMode(bool b);
};

OSG_END_NAMESPACE;

#endif // VRVIEWMANAGER_H_INCLUDED
