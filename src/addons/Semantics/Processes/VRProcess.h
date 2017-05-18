#ifndef VRProcess_H_INCLUDED
#define VRProcess_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/utils/VRName.h"
#include "core/math/VRMathFwd.h"
#include "core/math/graph.h"

#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

enum PROCESS_WIDGET {
    SUBJECT,
    MESSAGE,
    ACTION
};

struct VRProcessNode : VRName {
    VREntityPtr entity;
    VRTransformPtr widget;
    PROCESS_WIDGET type;
    string label;
    int ID = 0;

    VRProcessNode(string name, PROCESS_WIDGET type, int ID);
    ~VRProcessNode();
    static VRProcessNodePtr create(string name, PROCESS_WIDGET type, int ID);

    void update(Graph::node& n, bool changed);

    int getID();
    string getLabel();
};

class VRProcess : public std::enable_shared_from_this<VRProcess>, public VRName {
    public:
        struct Diagram : public Graph {
            map<int, VRProcessNodePtr> processnodes;

            void update(int i, bool changed);
            void remNode(int i);
            void clear();
        };

        typedef shared_ptr< Diagram > DiagramPtr;

    private:
        VROntologyPtr ontology;
        DiagramPtr interactionDiagram;
        map<int, DiagramPtr> behaviorDiagrams;

        void update();

    public:
        VRProcess(string name);
        static VRProcessPtr create(string name);
        VRProcessPtr ptr();

        void open(string path);
        void setOntology(VROntologyPtr o);
        DiagramPtr getInteractionDiagram();
        DiagramPtr getBehaviorDiagram(int subject);
        vector<VRProcessNodePtr> getSubjects();
        VRProcessNodePtr getNode(int i, DiagramPtr diag = 0);

        VRProcessNodePtr addSubject(string name);
        VRProcessNodePtr addMessage(string name, int i, int j, DiagramPtr diag = 0);
        VRProcessNodePtr addAction(string name, DiagramPtr diag);
};

OSG_END_NAMESPACE;

#endif // VRProcess_H_INCLUDED
