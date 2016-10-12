#include "VRProcess.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRTransform.h"
#include <iostream>

using namespace OSG;

VRProcessNode::VRProcessNode() {;}

void VRProcessNode::update(graph_base::node& n, bool changed) { // callede when graph node changes
    if (widget && !widget->isDragged() && changed) widget->setFrom(n.box.center());

    if (widget && widget->isDragged()) {
        auto m = widget->getMatrixTo( widget->getDragParent() );
        n.box.setCenter( Vec3f(m[3]) );
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
    ontology = VROntology::create(getBaseName());
    ontology->open(path);
    update();
}

void VRProcess::setOntology(VROntologyPtr o) { ontology = o; update(); }

VRProcess::DiagramPtr VRProcess::getInteractionDiagram() { return interactionDiagram; }
VRProcess::DiagramPtr VRProcess::getBehaviorDiagram(int subject) { return behaviorDiagrams.count(subject) ? behaviorDiagrams[subject] : 0; }

vector<VRProcessNode> VRProcess::getSubjects() {
    vector<VRProcessNode> res;
    for (int i=0; i<interactionDiagram->size(); i++) {
        auto& e = interactionDiagram->getElement(i);
        if (e.type == SUBJECT) res.push_back(e);
    }
    return res;
}

void VRProcess::update() {
    if (!ontology) return;

    VRReasonerPtr reasoner = VRReasoner::create();
    reasoner->setVerbose(true,  false); //
    auto query = [&](string q) { return reasoner->process(q, ontology); };

    map<string, int> nodes;
    auto addDiagNode = [&](DiagramPtr diag, string label, PROCESS_WIDGET type) {
        auto n = VRProcessNode();
        n.label = label;
        n.type = type;
        int i = diag->addNode(n);
        diag->getElement(i).ID = i;
        return i;
    };

    auto connect = [&](DiagramPtr diag, int i, string parent, graph_base::CONNECTION mode) {
        if (nodes.count(parent)) {
            switch (mode) {
                case graph_base::HIERARCHY: diag->connect(nodes[parent], i, mode); break;
                case graph_base::DEPENDENCY: diag->connect(i, nodes[parent], mode); break;
            }
        }
    };

    /** get interaction diagram **/

    auto layers = query("q(x):Layer(x)");
    if (layers.size() == 0) return;
    auto layer = layers[0]; // only use first layer
    interactionDiagram = DiagramPtr( new Diagram() );

    string q_subjects = "q(x):ActiveProcessComponent(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(q_subjects) ) {
        string label;
        if (auto l = subject->getValue("hasModelComponentLable") ) label = l->value;
        int nID = addDiagNode(interactionDiagram, label, SUBJECT);
        if (auto ID = subject->getValue("hasModelComponentID") ) nodes[ID->value] = nID;
    }

    map<string, map<string, vector<VREntityPtr>>> messages;
    string q_messages = "q(x):MessageExchange(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto message : query(q_messages) ) {
        string sender;
        string receiver;
        if (auto s = message->getValue("sender") ) sender = s->value;
        if (auto r = message->getValue("receiver") ) receiver = r->value;
        messages[sender][receiver].push_back(message);
    }

    for ( auto sender : messages ) {
        for (auto receiver : sender.second) {
            string label = "Msg:";
            for (auto message : receiver.second) {
                string q_message = "q(x):MessageSpec(x);MessageExchange("+message->getName()+");is(x,"+message->getName()+".hasMessageType)";
                auto msgs = query(q_message);
                if (msgs.size())
                    if (auto l = msgs[0]->getValue("hasModelComponentLable") ) label += "\n - " + l->value;
            }

            int nID = addDiagNode(interactionDiagram, label, MESSAGE);
            connect(interactionDiagram, nID, sender.first, graph_base::HIERARCHY);
            connect(interactionDiagram, nID, receiver.first, graph_base::DEPENDENCY);
        }
    }

    /** get behavior diagrams **/

    for (auto behavior : query("q(x):Behavior(x)")) {
        auto behaviorDiagram = DiagramPtr( new Diagram() );
        string q_Subject = "q(x):ActiveProcessComponent(x);Behavior("+behavior->getName()+");has(x,"+behavior->getName()+")";

/** TODO:
    important reasoning fix:
        ActiveProcessComponent does not have property of type behavior, but Actor and AbstractActor do!
        When checking for an ActiveProcessComponent that has a behavior, the instances found for ActiveProcessComponent will have a subtype like Actor!
        This has to be taken into account when checking the has relation! (instead of only using the declared concept of ActiveProcessComponent)
**/

        //string q_Subject = "q(x):ActiveProcessComponent(x)";
        auto subjects = query(q_Subject);
        for (auto s : subjects) cout << s->toString() << endl;

        if (subjects.size() == 0) continue;
        auto subject = subjects[0];
        auto ID = subject->getValue("hasModelComponentID");
        int sID = nodes[ID->value];
        behaviorDiagrams[sID] = behaviorDiagram;
    }
}




