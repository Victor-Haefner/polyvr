#ifndef VRCHANGELIST_H_INCLUDED
#define VRCHANGELIST_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRChangeList {
    private:
        string name;
        size_t totalEntites = 0;

    public:
        VRChangeList(string name);
        ~VRChangeList();

        int getDestroyed();
        int getCreated();
        int getChanged();

        size_t getTotalEntities();

        void update();
        void stopOutput();
};

OSG_END_NAMESPACE;

#endif // VRCHANGELIST_H_INCLUDED
