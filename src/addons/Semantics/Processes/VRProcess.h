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

enum PROCESS_WIDGET {
    SUBJECT,
    MESSAGE
};

struct VRProcessNode : VRName {
    VREntityPtr entity;
    VRTransformPtr widget;
    PROCESS_WIDGET type;
    int ID = 0;

    string label;

    VRProcessNode();
    void update(graph_base::node& n, bool changed);

    int getID();
    string getLabel();
};

class VRProcess : public std::enable_shared_from_this<VRProcess>, public VRName {
    public:
        typedef graph<VRProcessNode> Diagram;
        typedef shared_ptr< Diagram > DiagramPtr;

    private:
        VROntologyPtr ontology;
        DiagramPtr interactionDiagram;
        map<string, DiagramPtr> behaviorDiagrams;

        void update();

    public:
        VRProcess(string name);
        static VRProcessPtr create(string name);
        VRProcessPtr ptr();

        void open(string path);
        void setOntology(VROntologyPtr o);
        DiagramPtr getInteractionDiagram();
        DiagramPtr getBehaviorDiagram(string subject);
        vector<VRProcessNode> getSubjects();
};

OSG_END_NAMESPACE;

#endif // VRProcess_H_INCLUDED
