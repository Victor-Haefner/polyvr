#ifndef VRFACTORY_H_INCLUDED
#define VRFACTORY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFactory {
    public:
        struct node {
            int ID;

            node* parent;
            map<int, node*> children;

            VRLodPtr lod;
        };

    public:
        VRFactory();
        static shared_ptr<VRFactory> create();

        bool loadVRML(string path, VRProgressPtr p, VRTransformPtr t, bool thread = 0);
        VRObjectPtr setupLod(vector<string> paths);
};

OSG_END_NAMESPACE;

#endif // VRFACTORY_H_INCLUDED
