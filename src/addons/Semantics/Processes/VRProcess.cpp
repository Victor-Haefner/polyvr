#include "VRProcess.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRTransform.h"
#include <iostream>

using namespace OSG;

template<> string typeName(const VRProcessPtr& o) { return "Process"; }
template<> string typeName(const VRProcessNodePtr& o) { return "ProcessNode"; }
template<> string typeName(const VRProcessDiagramPtr& o) { return "ProcessDiagram"; }


VRProcessDiagram::VRProcessDiagram() {}
VRProcessDiagram::~VRProcessDiagram() {}
VRProcessDiagramPtr VRProcessDiagram::create() { return VRProcessDiagramPtr( new VRProcessDiagram() ); }

void VRProcessDiagram::update(int i, bool changed) {
    if (processnodes.count(i)) processnodes[i]->update(nodes[i], changed);
}

void VRProcessDiagram::remNode(int i) { Graph::remNode(i); processnodes.erase(i); }
void VRProcessDiagram::clear() { Graph::clear(); processnodes.clear(); }

VRProcessNode::VRProcessNode(string name, PROCESS_WIDGET type, int ID) : type(type), label(name), ID(ID) {;}
VRProcessNode::~VRProcessNode() {}
VRProcessNodePtr VRProcessNode::create(string name, PROCESS_WIDGET type, int ID) { return VRProcessNodePtr( new VRProcessNode(name, type, ID) ); }

void VRProcessNode::update(Graph::node& n, bool changed) { // callede when graph node changes
    if (widget && !widget->isDragged() && changed) widget->setFrom(n.box.center());

    if (widget && widget->isDragged()) {
        auto m = widget->getDragParent()->getMatrixTo( widget );
        n.box.setCenter( Vec3d(m[3]) );
    }
}

int VRProcessNode::getID() { return ID; }
string VRProcessNode::getLabel() { return label; }

VRProcess::VRProcess(string name) {
    setStorageType("Process");
    setPersistency(0);
    setNameSpace("Process");
    setName(name);

    storeObj("Ontology", ontology);
}

VRProcessPtr VRProcess::create(string name) { return VRProcessPtr( new VRProcess(name) ); }
VRProcessPtr VRProcess::ptr() { return shared_from_this(); }

void VRProcess::open(string path) {
    if (!ontology) ontology = VROntology::create(getBaseName());
    ontology->openOWL(path);
    update();
}

void VRProcess::setOntology(VROntologyPtr o) { ontology = o; update(); }

VRProcessDiagramPtr VRProcess::getInteractionDiagram() {
    if (!interactionDiagram) interactionDiagram = VRProcessDiagram::create();
    return interactionDiagram;
}

VRProcessDiagramPtr VRProcess::getBehaviorDiagram(int subject) { return behaviorDiagrams.count(subject) ? behaviorDiagrams[subject] : 0; }

