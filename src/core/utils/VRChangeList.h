#ifndef VRCHANGELIST_H_INCLUDED
#define VRCHANGELIST_H_INCLUDED

#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRChangeList {
    private:
        size_t totalEntites = 0;

    public:
        VRChangeList();
        ~VRChangeList();

        int getDestroyed();
        int getCreated();
        int getChanged();

        size_t getTotalEntities();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRCHANGELIST_H_INCLUDED
