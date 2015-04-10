#ifndef VRPATHTOOL_H_INCLUDED
#define VRPATHTOOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <vector>
#include <map>



using namespace std;
OSG_BEGIN_NAMESPACE

class VRGeometry;
class VRDevice;
class VRObject;
class path;

class VRPathtool {
    private:
        struct entry {
            path* p = 0;
            map<VRGeometry*, int> handles;
            VRGeometry* line = 0;
            VRObject* anchor = 0;
        };

        map<int, entry*> paths;
        map<VRGeometry*, entry*> handles_dict;

        void updatePath(entry* e);
        void update();

    public:
        VRPathtool();

        int newPath(VRDevice* dev, VRObject* anchor);
        void extrude(VRDevice* dev, int i);
        void remPath(int i);
};

OSG_END_NAMESPACE

#endif // VRPATHTOOL_H_INCLUDED
