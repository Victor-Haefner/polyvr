#ifndef VRGUIMANAGER_H_INCLUDED
#define VRGUIMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiManager {
    private:
        bool standalone = false;

        VRGuiManager();

        void update();

    public:
        static VRGuiManager* get();
        ~VRGuiManager();

        void updateGtk();
};

OSG_END_NAMESPACE;

#endif // VRGUIMANAGER_H_INCLUDED
