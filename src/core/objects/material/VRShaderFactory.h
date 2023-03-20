#ifndef VRSHADERFACTORY_H_INCLUDED
#define VRSHADERFACTORY_H_INCLUDED


#include "VRMaterialFwd.h"
#include <OpenSG/OSGConfig.h>
#include <map>

using namespace std;

OSG_BEGIN_NAMESPACE;

class VRShaderFactory {
    private:
        map<string, string> chunks;

    public:
        VRShaderFactory();
        ~VRShaderFactory();

        int computeGLSLVersion(string& code);
};

OSG_END_NAMESPACE;

#endif // VRSHADERFACTORY_H_INCLUDED
