#include "VRProcess.h"

#include "addons/Semantics/Reasoning/VROntology.h"
#include "addons/Semantics/Reasoning/VRReasoner.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "core/utils/VRStorage_template.h"
#include "core/objects/VRTransform.h"
#include <iostream>

using namespace OSG;


VRProcessDiagram::VRProcessDiagram() {}
VRProcessDiagram::~VRProcessDiagram() {}
VRProcessDiagramPtr VRProcessDiagram::create() { return VRProcessDiagramPtr( new VRProcessDiagram() ); }

void VRProcessDiagram::update(int i, bool changed) {
    if (processNodes.count(i)) processNodes[i]->update(nodes[i], changed);
}

vector<VRProcessNodePtr> VRProcessDiagram::getNodes() {
    vector<VRProcessNodePtr> res;
    for (auto p : processNodes) res.push_back(p.second);
    return res;
}

void VRProcessDiagram::remNode(int i) {
    Graph::remNode(i);
    nodesByName.erase(processNodes[i]->label);
    processNodes.erase(i);
}

void VRProcessDiagram::clear() {
    Graph::clear();
    nodesByName.clear();
    processNodes.clear();
}

VRProcessNode::VRProcessNode(string name, PROCESS_WIDGET type, int ID, int sID) : type(type), label(name), ID(ID), subject(sID) { setName(name); }
VRProcessNode::~VRProcessNode() {}
VRProcessNodePtr VRProcessNode::create(string name, PROCESS_WIDGET type, int ID, int sID) { return VRProcessNodePtr( new VRProcessNode(name, type, ID, sID) ); }

void VRProcessNode::update(Graph::node& n, bool changed) { // called when graph node changes
    if (widget && !widget->isDragged() && changed) widget->setFrom(n.box.center());

    if (widget && widget->isDragged()) {
        auto m = widget->getDragParent()->getMatrixTo( widget );
        n.box.setCenter( Vec3d(m[3]) );
    }
}

Vec3d VRProcessNode::getPosition(Vec3d p, float scale) {
    if (entity) {
        // width, height, X, Y, page aspect ratio
        vector<float> data = entity->getAllValues<float>("hasVisualisationDataProperty");
        if (data.size() > 4) p = Vec3d(data[2]*data[4],0,data[3])*scale/data[1];
    }
    return p;
}

VREntityPtr VRProcessNode::getEntity() { return entity; }
int VRProcessNode::getID() { return ID; }
string VRProcessNode::getLabel() { return label; }
int VRProcessNode::getSubjectID() { return subject; }

void VRProcessNode::setWaitForExternalAction(bool b) { waitForExternalAction = b; }

VRProcess::VRProcess(string name) {
    setStorageType("Process");
    setPersistency(0);
    setNameSpace("Process");
    setName(name);

    storeObj("Ontology", ontology);

    ontology = VROntology::create(name);
    //ontology->
    update();
}

VRProcessPtr VRProcess::create(string name) { return VRProcessPtr( new VRProcess(name) ); }
VRProcessPtr VRProcess::ptr() { return shared_from_this(); }

void VRProcess::open(string path) {
    if (!ontology) ontology = VROntology::create(getBaseName());
    ontology->openOWL(path);
    update();
}

void VRProcess::setOntology(VROntologyPtr o) { ontology = o; update(); }
VROntologyPtr VRProcess::getOntology() { return ontology; }

VRProcessDiagramPtr VRProcess::getInteractionDiagram() {
    if (!interactionDiagram) interactionDiagram = VRProcessDiagram::create();
    return interactionDiagram;
}

VRProcessDiagramPtr VRProcess::getBehaviorDiagram(int subject) { return behaviorDiagrams.count(subject) ? behaviorDiagrams[subject] : 0; }

