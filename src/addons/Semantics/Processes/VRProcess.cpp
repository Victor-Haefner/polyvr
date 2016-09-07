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

void VRProcess::Node::update(graph_base::node& n) {
    if (widget) {
        if (widget->isDragged()) {
            auto m = widget->getMatrixTo( widget->getDragParent() );
            n.pos = Vec3f(m[3]);
        } else widget->setFrom(n.pos);
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

    VRReasonerPtr r = VRReasoner::create();
    auto query = [&](string q) { return r->process(q, ontology); };

    auto layers = query("q(x):Layer(x)");
    if (layers.size() == 0) return;
    auto layer = layers[0]; // only use first layer
    interactionDiagram = DiagramPtr( new Diagram() );

    string q_subjects = "q(x):ActiveProcessComponent(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(q_subjects) ) {
        auto n = Node(subject);
        if (auto label = subject->getValue("hasModelComponentLable") ) n.label = label->value;
        interactionDiagram->addNode( n );
        cout << " " << subject->toString() << endl;
    }

    string q_messages = "q(x):StandardMessageExchange(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto message : query(q_messages) ) {
        string q_message = "q(x):MessageSpec(x);StandardMessageExchange("+message->getName()+");is("+message->getName()+".hasMessageType,x)";
        auto n = Node(message);
        auto msgs = query(q_message);
        if (msgs.size())
            if (auto label = msgs[0]->getValue("hasModelComponentLable") ) n.label = label->value;
        interactionDiagram->addNode(n);
        cout << " " << message->toString() << endl;
    }

    //interactionDiagram->connect( n );
}
