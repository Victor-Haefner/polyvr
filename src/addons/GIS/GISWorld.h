#ifndef GISWORLD_H_INCLUDED
#define GISWORLD_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

class GISWorld {
    private:
        //GraphPtr streets;
        //VROntologyPtr world;

    public:
        GISWorld();

        static void setupOntology();
};

OSG_END_NAMESPACE;

#endif // GISWORLD_H_INCLUDED
