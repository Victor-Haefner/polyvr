#include "VRProduction.h"
#include "VRLogistics.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace std;
using namespace OSG;

// Ontology --------------------------------------------------

int guid() {
    static int id = 0;
    id++;
    return id;
}

VRNamedID::VRNamedID() {
    ID = guid();
}

VRConcept::VRConcept(string name) {
    this->name = name;
}

void VRConcept::append(VRConcept* c) {
    children[c->ID] = c;
}

VRConcept* VRConcept::append(string name) {
    auto c = new VRConcept(name);
    append(c);
    return c;
}

VRTaxonomy::VRTaxonomy() {
    thing = new VRConcept("Thing");
}

VRConcept* VRTaxonomy::get(string name, VRConcept* p) {
    if (p == 0) p = thing;
    if (p->name == name) return p;
    VRConcept* c = 0;
    for (auto ci : p->children) {
        c = get(name, ci.second);
        if (c) return c;
    }
    return c;
}

VROntology::VROntology() {
    taxonomy = new VRTaxonomy();
}

void VROntology::merge(VROntology* o) {
    for (auto c : o->taxonomy->thing->children)
        taxonomy->thing->append(c.second);
}

void VROntologyInstance::set(string name, string value) {
    for (auto p : concept->properties) {
        if (p.second->name != name) continue;
        properties[p.second->ID] = value;
        break;
    }
}

void VROntologyInstance::set(string name, VROntologyInstance* i) {
    set(name, i->name);
}

VROntologyInstance::VROntologyInstance(string name, VRConcept* c) {
    this->name = name;
    concept = c;
    for (auto p : c->properties) properties[p.first] = ""; // init empty properties -> default values?
}

VROntologyInstance* VROntology::addInstance(string concept, string name) {
    auto c = taxonomy->get(concept);
    auto i = new VROntologyInstance(name, c);
    return i;
}


// Production --------------------------------------------------

void VRProcess::addFragment(VRProcessFragment* f) {
    fragments[f->ID] = f;
}

VRProductionJob::VRProductionJob(VRProcess* p) {
    process = p;
}

VRProduction::VRProduction() {
    intraLogistics = new FLogistics();
    network = intraLogistics->addNetwork();
}

VRProductionMachine::VRProductionMachine() {
    description = new VROntology();
}

VRProductionMachine* VRProduction::addMachine(VRGeometry* m) {
    auto pm = new VRProductionMachine();
    pm->geo = m;
    machines[pm->ID] = pm;
    FNode* n0 = network->addNode();
    for (auto n : network->getNodes()) if (n != n0) { n0->connect(n); n->connect(n0); }
    return pm;
}

VRProcess* VRProduction::getProcess(VRProduct* p) {
    auto pr = new VRProcess();
    processes.push_back(pr);
    return pr;
}

VRProductionJob* VRProduction::queueJob(VRProcess* p) {
    auto j = new VRProductionJob(p);
    jobs.push_back(j);
    return j;
}

VRProduct::VRProduct() {
    description = new VROntology();
}

void VRProduction::start() { ; }
void VRProduction::stop() { ; }

VRProduction* VRProduction::test() {
    // ontologies
    auto mathOnto = new VROntology();
    auto featureOnto = new VROntology();
    auto kinematicsOnto = new VROntology();
    auto machineOnto = new VROntology();

    mathOnto->taxonomy->thing->append("Volume");
    mathOnto->taxonomy->thing->append("Vector");
    mathOnto->taxonomy->get("Vector")->append("Position");
    mathOnto->taxonomy->get("Vector")->append("Normal");
    mathOnto->taxonomy->get("Vector")->append("Direction");
    mathOnto->taxonomy->get("Volume")->append("Cylinder");
    mathOnto->taxonomy->get("Volume")->append("Box");

    featureOnto->merge(mathOnto);
    featureOnto->taxonomy->thing->append("Feature");
    featureOnto->taxonomy->get("Feature")->append("Borehole");

    kinematicsOnto->merge(mathOnto);
    kinematicsOnto->taxonomy->thing->append("Body");
    kinematicsOnto->taxonomy->thing->append("Joint");

    machineOnto->taxonomy->thing->append("Action");
    machineOnto->taxonomy->get("Action")->append("Task");
    machineOnto->taxonomy->get("Action")->append("Skill");


    auto productOnto = new VROntology();
    auto drillOnto = new VROntology();
    auto robotOnto = new VROntology();
    productOnto->merge(featureOnto);

    drillOnto->merge(featureOnto);
    drillOnto->merge(machineOnto);
    drillOnto->merge(kinematicsOnto);
    robotOnto->merge(mathOnto);
    robotOnto->merge(machineOnto);
    robotOnto->merge(kinematicsOnto);


    // drill ----------
    auto drillPos = drillOnto->addInstance("Position", "position"); // working space
    drillPos->set("x", "range -100mm 100mm");
    drillPos->set("y", "range -100mm 100mm");
    drillPos->set("z", "range -100mm 100mm");
    auto drillDir = drillOnto->addInstance("Direction", "direction"); // drill direction
    drillDir->set("x", "0");
    drillDir->set("y", "-1");
    drillDir->set("z", "0");
    auto drill = drillOnto->addInstance("Skill", "drill");
    drill->set("position", drillPos);
    drill->set("direction", drillDir);
    drill->set("depth", "range 0mm 50mm"); // hole depth
    drill->set("radius", "range 3mm 10mm"); // hole radius

    // robot ----------
    auto grabFrom = drillOnto->addInstance("Position", "grabFrom"); // working space
    grabFrom->set("x", "range -100mm 100mm");
    grabFrom->set("y", "range -100mm 100mm");
    grabFrom->set("z", "range -100mm 100mm");
    auto grabDir = drillOnto->addInstance("Direction", "grabDir"); // drill direction
    grabDir->set("x", "range -100mm 100mm");
    grabDir->set("y", "range -100mm 100mm");
    grabDir->set("z", "range -100mm 100mm");
    auto grab = robotOnto->addInstance("Skill", "grab");
    grab->set("position", grabFrom);
    grab->set("direction", grabDir);

    auto moveBeg = drillOnto->addInstance("Position", "moveBeg"); // working space
    moveBeg->set("x", "range -100mm 100mm");
    moveBeg->set("y", "range -100mm 100mm");
    moveBeg->set("z", "range -100mm 100mm");
    auto moveEnd = drillOnto->addInstance("Direction", "moveEnd"); // drill direction
    moveEnd->set("x", "range -100mm 100mm");
    moveEnd->set("y", "range -100mm 100mm");
    moveEnd->set("z", "range -100mm 100mm");
    auto move = robotOnto->addInstance("Skill", "move");
    move->set("position", moveBeg);
    move->set("position", moveEnd);
    move->set("speed", "range 0mm/s 500mm/s"); // robot speed


    // production -----------------------------------------------
    auto machine = new VRGeometry("machine");
    machine->setPrimitive("Box", "1 2 1 1 1 1");

    auto p = new VRProduction();
    auto m1 = p->addMachine((VRGeometry*)machine->duplicate());
    auto m2 = p->addMachine((VRGeometry*)machine->duplicate());
    m1->description->merge(machineOnto);
    m2->description->merge(robotOnto);

    auto product = new VRProduct();
    product->description->merge(productOnto);

    auto proc = p->getProcess(product);
    p->queueJob(proc);

    p->start();

    return p;
}
