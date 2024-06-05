#ifndef VRIMGUIPROFILER_H_INCLUDED
#define VRIMGUIPROFILER_H_INCLUDED

#include "VRImguiUtils.h"
#include "core/gui/VRGuiSignals.h"

using namespace std;

class ImProfiler : public ImWidget {
    private:
        string currentTab = "System";
        string selected = "System";

        string vendor;
        string version;
        string glsl;
        string hasGeomShader;
        string hasTessShader;

        string Nnodes = "0";
        string Ntransformations = "0";
        string Ngeometries = "0";

        void updateSystem(OSG::VRGuiSignals::Options& o);
        void updateScene(OSG::VRGuiSignals::Options& o);
        void updatePerformance(OSG::VRGuiSignals::Options& o);

        void renderTabSystem();
        void renderTabScene();
        void renderTabPerformance();

    public:
        ImProfiler();
        void begin() override;
};

#endif // VRIMGUIPROFILER_H_INCLUDED
