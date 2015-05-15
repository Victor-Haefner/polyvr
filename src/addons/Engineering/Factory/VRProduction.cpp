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
    process = new VRProcess();
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

VRProduct::VRProduct() {
    description = new VROntology();
}

void VRProduction::stop() { running = false; }
void VRProduction::start() { running = true; }

void VRProduction::update() {
    if (!running) return;

    int t = glutGet(GLUT_ELAPSED_TIME);
    int dt = t-last_takt;
    if (dt < takt) return;
    last_takt = t;

    cout << "production jobs " << jobs.size() << endl;
    for (VRProductionJob* job : jobs) {
        auto features = job->product->description->getInstances("Feature"); // get all features and derived concepts
        cout << " job with " << features.size() << " features" << endl;
        for (VROntologyInstance* feature : features) {
            cout << "  feature " << feature->name << endl;
            //if ()
        }
    }
}

VRObject* VRProduction::test() {
    // ontologies
    auto mathOnto = new VROntology();
    auto featureOnto = new VROntology();
    auto kinematicsOnto = new VROntology();
    auto machineOnto = new VROntology();

    mathOnto->taxonomy->thing->append("Volume");
    mathOnto->taxonomy->thing->append("Vector");
    mathOnto->taxonomy->get("Vector")->addProperty("x", "float");
    mathOnto->taxonomy->get("Vector")->addProperty("y", "float");
    mathOnto->taxonomy->get("Vector")->addProperty("z", "float");
    mathOnto->taxonomy->get("Vector")->append("Position");
    mathOnto->taxonomy->get("Vector")->append("Normal");
    mathOnto->taxonomy->get("Vector")->append("Direction");
    mathOnto->taxonomy->get("Volume")->append("Cylinder");
    mathOnto->taxonomy->get("Volume")->append("Box");

    featureOnto->merge(mathOnto);
    featureOnto->taxonomy->thing->append("Feature");
    featureOnto->taxonomy->get("Feature")->append("Borehole");
    featureOnto->taxonomy->get("Feature")->addProperty("State", "int");
    featureOnto->taxonomy->get("Borehole")->addProperty("Radius", "float");
    featureOnto->taxonomy->get("Borehole")->addProperty("Direction", "Direction");
    featureOnto->taxonomy->get("Borehole")->addProperty("Entrypoint", "Position");
    featureOnto->taxonomy->get("Borehole")->addProperty("Depth", "float");

    kinematicsOnto->merge(mathOnto);
    kinematicsOnto->taxonomy->thing->append("Body");
    kinematicsOnto->taxonomy->thing->append("Joint");

    machineOnto->taxonomy->thing->append("Action");
    machineOnto->taxonomy->get("Action")->append("Task");
    machineOnto->taxonomy->get("Action")->append("Skill");


    auto productOnto = new VROntology();
    productOnto->merge(featureOnto);
    productOnto->taxonomy->thing->append("Product");
    productOnto->taxonomy->get("Product")->addProperty("Feature", "Feature");

    auto drillOnto = new VROntology();
    drillOnto->merge(featureOnto);
    drillOnto->merge(machineOnto);
    drillOnto->merge(kinematicsOnto);

    auto robotOnto = new VROntology();
    robotOnto->merge(mathOnto);
    robotOnto->merge(machineOnto);
    robotOnto->merge(kinematicsOnto);


    // production -----------------------------------------------
    auto production = new VRProduction();
    VRObject* anchor = new VRObject("production");
    auto machine = new VRGeometry("machine");
    machine->setPrimitive("Box", "1 2 1 1 1 1");

    // drill ----------
    auto drill = production->addMachine((VRGeometry*)machine->duplicate());
    drill->geo->translate(Vec3f(-1.5,0,0));
    drill->description->merge(drillOnto);
    auto drillPos = drill->description->addInstance("Position", "position"); // working space
    drillPos->set("x", "range -100mm 100mm");
    drillPos->set("y", "range -100mm 100mm");
    drillPos->set("z", "range -100mm 100mm");
    auto drillDir = drill->description->addInstance("Direction", "direction"); // drill direction
    drillDir->set("x", "0");
    drillDir->set("y", "-1");
    drillDir->set("z", "0");
    auto drilling = drill->description->addInstance("Skill", "drill");
    drilling->set("position", "position");
    drilling->set("direction", "direction");
    drilling->set("depth", "range 0mm 50mm"); // hole depth
    drilling->set("radius", "range 3mm 10mm"); // hole radius

    // robot ----------
    auto robot = production->addMachine((VRGeometry*)machine->duplicate());
    robot->geo->translate(Vec3f(1.5,0,0));
    robot->description->merge(robotOnto);
    auto grabFrom = drillOnto->addInstance("Position", "grabFrom"); // working space
    grabFrom->set("x", "range -100mm 100mm"); // TODO: those ranges should be in rules
    grabFrom->set("y", "range -100mm 100mm");
    grabFrom->set("z", "range -100mm 100mm");
    auto grabDir = drillOnto->addInstance("Direction", "grabDir"); // drill direction
    grabDir->set("x", "range -100mm 100mm");
    grabDir->set("y", "range -100mm 100mm");
    grabDir->set("z", "range -100mm 100mm");
    auto grab = robotOnto->addInstance("Skill", "grab");
    grab->set("position", "grabFrom");
    grab->set("direction", "grabDir");

    auto moveBeg = drillOnto->addInstance("Position", "moveBeg"); // working space
    moveBeg->set("x", "range -100mm 100mm");
    moveBeg->set("y", "range -100mm 100mm");
    moveBeg->set("z", "range -100mm 100mm");
    auto moveEnd = drillOnto->addInstance("Direction", "moveEnd"); // drill direction
    moveEnd->set("x", "range -100mm 100mm");
    moveEnd->set("y", "range -100mm 100mm");
    moveEnd->set("z", "range -100mm 100mm");
    auto move = robotOnto->addInstance("Skill", "move");
    move->set("position", "moveBeg");
    move->set("position", "moveEnd");
    move->set("speed", "range 0mm/s 500mm/s"); // robot speed

    // product ---------------
    auto product = new VRProduct();
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
