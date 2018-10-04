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
    STATE, //Do State, Send State, Receive State
    TRANSITION
};

struct VRProcessNode : VRName {
    VREntityPtr entity;
    VRTransformPtr widget;
    PROCESS_WIDGET type;
    string label;
    int ID = 0;
    int subject = 0;
    bool isInitialState = 0;

    VRProcessNode(string name, PROCESS_WIDGET type, int ID, int sID);
    ~VRProcessNode();
    static VRProcessNodePtr create(string name, PROCESS_WIDGET type, int ID, int sID);

    void update(Graph::node& n, bool changed);

    int getID();
    string getLabel();
};

struct VRProcessDiagram : public Graph {
    map<int, VRProcessNodePtr> processnodes;

    VRProcessDiagram();
    ~VRProcessDiagram();
    static VRProcessDiagramPtr create();

    void update(int i, bool changed);
    void remNode(int i);
    void clear();
};

class VRProcess : public std::enable_shared_from_this<VRProcess>, public VRName {
    private:
        VROntologyPtr ontology;
        VRProcessDiagramPtr interactionDiagram;
        map<int, VRProcessDiagramPtr> behaviorDiagrams;
        void printNodes(VRProcessDiagramPtr d);

        void update();

    public:
        VRProcess(string name);
        static VRProcessPtr create(string name);
        VRProcessPtr ptr();

        void open(string path);
        void setOntology(VROntologyPtr o);
        VROntologyPtr getOntology();
        VRProcessDiagramPtr getInteractionDiagram();
        VRProcessDiagramPtr getBehaviorDiagram(int subject);
        VRProcessNodePtr getNode(int i, VRProcessDiagramPtr diag = 0);

        vector<VRProcessNodePtr> getSubjectMessages(int subjectID);
        vector<VRProcessNodePtr> getMessageSubjects(int messageID);
        vector<VRProcessNodePtr> getSubjects();
        vector<VRProcessNodePtr> getMessages();
        vector<VRProcessNodePtr> getSubjectStates(int subjectID);
        vector<VRProcessNodePtr> getStateTransitions(int subjectID, int stateID);
        vector<VRProcessNodePtr> getTransitionStates(int subjectID, int transitionID);
        vector<VRProcessNodePtr> getTransitions(int subjectID);
        map<VRProcessNodePtr, VRProcessNodePtr> getInitialStates(); // <subjectNode, initialStateNode>

        VRProcessNodePtr addSubject(string name);
        VRProcessNodePtr addMessage(string name, int i, int j, VRProcessDiagramPtr diag = 0);
        VRProcessNodePtr addState(string name, int sID);
        void setInitialState(VRProcessNodePtr state, int sID);
        VRProcessNodePtr addTransition(string name, int sID, int i, int j, VRProcessDiagramPtr d = 0);

        void remNode(VRProcessNodePtr n);
        VRProcessNodePtr getTransitionState(VRProcessNodePtr transition);
};

OSG_END_NAMESPACE;

#endif // VRProcess_H_INCLUDED
