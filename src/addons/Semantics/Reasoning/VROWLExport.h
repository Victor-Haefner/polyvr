#ifndef VROWLEXPORT_H_INCLUDED
#define VROWLEXPORT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRSemanticsFwd.h"

#include <map>
#include <vector>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROWLExport {
    private:
    public:
        VROWLExport();

        void write(VROntologyPtr o, string path);
};

OSG_END_NAMESPACE;

#endif // VROWLEXPORT_H_INCLUDED
