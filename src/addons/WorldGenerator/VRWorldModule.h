#ifndef VRWORLDMODULE_H_INCLUDED
#define VRWORLDMODULE_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <OpenSG/OSGVector.h>
#include "VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWorldModule {
    protected:
        VRWorldGeneratorPtr world;
        VRTerrainPtr terrain;

    public:
        VRWorldModule();
        ~VRWorldModule();

        void setWorld(VRWorldGeneratorPtr w);
};

OSG_END_NAMESPACE;

#endif // VRWORLDMODULE_H_INCLUDED
