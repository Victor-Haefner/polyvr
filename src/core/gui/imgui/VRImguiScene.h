#ifndef VRIMGUISCENE_H_INCLUDED
#define VRIMGUISCENE_H_INCLUDED

#include "VRImguiUtils.h"
#include "VRImguiSceneRendering.h"
#include "VRImguiSceneScenegraph.h"
#include "VRImguiSceneScripting.h"
#include "VRImguiSceneNavigation.h"
#include "VRImguiSceneSemantics.h"
#include "VRImguiSceneNetwork.h"

using namespace std;

class ImSceneEditor : public ImWidget {
    private:
        string currentTab = "Scripting";

    public:
        ImRendering rendering;
        ImScenegraph scenegraph;
        ImScripting scripting;
        ImNavigation navigation;
        ImSemantics semantics;
        ImNetwork network;

        ImSceneEditor();
        void begin() override;
};

#endif // VRIMGUISCENE_H_INCLUDED