bool VRProcess::isFunctionState(VRProcessNodePtr state){ return (!state->isSendState && !state->isReceiveState); }
bool VRProcess::isSendState(VRProcessNodePtr state){ return state->isSendState; }
bool VRProcess::isReceiveState(VRProcessNodePtr state){ return state->isReceiveState; }
bool VRProcess::isInitialState(VRProcessNodePtr state){ return state->isInitialState; }

vector<VRProcessNodePtr> VRProcess::getSubjects() {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    for (auto node : interactionDiagram->processNodes) {
        if (node.second) if (node.second->type == SUBJECT) res.push_back(node.second);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getMessages() {
    vector<VRProcessNodePtr> res;
    if (!interactionDiagram) return res;
    for (auto node : interactionDiagram->processNodes) {
        if (!node.second) continue;
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

vector<VRProcessNodePtr> VRProcess::getOutgoingMessages(int subjectID) {
    auto diag = behaviorDiagrams[subjectID];
    auto nextNodes = diag->getNextNodes(subjectID);
    vector<VRProcessNodePtr> res;
    for (auto node : nextNodes){
        res.push_back(diag->processNodes[node.ID]);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getIncomingMessages(int subjectID) {
    auto diag = behaviorDiagrams[subjectID];
    auto previousNodes = diag->getPreviousNodes(subjectID);
    vector<VRProcessNodePtr> res;
    for (auto node : previousNodes){
        res.push_back(diag->processNodes[node.ID]);
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

vector<VRProcessNodePtr> VRProcess::getMessageReceiver(int messageID) {
    auto diag = interactionDiagram;
    auto nextNodes = diag->getNextNodes(messageID);
    vector<VRProcessNodePtr> res;
    for (auto node : nextNodes){
        res.push_back(diag->processNodes[node.ID]);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getMessageSender(int messageID) {
    auto diag = interactionDiagram;
    auto previousNodes = diag->getPreviousNodes(messageID);
    vector<VRProcessNodePtr> res;
    for (auto node : previousNodes) {
        res.push_back(diag->processNodes[node.ID]);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getSubjectStates(int subjectID) {
    vector<VRProcessNodePtr> res;
    if (!behaviorDiagrams.count(subjectID)) return res;
    auto d = behaviorDiagrams[subjectID];
    for (auto node : d->processNodes) {
        if (node.second->type == STATE) res.push_back(node.second);
    }
    return res;
}

VRProcessNodePtr VRProcess::getSubjectState(int subjectID, string name) {
    if (!behaviorDiagrams.count(subjectID)) return 0;
    auto d = behaviorDiagrams[subjectID];
    if (!d->nodesByName.count(name)) return 0;
    int nID = d->nodesByName[name];
    if (!d->processNodes.count(nID)) return 0;
    auto node = d->processNodes[nID];
    if (node->type == STATE && node->label == name) return node;
    return 0;
}

VRProcessNodePtr VRProcess::getSubject(int ID) {
    return getNode(ID);
}

VRProcessNodePtr VRProcess::getSubjectByName(string name) {
    return getNodeByName(name);
}

void VRProcess::printNodes(VRProcessDiagramPtr d){
    for (auto node : d->processNodes) cout << node.second->getLabel() << endl;
}

vector<VRProcessNodePtr> VRProcess::getSubjectTransitions(int subjectID) {
    vector<VRProcessNodePtr> res;
    if (!behaviorDiagrams.count(subjectID)) return res;
    auto d = behaviorDiagrams[subjectID];
    for (auto node : d->processNodes) {
        if (node.second->type == TRANSITION) res.push_back(node.second);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getStateTransitions(int subjectID, int stateID) {
    vector<VRProcessNodePtr> res;
    if (!behaviorDiagrams.count(subjectID)) return res;
    auto d = behaviorDiagrams[subjectID];
    auto neighbors = d->getNeighbors( stateID );

    for (auto neighbor : neighbors){
        auto transition = d->processNodes[neighbor.ID];
        res.push_back(transition);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getStateOutTransitions(int subjectID, int stateID) {
    vector<VRProcessNodePtr> res;
    if (!behaviorDiagrams.count(subjectID)) return res;
    auto d = behaviorDiagrams[subjectID];
    auto neighbors = d->getNextNodes( stateID );

    for (auto neighbor : neighbors){
        auto transition = d->processNodes[neighbor.ID];
        res.push_back(transition);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getTransitionStates(int subjectID, int transitionID) {
    auto d = behaviorDiagrams[subjectID];
    auto neighbors = d->getNeighbors( transitionID );
    vector<VRProcessNodePtr> res;
    for (auto node : neighbors) {
        auto state = d->processNodes[node.ID];
        res.push_back(state);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getTransitionSourceState(int subjectID, int transitionID) {
    auto d = behaviorDiagrams[subjectID];
    auto neighbors = d->getNextNodes( transitionID );
    vector<VRProcessNodePtr> res;
    for (auto node : neighbors) {
        auto state = d->processNodes[node.ID];
        res.push_back(state);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getTransitions(int subjectID) {
    vector<VRProcessNodePtr> res;
    auto d = behaviorDiagrams[subjectID];

    for (auto node : d->processNodes){
        if (node.second->type == TRANSITION) res.push_back(node.second);
    }
    return res;
}

vector<VRProcessNodePtr> VRProcess::getInitialStates() {
    vector<VRProcessNodePtr> res;
    for (auto diag : behaviorDiagrams) {
        for (auto n : diag.second->processNodes) {
            if (n.second->isInitialState) {
                res.push_back( n.second );
                break;
            }
        }
    }

    if(!res.size()) cout << "VRProcess::getInitialStates, No initial states could be found." << endl;
    return res;
}

vector<VRProcessNodePtr> VRProcess::getStateMessages(VRProcessNodePtr state){
    if (!stateToMessages.count(state)) return vector<VRProcessNodePtr>();
    return stateToMessages[state];
}

vector<VRProcessNodePtr> VRProcess::getTransitionMessages(VRProcessNodePtr transition){
    if (!transitionToMessages.count(transition)) return vector<VRProcessNodePtr>();
    return transitionToMessages[transition];
}

void VRProcess::update() {
    if (!ontology) return;

    map<VREntityPtr, VRProcessNodePtr> messageEntityToNode;

    VRReasonerPtr reasoner = VRReasoner::create();
    reasoner->setVerbose(true,  false); //
    auto query = [&](string q) { return reasoner->process(q, ontology); };

    /** get interaction diagram **/
    auto layers = ontology->getEntities("ModelLayer");
    if (layers.size() == 0) return;
    auto layer = layers[0]; // only use first layer
    interactionDiagram = VRProcessDiagram::create();

    map<string, int> nodes;
    string q_subjects = "q(x):Subject(x);ModelLayer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto subject : query(q_subjects) ) {
        string label;
        if (auto l = subject->get("hasModelComponentLabel") ) label = l->value;
        //if subject->get("InterfaceSubject")...
        //add Multisubject
        int nID = addSubject(label, subject)->ID;
        if (auto ID = subject->get("hasModelComponentID") ) nodes[ID->value] = nID;
        //cout << " VRProcess::update subject: " << label << endl;
    }

    //TODO: parse InterfaceSubject
    //string q_interfaceSubjects = "q(x):InterfaceSubject(x);ModelLayer("+layer->getName()+");has("+layer->getName()+",x)";

    map<string, map<string, vector<VREntityPtr>>> messages;
    string q_messages = "q(x):MessageExchange(x);ModelLayer("+layer->getName()+");has("+layer->getName()+",x)";
    for ( auto message : query(q_messages) ) {
        string sender;
        string receiver;
        if (auto s = message->get("hasSender") ) sender = s->value;
        else cout << "Warning! in VRProcess::update, no sender for message '" << message->getName() << "'" << endl;
        if (auto r = message->get("hasReceiver") ) receiver = r->value;
        else cout << "Warning! in VRProcess::update, no receiver for message '" << message->getName() << "'" << endl;
        if (sender != "" && receiver != "") messages[sender][receiver].push_back(message);
    }

    for ( auto sender : messages ) {
        for (auto receiver : sender.second) { // TODO: first version is pretty, but messes up the messaging!
            /*string label = "Msg:";
            vector<VREntityPtr> res;
            for (auto message : receiver.second) {
                string q_message = "q(x):MessageSpecification(x);MessageExchange("+message->getName()+");is(x,"+message->getName()+".hasMessageType)";
                auto msgs = query(q_message);
                if (msgs.size())
                    if (auto l = msgs[0]->get("hasModelComponentLabel") ) label += "\n - " + l->value;
                res.push_back(msgs[0]);
            }
            auto messageNode = addMessage(label, nodes[sender.first], nodes[receiver.first]);
            for (auto entity : res) {
                messageEntityToNode[entity] = messageNode;
            }*/

            for (auto message : receiver.second) {
                string q_message = "q(x):MessageSpecification(x);MessageExchange("+message->getName()+");is(x,"+message->getName()+".hasMessageType)";
                auto msgs = query(q_message);
                if (msgs.size()) {
                    if (auto l = msgs[0]->get("hasModelComponentLabel") ) {
                        auto messageNode = addMessage(l->value, nodes[sender.first], nodes[receiver.first], 0, msgs[0]);
                        messageEntityToNode[msgs[0]] = messageNode;
                    }
                }
            }
        }
    }

    //return;
    /** get behavior diagrams **/

    for (auto behavior : query("q(x):SubjectBehavior(x)")) {

        map<VREntityPtr, TRANSITION_CONDITION> transitionToCondition;
        map<VREntityPtr,VREntityPtr> transitionsToMessages;

        auto behaviorDiagram = VRProcessDiagram::create();
        string q_Subject = "q(x):Subject(x);SubjectBehavior("+behavior->getName()+");has(x,"+behavior->getName()+")";
        auto subjects = query(q_Subject);
        if (subjects.size() == 0) continue;
        auto subject = subjects[0];
        auto ID = subject->get("hasModelComponentID");
        int sID = nodes[ID->value];
        behaviorDiagrams[sID] = behaviorDiagram;

        string q_States = "q(x):State(x);SubjectBehavior("+behavior->getName()+");has("+behavior->getName()+",x)"; //State
        for (auto state : query(q_States)) {
            string label;
            if (auto l = state->get("hasModelComponentLabel") ) label = l->value;
            int nID = addState(label, sID, state)->ID;
            if (auto ID = state->get("hasModelComponentID") ) nodes[ID->value] = nID;
        }

        map<string, map<string, vector<VREntityPtr>> > transitions;
        string q_Transitions = "q(x):Transition(x);SubjectBehavior("+behavior->getName()+");has("+behavior->getName()+",x)";
        for (auto transition : query(q_Transitions)) {
            string source;
            string target;
            if (auto s = transition->get("hasSourceState") ) source = s->value;
            if (auto r = transition->get("hasTargetState") ) target = r->value;
            transitions[source][target].push_back(transition);

            if (auto transitionCondition = transition->get("hasTransitionCondition") ) {
                auto transitionConditionEntity = ontology->getEntity(transitionCondition->value);
                if (transitionConditionEntity->is_a("ReceiveTransitionCondition") ) transitionToCondition[transition] = RECEIVE_CONDITION;
                else if (transitionConditionEntity->is_a("SendTransitionCondition") ) transitionToCondition[transition] = SEND_CONDITION;

                /* old standard pass
                if (auto messageConnector = transition->get("refersTo")) {
                    if (auto connector = ontology->getEntity(messageConnector->value)) {
                        if (auto m = connector->get("hasMessageType")){
                            if (auto messageEntity = ontology->getEntity(m->value)) transitionsToMessages[transition] = messageEntity;
                        }
                    }
                }
                */
                if (auto messageConnector = transitionConditionEntity->get("requiresPerformedMessageExchange")){
                    if (auto connector = ontology->getEntity(messageConnector->value)) {
                        if (auto m = connector->get("hasMessageType")) {
                            if (auto messageEntity = ontology->getEntity(m->value)) transitionsToMessages[transition] = messageEntity;
                        }
                    }
                }
            }
        }

        for ( auto source : transitions ) {
            for (auto target : source.second) {
                for (auto transitionEntity : target.second) {
                    string msg = transitionEntity->get("hasModelComponentLabel")->value;
                    auto transitionNode = addTransition(msg, sID, nodes[source.first], nodes[target.first], behaviorDiagram);

                    if (transitionToCondition.count(transitionEntity)) {
                        auto node = getNode( nodes[source.first] );
                        transitionNode->transition = transitionToCondition[transitionEntity];
                        if (transitionNode->transition == SEND_CONDITION) node->isSendState = true;
                        if (transitionNode->transition == RECEIVE_CONDITION) node->isReceiveState = true;

                        if (transitionsToMessages.count(transitionEntity)) { // ?
                            auto messageEntity = transitionsToMessages[transitionEntity];
                            auto messageNode = messageEntityToNode[messageEntity];
                            transitionToMessages[transitionNode].push_back(messageNode);
                            auto sourceNode = getNode(nodes[source.first]);
                            auto targetNode = getNode(nodes[target.first]);
                            stateToMessages[sourceNode].push_back(messageNode);
                            stateToMessages[targetNode].push_back(messageNode);
                        }
                    }
                }
            }
        }
    }
}

void VRProcess::remNode(VRProcessNodePtr n) {
    if (auto diag = interactionDiagram) {
        if (diag->processNodes.count(n->ID)) {
            diag->remNode(n->ID);
            if (behaviorDiagrams.count(n->ID)) behaviorDiagrams.erase(n->ID);
        }
    }

    for (auto diag : behaviorDiagrams) {
        if (diag.second->processNodes.count(n->ID)) diag.second->remNode(n->ID);
    }
}

VRProcessNodePtr VRProcess::addSubject(string name, VREntityPtr e) {
    if (!interactionDiagram) interactionDiagram = VRProcessDiagram::create();
    auto sID = interactionDiagram->addNode();
    auto s = VRProcessNode::create(name, SUBJECT, sID, sID);
    s->entity = e;
    interactionDiagram->processNodes[sID] = s;
    interactionDiagram->nodesByName[s->label] = sID;
    auto behaviorDiagram = VRProcessDiagram::create();
    behaviorDiagrams[sID] = behaviorDiagram;
    return s;
}

VRProcessNodePtr VRProcess::getMessage(string msg, int sID1, int sID2) {
    for (auto m : getMessages()) {
        if (m->label == msg) {
            auto sender = getMessageSender(m->getID())[0];
            auto receiver = getMessageReceiver(m->getID())[0];
            if (m->label == msg && sender->getID() == sID1 && receiver->getID() == sID2) return m;
        }
    }
    return 0;
}

VRProcessNodePtr VRProcess::addMessage(string name, int i, int j, VRProcessDiagramPtr diag, VREntityPtr e) {
    if (!diag) diag = interactionDiagram;
    if (!diag) return 0;

    // only add message once!
    auto m = getMessage(name, i, j);
    if (m) return m;

    auto mID = diag->addNode();
    m = VRProcessNode::create(name, MESSAGE, mID, i);
    m->entity = e;
    diag->processNodes[mID] = m;
    diag->nodesByName[m->label] = mID;
    diag->connect(i, mID, Graph::HIERARCHY);
    diag->connect(mID, j, Graph::DEPENDENCY);
    return m;
}

VRProcessNodePtr VRProcess::addState(string name, int sID, VREntityPtr e) {
    if (!behaviorDiagrams.count(sID)) return 0;
    auto diag = behaviorDiagrams[sID];
    //if (!diag) return 0;
    auto nID = diag->addNode();
    auto s = VRProcessNode::create(name, STATE, nID, sID);
    s->entity = e;
    diag->processNodes[nID] = s;
    diag->nodesByName[s->label] = nID;
    return s;
}

bool VRProcess::checkState(VRProcessNodePtr state) {
    if (!state) return false;
    int sID = state->subject;
    auto diag = behaviorDiagrams[sID];
    if (!diag) cout << "VRProcess::checkState: No Behavior for subject " << getSubjects()[sID]->getLabel() << " found." << endl;

    if (state->type != STATE) {
        cout << "VRProcess::checkState: The given Processnode if not of type STATE." << endl;
        return false;
    }
    return true;
}

void VRProcess::setInitialState(VRProcessNodePtr state) {
    if (!checkState(state)) return;
    auto diag = behaviorDiagrams[state->subject];
    for (auto n : diag->processNodes) n.second->isInitialState = false; // reset all
    state->isInitialState = true;
}

void VRProcess::setSendState(VRProcessNodePtr sender, VRProcessNodePtr sendTransition, int recvSubject, string message) {
    if (!checkState(sender)) return;
    if (!sendTransition) return;

    sender->isSendState = true;
    sender->isReceiveState = false;
    sendTransition->transition = SEND_CONDITION;

    auto messageNode = addMessage(message, sender->subject, recvSubject, 0, 0);
    transitionToMessages[sendTransition].push_back(messageNode);
    stateToMessages[sender].push_back(messageNode);
}

void VRProcess::setReceiveState(VRProcessNodePtr receiver, VRProcessNodePtr recvTransition, int sendSubject, string message) {
    if (!checkState(receiver)) return;
    if (!recvTransition) return;

    receiver->isSendState = false;
    receiver->isReceiveState = true;
    recvTransition->transition = RECEIVE_CONDITION;

    auto messageNode = addMessage(message, sendSubject, receiver->subject, 0, 0);
    transitionToMessages[recvTransition].push_back(messageNode);
    stateToMessages[receiver].push_back(messageNode);
}

VRProcessNodePtr VRProcess::addTransition(string name, int sID, int i, int j, VRProcessDiagramPtr diag, VRProcessCbPtr callback ){
    if (!diag) diag = behaviorDiagrams[sID];
    if (!diag) return 0;
    auto tID = diag->addNode();
    auto t = VRProcessNode::create(name, TRANSITION, tID, sID);
    t->callback = callback;
    diag->processNodes[tID] = t;
    diag->nodesByName[t->label] = tID;
    diag->connect(i, tID, Graph::HIERARCHY);
    diag->connect(tID, j, Graph::DEPENDENCY);
    return t;
}

VRProcessNodePtr VRProcess::getNodeByName(string name, VRProcessDiagramPtr diag) {
    if (diag && diag->nodesByName.count(name)) return getNode(diag->nodesByName[name], diag);
    if (interactionDiagram && interactionDiagram->nodesByName.count(name)) return getNode(interactionDiagram->nodesByName[name], interactionDiagram);
    for (auto diag : behaviorDiagrams) if (diag.second && diag.second->nodesByName.count(name)) return getNode(diag.second->nodesByName[name], diag.second);
    return 0;
}

VRProcessNodePtr VRProcess::getNode(int i, VRProcessDiagramPtr diag) {
    if (diag && diag->processNodes.count(i)) return diag->processNodes[i];
    if (interactionDiagram && interactionDiagram->processNodes.count(i)) return interactionDiagram->processNodes[i];
    for (auto diag : behaviorDiagrams) if (diag.second && diag.second->processNodes.count(i)) return diag.second->processNodes[i];
    return 0;
}

VRProcessNodePtr VRProcess::getTransitionState(VRProcessNodePtr transition) {
    int tID = transition->getID();
    int sID = transition->subject;
    auto diag = behaviorDiagrams[sID];
    auto nextNodes = diag->getNextNodes(tID);
    if (nextNodes.size() == 0) return 0;
    return getNode(nextNodes[0].ID, diag);
}





