#ifndef VRINTERSECTACTION_H_INCLUDED
#define VRINTERSECTACTION_H_INCLUDED

#include <OpenSG/OSGIntersectAction.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRIntersectAction : public IntersectAction {
    private:
        bool doSkipVolumes = false;

    public:
        VRIntersectAction() {}

        void setSkipVolumes(bool b) { doSkipVolumes = b; }
        bool skipVolume() { return doSkipVolumes; }
};

OSG_END_NAMESPACE;

#endif // VRINTERSECTACTION_H_INCLUDED
