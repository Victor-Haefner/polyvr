#ifndef VRProcess_H_INCLUDED
#define VRProcess_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/utils/VRName.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"
#include "core/math/graphT.h"

#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProcess : public std::enable_shared_from_this<VRProcess>, public VRName {
    public:
        struct Subject : VRName {
            Subject(VREntityPtr e);
            void update(graph_base::node& n);
        };

        struct Fragment : VRName {
            void update(graph_base::node& n);
        };

        typedef graph<Subject> InteractionDiagram;
        typedef graph<Fragment> BehaviorDiagram;
        typedef shared_ptr< InteractionDiagram > InteractionDiagramPtr;
        typedef shared_ptr< BehaviorDiagram > BehaviorDiagramPtr;

    private:
        VROntologyPtr ontology;
        InteractionDiagramPtr interactionDiagram;
        map<string, BehaviorDiagramPtr> behaviorDiagrams;

        void update();

    public:
        VRProcess(string name);
        static VRProcessPtr create(string name);
        VRProcessPtr ptr();

        void open(string path);
        void setOntology(VROntologyPtr o);
        graph_basePtr getInteractionDiagram();
        graph_basePtr getBehaviorDiagram(string subject);
};

OSG_END_NAMESPACE;

#endif // VRProcess_H_INCLUDED
