#include "VRProcess.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "core/utils/VRStorage_template.h"
#include <iostream>

using namespace OSG;

VRProcess::Subject::Subject(VREntityPtr e) {
    ;
}

void VRProcess::Subject::update(graph_base::node& n) {}
void VRProcess::Fragment::update(graph_base::node& n) {}

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

graph_basePtr VRProcess::getInteractionDiagram() { return dynamic_pointer_cast<graph_base>(interactionDiagram); }
graph_basePtr VRProcess::getBehaviorDiagram(string subject) { return dynamic_pointer_cast<graph_base>(behaviorDiagrams[subject]); }

void VRProcess::update() {
    if (!ontology) return;

    VRReasonerPtr r = VRReasoner::create();
    auto query = [&](string q) { return r->process(q, ontology); };

    auto layers = query("q(x):Layer(x)");
    if (layers.size() == 0) return;

    auto layer = layers[0]; // only use first layer
    interactionDiagram = InteractionDiagramPtr( new InteractionDiagram() );

    string subjects = "q(x):Actor(x);Layer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(subjects) ) {
        interactionDiagram->addNode( Subject(subject) );
        cout << " " << subject->toString() << endl;
    }
}