vector<VRProcessNodePtr> VRProcess::getSubjects() {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    for (auto node : interactionDiagram->processnodes) {
        if (node.second->type == SUBJECT) res.push_back(node.second);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getMessages() {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    for (auto node : interactionDiagram->processnodes) {
        if (node.second->type == MESSAGE) res.push_back(node.second);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getSubjectMessages(int subjectID) {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    auto d = interactionDiagram;
    auto neighbors = d->getNeighbors( subjectID );
    for (auto node : neighbors) {
        auto subject = getNode( node.ID );
        res.push_back(subject);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getMessageSubjects(int messageID) {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    auto d = interactionDiagram;
    auto neighbors = d->getNeighbors( messageID );
    for (auto node : neighbors) {
        auto message = getNode( node.ID );
        res.push_back(message);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getSubjectActions(int subjectID) {
    vector<VRProcessNodePtr> res;
    if (!behaviorDiagrams.count(subjectID)) return res;
    auto d = behaviorDiagrams[subjectID];
    for (auto node : d->processnodes) {
        if (node.second->type == ACTION) res.push_back(node.second);
    }
    return res;
}
//TODO: fixing; doesn't return action transitions but empty list
vector<VRProcessNodePtr> VRProcess::getActionTransitions(int subjectID, int actionID) {
    auto d = getBehaviorDiagram(subjectID);
    auto neighbors = d->getNeighbors( actionID );
    vector<VRProcessNodePtr> res;
    for (auto node : neighbors) {
        auto transition = getNode( node.ID );
        res.push_back(transition);
    }
    return res;
}

void VRProcess::update() {
    if (!ontology) return;

    VRReasonerPtr reasoner = VRReasoner::create();
    reasoner->setVerbose(true,  false); //
    auto query = [&](string q) { return reasoner->process(q, ontology); };

    /** get interaction diagram **/
    auto layers = ontology->getEntities("ModelLayer");
    if (layers.size() == 0) return;
    auto layer = layers[0]; // only use first layer
    interactionDiagram = VRProcessDiagram::create();

    map<string, int> nodes;
    string q_subjects = "q(x):ActiveProcessComponent(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(q_subjects) ) {
        string label;
        if (auto l = subject->get("hasModelComponentLable") ) label = l->value;
        int nID = addSubject(label)->ID;
        if (auto ID = subject->get("hasModelComponentID") ) nodes[ID->value] = nID;
        //cout << " VRProcess::update subject: " << label << endl;
    }

    map<string, map<string, vector<VREntityPtr>>> messages;
    string q_messages = "q(x):MessageExchange(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto message : query(q_messages) ) {
        string sender;
        string receiver;
        if (auto s = message->get("sender") ) sender = s->value;
        if (auto r = message->get("receiver") ) receiver = r->value;
        messages[sender][receiver].push_back(message);
    }

    for ( auto sender : messages ) {
        for (auto receiver : sender.second) {
            string label = "Msg:";
            for (auto message : receiver.second) {
                string q_message = "q(x):MessageSpec(x);MessageExchange("+message->getName()+");is(x,"+message->getName()+".hasMessageType)";
                auto msgs = query(q_message);
                if (msgs.size())
                    if (auto l = msgs[0]->get("hasModelComponentLable") ) label += "\n - " + l->value;
            }

            addMessage(label, nodes[sender.first], nodes[receiver.first]);
        }
    }

    //return;
    /** get behavior diagrams **/

    for (auto behavior : query("q(x):Behavior(x)")) {
        auto behaviorDiagram = VRProcessDiagram::create();
        string q_Subject = "q(x):ActiveProcessComponent(x);Behavior("+behavior->getName()+");has(x,"+behavior->getName()+")";
        auto subjects = query(q_Subject);
        if (subjects.size() == 0) continue;
        auto subject = subjects[0];
        auto ID = subject->get("hasModelComponentID");
        int sID = nodes[ID->value];
        behaviorDiagrams[sID] = behaviorDiagram;

        string q_States = "q(x):State(x);Behavior("+behavior->getName()+");has("+behavior->getName()+",x)";
        for (auto state : query(q_States)) {
            string label;
            if (auto l = state->get("hasModelComponentLable") ) label = l->value;
            int nID = addAction(label, sID)->ID;
            if (auto ID = state->get("hasModelComponentID") ) nodes[ID->value] = nID;
        }

        map<string, map<string, vector<VREntityPtr>>> edges;
        string q_Edges = "q(x):TransitionEdge(x);Behavior("+behavior->getName()+");has("+behavior->getName()+",x)";
        for (auto edge : query(q_Edges)) {
            string source;
            string target;
            if (auto s = edge->get("hasSourceState") ) source = s->value;
            if (auto r = edge->get("hasTargetState") ) target = r->value;
            edges[source][target].push_back(edge);
        }

        for ( auto source : edges ) {
            for (auto target : source.second) {
                addMessage("Msg:", nodes[source.first], nodes[target.first], behaviorDiagram);
            }
        }
    }
}

void VRProcess::remNode(VRProcessNodePtr n) {
    if (auto diag = interactionDiagram) {
        if (diag->processnodes.count(n->ID)) {
            diag->remNode(n->ID);
            if (behaviorDiagrams.count(n->ID)) behaviorDiagrams.erase(n->ID);
        }
    }

    for (auto diag : behaviorDiagrams) {
        if (diag.second->processnodes.count(n->ID)) diag.second->remNode(n->ID);
    }
}

VRProcessNodePtr VRProcess::addSubject(string name) {
    if (!interactionDiagram) interactionDiagram = VRProcessDiagram::create();
    auto sID = interactionDiagram->addNode();
    auto s = VRProcessNode::create(name, SUBJECT, sID);
    interactionDiagram->processnodes[sID] = s;
    auto behaviorDiagram = VRProcessDiagram::create();
    behaviorDiagrams[sID] = behaviorDiagram;
    return s;
}

VRProcessNodePtr VRProcess::addMessage(string name, int i, int j, VRProcessDiagramPtr diag) {
    if (!diag) diag = interactionDiagram;
    if (!diag) return 0;
    auto mID = diag->addNode();
    auto m = VRProcessNode::create(name, MESSAGE, mID);
    diag->processnodes[mID] = m;
    diag->connect(i, mID, Graph::HIERARCHY);
    diag->connect(mID, j, Graph::DEPENDENCY);
    return m;
}

VRProcessNodePtr VRProcess::addAction(string name, int sID) {
    if (!behaviorDiagrams.count(sID)) {
        cout << "VRProcess::addAction " << sID << "  " << behaviorDiagrams.size() << endl;
        for (auto d : behaviorDiagrams) cout << " " << d.first << endl;
    }

    if (!behaviorDiagrams.count(sID)) return 0;
    auto diag = behaviorDiagrams[sID];
    if (!diag) return 0;
    auto aID = diag->addNode();
    auto a = VRProcessNode::create(name, ACTION, aID);
    diag->processnodes[aID] = a;
    return a;
}

VRProcessNodePtr VRProcess::getNode(int i, VRProcessDiagramPtr diag) {
    if (!diag) diag = interactionDiagram;
    return diag->processnodes[i];
}





