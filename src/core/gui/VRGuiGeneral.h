#ifndef VRGUIGENERAL_H_INCLUDED
#define VRGUIGENERAL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "VRGuiSignals.h"

struct _GdkEventButton;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiGeneral {
    private:
        bool updating = false;

        // background signals
        bool setColor(_GdkEventButton* b);
        void setExtension();
        void setPath();
        void setMode();
        void openBGpath();
        void on_bg_path_choose();

        // rendering signals
        void toggleFrustumCulling();
        void toggleOcclusionCulling();
        void toggleTwoSided();
        void toggleDeferredShader();
        void toggleDRendChannel();
        void toggleSSAO();
        void toggleCalib();
        void toggleHMDD();
        void toggleMarker();
        void toggleFXAA();

        bool setSSAOradius( int st, double d );
        bool setSSAOkernel( int st, double d );
        bool setSSAOnoise( int st, double d );

        // other
        void on_tfps_changed();
        void dumpOSG();

    public:
        VRGuiGeneral();


        void updateScene();
};

OSG_END_NAMESPACE

#endif // VRGUIGENERAL_H_INCLUDED
