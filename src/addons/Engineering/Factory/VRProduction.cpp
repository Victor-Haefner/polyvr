#include "VRProduction.h"
#include "VRLogistics.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

#include <GL/glut.h>

using namespace std;
using namespace OSG;

// Production --------------------------------------------------

void VRProcess::addFragment(VRProcessFragment* f) {
    fragments[f->ID] = f;
}

VRProductionJob::VRProductionJob(VRProduct* p) {
    product = p;
    process = new VRProcess("production_of_"+p->name);
}

VRProduction::VRProduction() {
    intraLogistics = new FLogistics();
    network = intraLogistics->addNetwork();

    auto fkt = new VRFunction<int>("production_update", boost::bind(&VRProduction::update, this));
    VRSceneManager::getCurrent()->addUpdateFkt(fkt);
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

VRProductionJob* VRProduction::queueJob(VRProduct* p) {
    auto j = new VRProductionJob(p);
    jobs.push_back(j);
    return j;
}

VRProduct::VRProduct(string name) {
    this->name = name;
    description = new VROntology();
}

void VRProduction::stop() { running = false; }
void VRProduction::start() { running = true; }

VRProcessResult::VRProcessResult(string name) { this->name = name;}

VRProcessFragment::VRProcessFragment(string name) { this->name = name; }
VRProcessResult* VRProcessFragment::addResult(string name) {
    auto r = new VRProcessResult(name);
    results.push_back(r);
    return r;
}

void VRProduction::update() {
    if (!running) return;

    int t = glutGet(GLUT_ELAPSED_TIME);
    int dt = t-last_takt;
    if (dt < takt) return;
    last_takt = t;

    VROntology* production = description;
    for (VRProductionJob* job : jobs) {
        VROntology* product = job->product->description;

        VROntology* jobOnto = new VROntology();
        jobOnto->merge(production);
        jobOnto->merge(product);


        // what process to produce product p
        string q = "r(process) : r.result.state=done";

        cout << jobOnto->answer(q) << endl;

        /*auto features = product->getInstances("Feature"); // get all features and derived concepts
        cout << " job with " << features.size() << " features" << endl;
        for (auto feature : features) {
            // new process fragment with result state of feature
            auto fragment = new VRProcessFragment(feature->name);
            fragment->addResult(feature->name)->state = "done";
            job->process->addFragment(fragment);
            cout << "  feature " << feature->name << endl;

            // get fragment action by asking the
        }

        cout << job->process->toString() << endl;*/
    }
}

VRProcess::VRProcess(string name) { this->name = name; }

string VRProcess::toString() {
    string data = "process " + name + "\n";
    for (auto f : fragments) data += " fragment " + f.second->name + "\n";
    return data;
}

VRObject* VRProduction::test() {
    // ontologies
    auto mathOnto = new VROntology();
    auto featureOnto = new VROntology();
    auto machineOnto = new VROntology();
    auto drillingOnto = new VROntology();
    auto processingOnto = new VROntology();
    auto actionOnto = new VROntology();
    auto processOnto = new VROntology();
    auto productOnto = new VROntology();
    auto productionOnto = new VROntology();
    auto drillOnto = new VROntology();
    auto robotOnto = new VROntology();
    auto objectOnto = new VROntology();

    mathOnto->addConcept("Volume");
    mathOnto->addConcept("Vector");
    mathOnto->addConcept("Quaternion");
    mathOnto->getConcept("Vector")->addProperty("x", "float");
    mathOnto->getConcept("Vector")->addProperty("y", "float");
    mathOnto->getConcept("Vector")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("x", "float");
    mathOnto->getConcept("Quaternion")->addProperty("y", "float");
    mathOnto->getConcept("Quaternion")->addProperty("z", "float");
    mathOnto->getConcept("Quaternion")->addProperty("w", "float");
    mathOnto->addConcept("Position", "Vector");
    mathOnto->addConcept("Normal", "Vector");
    mathOnto->addConcept("Direction", "Vector");
    mathOnto->addConcept("Orientation", "Quaternion");
    mathOnto->addConcept("Box", "Volume");
    mathOnto->getConcept("Box")->addProperty("min", "Vector");
    mathOnto->getConcept("Box")->addProperty("max", "Vector");
    mathOnto->addRule("b(Box).min>=p(Position) & b.max<=p ? inside(p,b)=1");
    // TODO: quaternion rotation rule to change direction

    objectOnto->addConcept("Object");
    objectOnto->getConcept("Object")->addProperty("position", "Position");
    objectOnto->getConcept("Object")->addProperty("orientation", "Orientation");

    processOnto->addConcept("Feature");

    featureOnto->merge(mathOnto);
    featureOnto->addConcept("Feature");
    featureOnto->getConcept("Feature")->addProperty("State", "int");

    actionOnto->addConcept("Action");
    actionOnto->getConcept("Action")->append("Task");
    actionOnto->getConcept("Action")->append("Skill");

    machineOnto->merge(mathOnto);
    machineOnto->addConcept("Machine");
    machineOnto->getConcept("Machine")->append("Productionmachine");

    processingOnto->merge(featureOnto);
    processingOnto->merge(actionOnto);
    processingOnto->addConcept("Processing", "Action");
    processingOnto->getConcept("Processing")->addProperty("result", "Feature");
    // if processing unset and feature unset and feature and processing result have same concept, result is set to feature
    processingOnto->addRule("s(Processing).state=unset & f(Feature).state=unset & s.result.CONCEPT=f.CONCEPT ? f.state=set & s.state=set & s.result=f");
    // if processing done, then result is done and skill is unset
    processingOnto->addRule("s(Processing).state=done ? s.result.state=done & s.state=unset");

    drillingOnto->merge(processingOnto);
    drillingOnto->addConcept("Borehole", "Feature");
    drillingOnto->getConcept("Borehole")->addProperty("radius", "float");
    drillingOnto->getConcept("Borehole")->addProperty("direction", "Direction");
    drillingOnto->getConcept("Borehole")->addProperty("position", "Position");
    drillingOnto->getConcept("Borehole")->addProperty("depth", "float");
    drillingOnto->addConcept("Drilling", "Processing");
    drillingOnto->getConcept("Drilling")->addProperty("volume", "Box");
    drillingOnto->getConcept("Drilling")->addProperty("direction", "Direction");
    drillingOnto->addRule("b(Drilling).state=set & inside(b.result.position,b.volume) & b.result.direction=b.direction ? b.state=done");

    productOnto->merge(featureOnto);
    productOnto->merge(objectOnto);
    productOnto->addConcept("Product", "Object");
    productOnto->getConcept("Product")->addProperty("feature", "Feature");

    productionOnto->merge(productOnto);
    productionOnto->merge(machineOnto);
    productionOnto->merge(processingOnto);

    drillOnto->merge(machineOnto);
    drillOnto->merge(processingOnto);
    drillOnto->merge(drillingOnto);

    robotOnto->merge(actionOnto);
    robotOnto->merge(machineOnto);
    robotOnto->merge(objectOnto);
    robotOnto->addConcept("Manipulation", "Skill");
    robotOnto->addConcept("Grab", "Manipulation");
    robotOnto->addConcept("Translate", "Manipulation");
    robotOnto->addConcept("Rotate", "Manipulation");
    robotOnto->getConcept("Manipulation")->addProperty("volume", "Box");
    robotOnto->getConcept("Manipulation")->addProperty("state", "String");
    robotOnto->getConcept("Manipulation")->addProperty("object", "Object");
    robotOnto->addRule("g(Grab).state=unset & inside(o(Object).position,g.volume) ? g.object=o & g.state=set");
    robotOnto->addRule("g(Grab).state=set & t(Translate).state=unset & inside(o(Object).position,t.volume) ? t.object=o & t.state=set");
    robotOnto->addRule("g(Grab).state=set & r(Rotate).state=unset & inside(o(Object).position,r.volume) ? r.object=o & r.state=set");
    robotOnto->addRule("t(Translate).state=set & inside(p(Position), t.volume)? t.state=done");
    robotOnto->addRule("g(Grab).state=set ? g.state=done");
    robotOnto->addRule("r(Rotation).state=set ? r.state=done");


    // production -----------------------------------------------
    auto production = new VRProduction();
    VRObject* anchor = new VRObject("production");
    auto machine = new VRGeometry("machine");
    machine->setPrimitive("Box", "1 2 1 1 1 1");

    // drill ----------
    auto drill = production->addMachine((VRGeometry*)machine->duplicate());
    drill->geo->translate(Vec3f(-1.5,0,0));
    drill->description->merge(drillOnto);
    auto drillPos = drill->description->addInstance("position", "Position"); // working space
    drillPos->set("x", "range -100mm 100mm");
    drillPos->set("y", "range -100mm 100mm");
    drillPos->set("z", "range -100mm 100mm");
    auto drillDir = drill->description->addInstance("direction", "Direction"); // drill direction
    drillDir->set("x", "0");
    drillDir->set("y", "-1");
    drillDir->set("z", "0");
    auto drilling = drill->description->addInstance("drill", "Skill");
    drilling->set("position", "position");
    drilling->set("direction", "direction");
    drilling->set("depth", "range 0mm 50mm"); // hole depth
    drilling->set("radius", "range 3mm 10mm"); // hole radius

    // robot ----------
    auto robot = production->addMachine((VRGeometry*)machine->duplicate());
    robot->geo->translate(Vec3f(1.5,0,0));
    robot->description->merge(robotOnto);

    robotOnto->addVectorInstance("wsMin", "Vector", "-1", "-1", "-1");
    robotOnto->addVectorInstance("wsMax", "Vector", "1", "1", "1");
    auto workSpace = robotOnto->addInstance("workSpace", "Box");
    workSpace->set("min", "wsMin");
    workSpace->set("max", "wsMax");
    auto translate = robotOnto->addInstance("translate", "Translate");
    translate->set("volume", "workSpace");

    auto grabFrom = robot->description->addInstance("grabFrom", "Position"); // working space
    grabFrom->set("x", "range -100mm 100mm"); // TODO: those ranges should be in rules
    grabFrom->set("y", "range -100mm 100mm");
    grabFrom->set("z", "range -100mm 100mm");

    auto moveBeg = drillOnto->addInstance("Position", "moveBeg"); // working space
    moveBeg->set("x", "range -100mm 100mm");
    moveBeg->set("y", "range -100mm 100mm");
    moveBeg->set("z", "range -100mm 100mm");
    auto moveEnd = drillOnto->addInstance("Direction", "moveEnd"); // drill direction
    moveEnd->set("x", "range -100mm 100mm");
    moveEnd->set("y", "range -100mm 100mm");
    moveEnd->set("z", "range -100mm 100mm");

    // product ---------------
    auto product = new VRProduct("testProduct");
    product->description->merge(productOnto);
    auto Product = product->description->addInstance("Product", "testProduct");
    auto Btop = product->description->addInstance("Borehole", "Btop");
    auto Bbottom = product->description->addInstance("Borehole", "Bbottom");
    Product->add("Borehole", "Btop");
    Product->add("Borehole", "Bbottom");
    Btop->set("Radius", "0.1");
    Btop->set("Depth", "0.3");
    Btop->set("Entrypoint", "entryTop");
    Btop->set("Direction", "dirTop");
    auto entryTop = product->description->addInstance("Position", "entryTop");
    auto dirTop = product->description->addInstance("Direction", "dirTop");
    entryTop->set("x", "0");
    entryTop->set("y", "0.5");
    entryTop->set("z", "0");
    dirTop->set("x", "0");
    dirTop->set("y", "-1");
    dirTop->set("z", "0");
    Bbottom->set("Radius", "0.1");
    Bbottom->set("Depth", "0.3");
    Bbottom->set("Entrypoint", "entryBottom");
    Bbottom->set("Direction", "dirBottom");
    auto entryBottom = product->description->addInstance("Position", "entryBottom");
    auto dirBottom = product->description->addInstance("Direction", "dirBottom");
    entryBottom->set("x", "0");
    entryBottom->set("y", "-0.5");
    entryBottom->set("z", "0");
    dirBottom->set("x", "0");
    dirBottom->set("y", "1");
    dirBottom->set("z", "0");

    // production job -----------------------
    production->queueJob(product);
    production->start();

    anchor->addChild(drill->geo);
    anchor->addChild(robot->geo);
    return anchor;
}
