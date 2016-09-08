#include "VRProcess.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRTransform.h"
#include <iostream>

using namespace OSG;

VRProcess::Node::Node(VREntityPtr e) {
    ;
}

void VRProcess::Node::update(graph_base::node& n, bool changed) { // callede when graph node changes
    if (widget && !widget->isDragged() && changed) widget->setFrom(n.pos);

    if (widget && widget->isDragged()) {
        auto m = widget->getMatrixTo( widget->getDragParent() );
        n.pos = Vec3f(m[3]);
    }
}

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
VRProcess::DiagramPtr VRProcess::getBehaviorDiagram(string subject) { return behaviorDiagrams[subject]; }

void VRProcess::update() {
    if (!ontology) return;

    VRReasonerPtr reasoner = VRReasoner::create();
    reasoner->setVerbose(true, true);
    auto query = [&](string q) { return reasoner->process(q, ontology); };

    auto layers = query("q(x):Layer(x)");
    if (layers.size() == 0) return;
    auto layer = layers[0]; // only use first layer
    interactionDiagram = DiagramPtr( new Diagram() );

    map<string, int> nodes;
    auto addDiagNode = [&](VREntityPtr e, string label) {
        auto n = Node(e);
        n.label = label;
        int i = interactionDiagram->addNode(n);
        if (auto ID = e->getValue("hasModelComponentID") ) {
            nodes[ID->value] = i;
            cout << "addDiagNode " << ID->value << " " << i << endl;
        }
        return i;
    };

    auto connect = [&](int i, string parent) {
        if (nodes.count(parent)) {
            cout << "connect nodes " << i << " parent: " << parent << " parent ID " << nodes[parent] << endl;
            interactionDiagram->connect(nodes[parent], i, graph_base::HIERARCHY);
        }
    };

    string q_subjects = "q(x):ActiveProcessComponent(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(q_subjects) ) {
        auto n = Node(subject);
        string label;
        if (auto l = subject->getValue("hasModelComponentLable") ) label = l->value;
        addDiagNode(subject, label);
    }

    string q_messages = "q(x):StandardMessageExchange(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto message : query(q_messages) ) {
        string q_message = "q(x):MessageSpec(x);StandardMessageExchange("+message->getName()+");is(x,"+message->getName()+".hasMessageType)";
        auto msgs = query(q_message);
        string label = "Msg: ";
        if (msgs.size())
            if (auto l = msgs[0]->getValue("hasModelComponentLable") ) label += l->value;
        int i = addDiagNode(message, label);

        string sender;
        string receiver;
        if (auto s = message->getValue("sender") ) sender = s->value;
        if (auto r = message->getValue("receiver") ) receiver = r->value;
        connect(i, sender);
        connect(i, receiver);
    }

    //interactionDiagram->connect( n );
}




