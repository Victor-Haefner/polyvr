#ifndef VRFACTORY_H_INCLUDED
#define VRFACTORY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLod;
class VRObject;

class VRFactory {
    public:
        struct node {
            int ID;

            node* parent;
            map<int, node*> children;

            VRLod* lod;
        };

    public:
        VRFactory();

        VRObject* loadVRML(string path);
        VRObject* setupLod(string path, string path_low);
};

OSG_END_NAMESPACE;

#endif // VRFACTORY_H_INCLUDED
