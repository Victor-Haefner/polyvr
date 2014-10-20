#ifndef VRGUINET_H_INCLUDED
#define VRGUINET_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/VRSceneManager.h"
#include "VRGuiSignals.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiNet {
    private:

    public:
        VRGuiNet();

        void updateList();
};

OSG_END_NAMESPACE

#endif // VRGUINET_H_INCLUDED
