#ifndef GISWORLD_H_INCLUDED
#define GISWORLD_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"

OSG_BEGIN_NAMESPACE;

class GISWorld {
    private:
        //graph_basePtr streets;
        VROntologyPtr world;

        void setupOntology();

    public:
        GISWorld();
};

OSG_END_NAMESPACE;

#endif // GISWORLD_H_INCLUDED
