#ifndef VROWLIMPORT_H_INCLUDED
#define VROWLIMPORT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRSemanticsFwd.h"

#include <map>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROWLImport {
    private:
    public:
        VROWLImport();

        void load(VROntologyPtr o, string path);
};

OSG_END_NAMESPACE;

#endif // VROWLIMPORT_H_INCLUDED
