#include "VRSyncNode.h"
#include "core/networking/tcp/VRTCPServer.h"

#include "core/objects/VRLight.h"
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/OSGMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/VRCamera.h"
#include "core/setup/VRSetup.h"
//#include "core/math/pose.h"
#include "core/utils/VRStorage_template.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/import/VRImport.h"
#include <OpenSG/OSGMultiPassMaterial.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

#include <OpenSG/OSGNode.h>
#include <OpenSG/OSGNodeCore.h>
#include <OpenSG/OSGTransformBase.h>
#include "core/objects/OSGTransform.h"
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGContainerIdMapper.h>
#include <OpenSG/OSGNameAttachment.h>

#include <OpenSG/OSGChangeList.h>
#include <OpenSG/OSGThreadManager.h>

// needed to filter GLId field masks
#include <OpenSG/OSGSurface.h>
#include <OpenSG/OSGGeoProperty.h>
#include <OpenSG/OSGGeoMultiPropertyData.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureObjRefChunk.h>
#include <OpenSG/OSGUniformBufferObjStd140Chunk.h>
#include <OpenSG/OSGShaderStorageBufferObjStdLayoutChunk.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGUniformBufferObjChunk.h>
#include <OpenSG/OSGShaderStorageBufferObjChunk.h>
#include <OpenSG/OSGShaderExecutableChunk.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGShaderProgram.h>
#include <OpenSG/OSGProgramChunk.h>

#include <bitset>

//BUGS:
/*
Known bugs:
    - syncedContainer seem not get get empty (keeps filling with same id)
    - in PolyVR: when syncNodes are not initialized on_scene_load but by manually triggering the script - the program will crash
    - syncNodes need to be translated on initialization else no Transform Node will be created to track (PolyVR optimisation initialises with Group Node type; Transform Node only creates after a Transformation)

*/

//TODO:
/*
    - create (Node/Child) change handling and applying on remote SyncNode
    - copy Changes from state for initialization of new remote SyncNode from master's State
    - remove Changes (derefferencing?)
*/

using namespace OSG;

void printGeoGLIDs(Geometry* geo) {
    if (!geo) {
        cout << " printGeoGLIDs, no geo!" << endl;
        return;
    }

    cout << " geometry GL IDs: " << Vec4i(
            0,//geo->getAttribVaoGLId(),
            geo->getClassicGLId(),
            geo->getAttGLId(),
            0//geo->getClassicVaoGLId()
        ) << endl;
}

ThreadRefPtr applicationThread;

VRSyncNode::VRSyncNode(string name) : VRTransform(name) {
    type = "SyncNode";
    selfID = getNode()->node->getId();
    changelist = VRSyncChangelist::create();
    applicationThread = dynamic_cast<Thread *>(ThreadManager::getAppThread());
	updateFkt = VRUpdateCb::create("SyncNode update", bind(&VRSyncNode::update, this));
	VRScene::getCurrent()->addUpdateFkt(updateFkt, 100000);
}

VRSyncNode::~VRSyncNode() {
    cout << " VRSyncNode::~VRSyncNode " << name << endl;
}

VRSyncNodePtr VRSyncNode::ptr() { return static_pointer_cast<VRSyncNode>( shared_from_this() ); }
VRSyncNodePtr VRSyncNode::create(string name) { return VRSyncNodePtr(new VRSyncNode(name) ); }

void VRSyncNode::setDoWrapping(bool b) { doWrapping = b; }
void VRSyncNode::setDoAvatars(bool b) { doAvatars = b; }

bool VRSyncNode::isSubContainer(const UInt32& id) {
    auto fct = factory->getContainer(id);
    if (!fct) return false;


    UInt32 syncNodeID = getNode()->node->getId();
    auto type = factory->findType(fct->getTypeId());

    function<bool(Node*)> checkAncestor = [&](Node* node) {
        if (!node) return false;
        if (node->getId() == syncNodeID) return true;
        Node* parent = node->getParent();
        return checkAncestor(parent);
    };

    if (type->isNode()) {
        Node* node = dynamic_cast<Node*>(fct);
        if (!node) return false;
        return checkAncestor(node);
    }

    if (type->isNodeCore()) {
        NodeCore* core = dynamic_cast<NodeCore*>(fct);
        if (!core) return false;
        for (auto node : core->getParents())
            if (checkAncestor(dynamic_cast<Node*>(node))) return true;
        return false;
    }

    string typeName = fct->getTypeName();

    Attachment* att = dynamic_cast<Attachment*>(fct);
    if (att) {
        auto parents = att->getMFParents();
        for (UInt32 i = 0; i<parents->size(); i++) {
            FieldContainer* parent = parents->at(i);
            if (isSubContainer(parent->getId())) return true;
        }
        if (parents->size() == 0) {
            if (typeName == "MultiPassMaterial") return false; // Materials may not be attached to a geometry, thats fine!
            cout << " -- WARNING -- attachment FC has no parents: " << id << " type: " << fct->getTypeName();
            if (AttachmentContainer* attc = dynamic_cast<AttachmentContainer*>(fct)) {
                if (auto n = ::getName(attc)) cout << " named: " << n;
                else cout << " unnamed";
            }
            cout << endl;
        }
        return false;
    }

    if (typeName == "Viewport") return false; // TODO, use ID checks instead of string comparisions
    if (typeName == "PassiveWindow") return false;

    if (typeName == "ShaderProgram") {
        if (VRMaterial::fieldContainerMap.count(id)) {
            auto scID = VRMaterial::fieldContainerMap[id];
            return isSubContainer(scID);
        }
        cout << "  -- WARNING -- untracked ShaderProgram " << id << endl;
    }

    if (typeName == "ShaderVariableOSG" || typeName == "ShaderVariableInt" || typeName == "ShaderVariableReal") {
        if (VRMaterial::fieldContainerMap.count(id)) {
            auto scID = VRMaterial::fieldContainerMap[id];
            return isSubContainer(scID);
        }
        cout << "  -- WARNING -- untracked ShaderVariable " << id << endl;
    }

    if (typeName == "Image" || typeName == "SolidBackground" || typeName == "FrameBufferObject") { // TODO, implement propper check
        //cout << " -- WARNING -- unhandled FC type in isSubContainer: " << id << " " << typeName << endl;
        return false;
    }


    cout << " -- WARNING -- unhandled FC type in isSubContainer: " << id << " " << typeName << endl;

    return false;
}

// checks if a container was changed by remote
bool VRSyncNode::isRemoteChange(const UInt32& id) {
    return bool(::find(syncedContainer.begin(), syncedContainer.end(), id) != syncedContainer.end());
}

void VRSyncNode::addRemoteMapping(UInt32 lID, UInt32 rID) {
    remoteToLocalID[rID] = lID;
    localToRemoteID[lID] = rID;

    if (false) { // for debugging
        cout << " addRemoteMapping in " << getName() << ", map local " << lID << " to remote " << rID;
        if (auto afc = dynamic_cast<AttachmentContainer*>(factory->getContainer(lID))) {
            if (auto n = ::getName(afc)) cout << ", local named " << n;
        }
        if (auto afc = dynamic_cast<AttachmentContainer*>(factory->getContainer(rID))) {
            if (auto n = ::getName(afc)) cout << ", remote named " << n;
        }
        cout << endl;
    }
}

void VRSyncNode::replaceContainerMapping(UInt32 ID1, UInt32 ID2) {
    for (auto c : container) {
        if (c.second == ID1) addRemoteMapping(c.first, ID2);
    }
}

UInt32 VRSyncNode::getRegisteredContainerID(UInt32 syncID) {
    for (auto registered : container) {
        if (registered.second == syncID) return registered.first;
    }
    return -1;
}

void printNodeFieldMask(BitVector fieldmask) {
    string changeType = "";
    if (fieldmask & Node::AttachmentsFieldMask) changeType = " AttachmentsFieldMask";
//    else if (fieldmask & Node::bInvLocalFieldMask) changeType = " bInvLocalFieldMask ";
//    else if ( fieldmask & Node::bLocalFieldMask) changeType = " bLocalFieldMask      ";
    else if ( fieldmask & Node::ChangedCallbacksFieldMask) changeType = " ChangedCallbacksFieldMask";
    else if ( fieldmask & Node::ChildrenFieldMask) changeType = " ChildrenFieldMask   ";
    else if ( fieldmask & Node::ContainerIdMask) changeType = " ContainerIdMask ";
    else if ( fieldmask & Node::DeadContainerMask) changeType = " DeadContainerMask ";
    else if ( fieldmask & Node::NextFieldMask) changeType = " NextFieldMask ";
    else if ( fieldmask & Node::ParentFieldMask) changeType = " ParentFieldMask ";
    else if ( fieldmask & Node::SpinLockClearMask) changeType = " SpinLockClearMask ";
    else if ( fieldmask & Node::TravMaskFieldMask) changeType = " TravMaskFieldMask ";
    else if ( fieldmask & Node::VolumeFieldMask) changeType = " VolumeFieldMask ";
    else if ( fieldmask & Node::CoreFieldMask) changeType = " CoreFieldMask ";
    else changeType = " none ";

    cout << changeType << endl;
}

void printContainer (FieldContainerFactoryBase* factory, map<UInt32,UInt32> container) {
    for (auto c : container) {
        UInt32 id = c.first;
        FieldContainer* fcPt = factory->getContainer(id);
        if (!fcPt) continue;
        FieldContainerType* fcType = factory->findType(fcPt->getTypeId());
        if (fcType) {
            if (fcType->isNode()) {
                Node* node = dynamic_cast<Node*>(fcPt);
                cout << id << " " << fcPt->getTypeName() << " children " << node->getNChildren() << endl;
                if (node->getNChildren() == 0) cout << "no children" << endl;
                for (UInt32 i = 0; i < node->getNChildren(); i++) {
                    Node* childNode = node->getChild(i);
                    cout << "   " << childNode->getId() << endl;
                }
            }
        }
    }
}


void VRSyncNode::gatherLeafs(VRObjectPtr parent, vector<pair<Node*, VRObjectPtr>>& leafs, vector<VRObjectPtr>& inconsistentCores) {
    if (!parent) return;
    if (!parent->getNode()) return;
    if (!parent->getCore()) return;

    vector<Node*> vrChildren;
    for (auto child : parent->getChildren()) vrChildren.push_back( child->getNode()->node );

    vector<Node*> vrChildrenTMP;
    Node* pNode = parent->getNode()->node;

    // check if a transform core changed
    NodeCore* c1 = parent->getCore()->core;
    NodeCore* c2 = pNode->getCore();
    if (c1 != c2 && c2) {
        string type = c2->getTypeName();
        if (type != "Group") inconsistentCores.push_back(parent);
    }

    // ignore geometry nodes, they are part of vrgeometry
    if (pNode->getNChildren() == 1) {
        if (dynamic_cast<Geometry*>(pNode->getChild(0)->getCore())) { // check if geo core
            pNode = pNode->getChild(0);
        }
    }

    // check if transform core and obj are mapped
    if (c2 && !nodeToVRObject.count(c2->getId())) nodeToVRObject[c2->getId()] = parent;

    for (UInt32 i=0; i<pNode->getNChildren(); i++) {
        Node* child = pNode->getChild(i);
        vrChildrenTMP.push_back(child);
        if (i < vrChildren.size() && vrChildren[i] == child) continue; //try to check with index
        if (::find(vrChildren.begin(), vrChildren.end(), child) != vrChildren.end()) continue;
        leafs.push_back( make_pair(child, parent) );
    }

    for (auto child : parent->getChildren()) gatherLeafs(child, leafs, inconsistentCores);
}

VRObjectPtr VRSyncNode::OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, Node* geoParent) {
    if (!doWrapping) return 0;
    if (n == 0) return 0; // TODO add an osg wrap method for each object?

    VRObjectPtr tmp = 0;
    VRMaterialPtr tmp_m;
    VRGeometryPtr tmp_g;
    VRTransformPtr tmp_e;
    VRGroupPtr tmp_gr;

    if (!n) { cout << "WARNING! VRSyncNode::OSGConstruct: node invalid!" << endl; return 0; }
    if (!n->getCore()) { cout << "WARNING! VRSyncNode::OSGConstruct: node core invalid!" << endl; return 0; }

    NodeCoreMTRecPtr core = n->getCore();
    string t_name = core->getTypeName();
    string name = ::getName(n);

    if (t_name == "Group" || t_name == "Transform") {
        if (n->getChild(0)) {
            if (n->getChild(0)->getCore()) {
                string tp = n->getChild(0)->getCore()->getTypeName();
                if (tp == "Geometry") {
                    tmp_g = VRGeometry::create(name);
                    tmp_g->wrapOSG(OSGObject::create(n), OSGObject::create(n->getChild(0)));
                    tmp = tmp_g;
                    nodeToVRObject[n->getCore()->getId()] = tmp;
                }
            }
        }
    }

    if (tmp == 0 && t_name == "Group") {
        tmp = VRObject::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        //nodeToVRObject[n->getCore()->getId()] = tmp;
    }

    if (tmp == 0 && t_name == "Transform") {
        tmp_e = VRTransform::create(name);
        tmp_e->wrapOSG(OSGObject::create(n));
        tmp = tmp_e;
        nodeToVRObject[n->getCore()->getId()] = tmp;
    }

    if (t_name == "Geometry") return 0; // ignore geo leafs

    if (tmp == 0) { // fallback
        tmp = VRObject::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        //nodeToVRObject[n->getId()] = tmp;
    }

    for (uint i=0;i<n->getNChildren();i++) { // recursion
        VRObjectPtr obj;
        if (tmp) {
            obj = OSGConstruct(n->getChild(i), tmp, geoParent);
            if (obj) tmp->addChild(obj, false);
        }
    }

    return tmp;
}

void VRSyncNode::wrapOSGLeaf(Node* node, VRObjectPtr parent) {
    auto res = OSGConstruct(node, parent);
    if (res) parent->addChild(res, false);
}

void VRSyncNode::wrapOSG() { // TODO: check for deleted nodes!
    // traverse sub tree and get unwrapped OSG nodes
    vector<pair<Node*, VRObjectPtr>> leafs; // pair of nodes and VRObjects they are linked to
    vector<VRObjectPtr> inconsistentCores;
    gatherLeafs(ptr(), leafs, inconsistentCores);

    // TODO: wrapping the nodes breaks DnD sync
    for (auto p : leafs) wrapOSGLeaf(p.first, p.second);
    for (auto i : inconsistentCores) i->wrapOSG(i->getNode());
}

UInt32 VRSyncNode::findParent(map<UInt32,vector<UInt32>>& parentToChildren, UInt32 remoteNodeID) {
    UInt32 parentId = container.begin()->first;
    for (auto remoteParent : parentToChildren) {
        UInt32 remoteParentId = remoteParent.first;
        vector<UInt32> remoteChildren = remoteParent.second;
        for (UInt32 i = 0; i<remoteChildren.size(); i++) {
            UInt32 remoteChildId = remoteChildren[i];
            if (remoteChildId == remoteNodeID) {
                if (remoteToLocalID[remoteParentId]) return remoteToLocalID[remoteParentId];
            }
        }
    }
    return parentId;
}

void VRSyncNode::printRegistredContainers() {
    cout << endl << "registered container: " << name << endl;
    for (auto c : container) {
        UInt32 id = c.first;
        FieldContainer* fc = factory->getContainer(id);
        cout << " " << id << ", syncNodeID " << c.second;

        if (localToRemoteID.count(id))  cout << ", remoteID " << localToRemoteID[id];
        else                            cout << ",              ";

        if (fc) {
            cout << ", type: " << fc->getTypeName() << ", Refs: " << fc->getRefCount();
            if (Node* node = dynamic_cast<Node*>(fc)) cout << ", N children: " << node->getNChildren();
        }
        cout << endl;
    }
}

void VRSyncNode::printSyncedContainers() {
    cout << endl << "synced container:" << endl;
    for (UInt32 id : syncedContainer) cout << id << endl;
}

bool VRSyncNode::isRegistered(const UInt32& id) {
    return bool(container.count(id));
}

bool VRSyncNode::isExternalContainer(const UInt32& id, UInt32& mask) {
    if (externalContainer.count(id)) {
        mask = externalContainer[id];
        return true;
    }
    return false;
}

//checks in container if the node with syncID is already been registered
bool VRSyncNode::isRegisteredRemote(const UInt32& syncID) {
    for (auto reg : container) { //check if the FC is already registered, f.e. if nodeCore create entry arrives first a core along with its node will be created before the node create entry arrives
        if (reg.second == syncID) return true;
    }
    return false;
}

void VRSyncNode::getAllSubContainersRec(FieldContainer* node, FieldContainer* parent, map<FieldContainer*, vector<FieldContainer*>>& res) {
    if (!node) return;
    if (!isRegistered(node->getId())) {
        res[node].push_back(parent);
    }

    auto ptype = factory->findType(node->getTypeId());

    if (ptype->isNode()) {
        Node* pnode = dynamic_cast<Node*>(node);

        NodeCore* core = pnode->getCore();
        getAllSubContainersRec(core, node, res);

        auto attachments = pnode->getSFAttachments()->getValue();
        for (auto a : attachments) {
            Attachment* attachment = a.second;
            getAllSubContainersRec(attachment, node, res);
        }

        for (UInt32 i=0; i<pnode->getNChildren(); i++) {
            Node* child = pnode->getChild(i);
            getAllSubContainersRec(child, node, res);
        }
    }

    if (ptype->isNodeCore()) {
        NodeCore* pcore = dynamic_cast<NodeCore*>(node);
        auto attachments = pcore->getSFAttachments()->getValue();
        for (auto a : attachments) {
            Attachment* attachment = a.second;
            getAllSubContainersRec(attachment, node, res);
        }
    }

    if (ptype->isAttachment()) {
        Attachment* pattachment = dynamic_cast<Attachment*>(node);
        auto attachments = pattachment->getSFAttachments()->getValue();
        for (auto a : attachments) {
            Attachment* attachment = a.second;
            getAllSubContainersRec(attachment, node, res);
        }
    }
}

map<FieldContainer*, vector<FieldContainer*>> VRSyncNode::getAllSubContainers(FieldContainer* node) {
    map<FieldContainer*, vector<FieldContainer*>> res;
    getAllSubContainersRec(node, 0, res);
    return res;
}

bool poseChanged(Pose oldPose, PosePtr newPose, int thresholdPos, int thresholdAngle){
    Vec3d oldPos = oldPose.pos();
    Vec3d oldDir = oldPose.dir();
    Vec3d newPos = newPose->pos();
    Vec3d newDir = newPose->dir();
    //cout << "oldPos, oldDir " << oldPos << ", " << oldDir << " newPos,newDir" << newPos << ", " << newDir << endl;

    Vec3d distancePos = oldPos - newPos; //calculate distances

    float magOldDir = std::sqrt(oldDir.x()*oldDir.x() + oldDir.y()*oldDir.y() + oldDir.z()*oldDir.z()); //calculate dir angle
    float magNewDir = std::sqrt(newDir.x()*newDir.x() + newDir.y()*newDir.y() + newDir.z()*newDir.z());

    oldDir.normalize();
    newDir.normalize();
    float dotProduct = oldPos.x()*newPos.x() + oldPos.y()*newPos.y() + oldPos.z()*newPos.z();
    float cosAngle = dotProduct / (magOldDir*magNewDir);
    auto angle = std::acos(cosAngle);
    //cout << "poseChanged      Angle: " << angle << " cosAngle " << cosAngle << " dotProduct " << dotProduct << " magNewDir " << magNewDir << " magOldDir " << magOldDir << endl;

    if (abs(distancePos.x()) > thresholdPos || abs(distancePos.y()) > thresholdPos || abs(distancePos.z()) > thresholdPos) return true; // check if differences exceed thresholds
    else if (abs(angle) > thresholdAngle) return true;
    else return false;
}

void VRSyncNode::getAndBroadcastPoses() {
    if (!doAvatars) return;
    string poses = "poses|name$" + getConnectionLink();

    VRScenePtr scene = VRScene::getCurrent(); //get scene
    VRCameraPtr cam = scene->getActiveCamera(); //get camera pose
    PosePtr camPose = cam->getWorldPose();
    VRDevicePtr mouse = VRSetup::getCurrent()->getDevice("mouse"); //check devices and eventually get poses
    PosePtr mousePose = mouse->getBeacon()->getPose();
    VRDevicePtr flyStick = VRSetup::getCurrent()->getDevice("flystick"); //check devices and eventually get poses


    bool camChanged = poseChanged(oldCamPose, camPose, 0.1, 10);
    bool mouseChanged = poseChanged(oldMousePose, mousePose, 0.1, 10);

    //if (!camChanged && !mouseChanged) return;

    if (camChanged) {
        string pose_str = toString(camPose); //append poses to string
        poses += "|cam$" + pose_str;
        oldCamPose = *camPose; //update oldPoses
    }
    if (mousePose && mouseChanged) {
        poses += "|mouse$" + toString(mousePose);
        oldMousePose = *mousePose;
    }

    if (flyStick) {
        PosePtr flyStickPose = flyStick->getBeacon()->getPose();
        bool flyStickChanged = poseChanged(oldFlystickPose, flyStickPose, 0.1, 10);
        if (flyStickChanged) {
            poses += "|flystick$" + toString(flyStickPose);
            oldFlystickPose = *flyStickPose;
        }
    }

    broadcast(poses); //broadcast
    //cout << "broadcast poses " << poses << endl;
}

/*void VRSyncNode::sync(string uri) {
    if (!container.size()) return;
    vector<BYTE> data;
    vector<UInt32> containerData;
    for (auto c : container) {
        containerData.push_back(c.first);
        containerData.push_back(c.second);
    }
    data.insert(data.end(), (BYTE*)&containerData[0], (BYTE*)&containerData[0] + sizeof(UInt32)*containerData.size());
    string msg = VRSyncConnection::base64_encode(&data[0], data.size());
    remotes[uri]->send("sync");
    remotes[uri]->send(msg);
}*/

//update this SyncNode
void VRSyncNode::update() {
    handledPoses = false;
    getAndBroadcastPoses();
    auto localChanges = changelist->filterChanges(ptr());
    if (!localChanges) return;
    if (getChildrenCount() == 0) return; // TODO: this may happen if the only child is dragged..
    cout << endl << " > > >  " << name << " VRSyncNode::update()" << endl;


    //OSGChangeList* cl = (OSGChangeList*)applicationThread->getChangeList();
    //changelist->printChangeList(ptr(), cl);


    //printRegistredContainers(); // DEBUG: print registered container
    printSyncedContainers();
    //changelist->printChangeList(ptr(), localChanges);

    changelist->broadcastChangeList(ptr(), localChanges, true);
    syncedContainer.clear();
    cout << "            / " << name << " VRSyncNode::update()" << "  < < < " << endl;
}

void VRSyncNode::logSyncedContainer(UInt32 id) {
    if (isRemoteChange(id)) syncedContainer.push_back(id); // TODO: irgendwie komisch.. wird syncedContainer ueberhaupt verwendet?
}

void VRSyncNode::registerContainer(FieldContainer* c, UInt32 syncNodeID) {
    UInt32 ID = c->getId();
    if (container.count(ID)) return;
    //cout << " VRSyncNode::registerContainer " << getName() << " container: " << c->getTypeName() << " at fieldContainerId: " << ID << endl;
    container[ID] = syncNodeID;
}

void VRSyncNode::setAvatarBeacons(VRTransformPtr head, VRTransformPtr device) {
    avatarHeadBeacon = head;
    avatarDeviceBeacon = device;
}

void VRSyncNode::addRemoteAvatar(VRTransformPtr head, VRTransformPtr device) {
    UInt32 headID = head->getNode()->node->getId();
    UInt32 deviceID = device->getNode()->node->getId();
    broadcast("addAvatar|"+toString(headID)+":"+toString(deviceID));
}

void VRSyncNode::handleAvatar(string data) {
    auto IDs = splitString( splitString(data, '|')[1], ':');
    UInt32 avatarBeaconID = toInt(IDs[1]);
    UInt32 mouseBeaconID = avatarDeviceBeacon->getNode()->node->getId();

    addRemoteMapping(mouseBeaconID, avatarBeaconID); // local, remote
    broadcast("mapping|"+toString(avatarBeaconID)+":"+toString(mouseBeaconID));

    UInt32 mask = 0;
    mask |= Node::ChildrenFieldMask;
    externalContainer[mouseBeaconID] = mask;

    //cout << " ---> mapAvatar, " << mouseBeaconID << ": " << mouseBeacon->getName() << ", " << avatarBeaconID << ": " << avatarBeacon->getName() << endl;
}

size_t VRSyncNode::getContainerCount() { return container.size(); }

//returns registered IDs
vector<UInt32> VRSyncNode::registerNode(Node* node) { // deprecated?
    vector<UInt32> res;
    vector<UInt32> localRes;
    vector<UInt32> recursiveRes;
    NodeCoreMTRefPtr core = node->getCore();
    cout << "register node " << node->getId() << endl;

    registerContainer(node, container.size());
    if (!core) cout << "no core" << core << endl;
    cout << "register core " << core->getId() << endl;

    registerContainer(core, container.size());
    localRes.push_back(node->getId());
    localRes.push_back(core->getId());
    for (UInt32 i=0; i<node->getNChildren(); i++) {
        cout << "register child " << node->getChild(i)->getId() << endl;
        recursiveRes = registerNode(node->getChild(i));
    }
    res.reserve(localRes.size() + recursiveRes.size());
    res.insert(res.end(), localRes.begin(), localRes.end());
    res.insert(res.end(), recursiveRes.begin(), recursiveRes.end());
    return res;
}

VRObjectPtr VRSyncNode::copy(vector<VRObjectPtr> children) {
    return 0;
}

UInt32 VRSyncNode::getLocalType(UInt32 id) {
    return typeMapping[id];
}

void VRSyncNode::handleMapping(string mappingData) {
    auto pairs = splitString(mappingData, '|');
    for (auto p : pairs) {
        auto IDs = splitString(p, ':');
        if (IDs.size() != 2) continue;
        UInt32 lID = toInt(IDs[0]);
        UInt32 rID = toInt(IDs[1]);
        addRemoteMapping(lID, rID);
    }
    //printRegistredContainers();
}

void VRSyncNode::sendTypes() {
    auto dtIt = factory->begin(FieldContainer::getClassType());
    auto dtEnd = factory->end();

    string msg = "typeMapping";
    while (dtIt != dtEnd) {
        FieldContainerType* fcT = *dtIt;
        msg += "|" + toString(fcT->getId()) + ":" + fcT->getName();
        ++dtIt;
    }
    broadcast(msg);
}

void VRSyncNode::handleTypeMapping(string mappingData) {
    auto pairs = splitString(mappingData, '|');
    for (auto p : pairs) {
        auto IDs = splitString(p, ':');
        if (IDs.size() != 2) continue;
        UInt32 rID = toInt(IDs[0]);
        string name = IDs[1];
        FieldContainerType* fcT = factory->findType(name.c_str());
        if (!fcT) {
            cout << "Warning in VRSyncNode::handleTypeMapping, unknown remote type " << name.c_str() << endl;
            continue;
        }
        typeMapping[rID] = fcT->getId();
        //cout << " typeMapping: " << rID << " " << name << " " << fcT->getId() << endl;
    }
    //printRegistredContainers();
}

void VRSyncNode::handlePoses(string poses)  {
    //cout << "VRSyncNode::handlePoses: " << poses << endl;
    if (handledPoses) return;
    handledPoses = true;
    string nodeName;
    vector<string> pairs = splitString(poses, '|');
    vector<string> namePair = splitString(pairs[1], '$');
    if (namePair[0] == "name") nodeName = namePair[1];
    if (nodeName == "") return;
    for (unsigned int i = 2; i < pairs.size(); i++) {
        auto data = splitString(pairs[i], '$');
        if (data.size() != 2) continue;
        string deviceName = data[0];
        PosePtr pose = toValue<PosePtr>(data[1]);
        if (deviceName == "cam") remotesCameraPose[nodeName] = pose;
        else if (deviceName == "mouse") remotesMousePose[nodeName] = pose;
        else if (deviceName == "flystick") remotesFlystickPose[nodeName] = pose;
        //cout <<  "VRSyncNode::handlePoses      deviceName " << deviceName << " pose " << pose << " remotesCameraPose[nodeName] " << remotesCameraPose[nodeName] << " nodeName " << nodeName << endl;
    }
    //TODO: do something with poses

}

void VRSyncNode::handleOwnershipMessage(string ownership)  {
    cout << "VRSyncNode::handleOwnership" << endl;
    vector<string> str_vec = splitString(ownership, '|');
    string nodeName = str_vec[2];
    string objectName = str_vec[3];
    if (str_vec[1] == "request") {
        cout << "handle request" << endl;
        for (string s : owned) {

            cout << "check owned" << endl;
            if (s == objectName) {
                cout << "found owned, try to get object..." << endl;
                auto object = VRScene::getCurrent()->getRoot()->find(objectName);
                if (!object) {cout<< "object " << objectName << " not fould!" << endl; return;}
                for (auto dev : VRSetup::getCurrent()->getDevices()) { //if object is grabbed return
                    VRTransformPtr obj = dev.second->getDraggedObject();
                    VRTransformPtr gobj = dev.second->getDraggedGhost();
                    if (obj == 0 || gobj == 0) {cout << "continue" << endl; continue;}
                    if (obj->getName() == objectName || gobj->getName() == objectName) {
                        cout << "object is not available. ownership not granted." << endl;
                        return;
                    }
                }
                for (auto remote : remotes) { //grant ownership
                    cout << "remote.first " << remote.first << endl;
                    if (remote.first.find(nodeName) != string::npos) {
                        string message = "ownership|grant|" + nodeName + "|" + objectName;
                        remote.second->send(message);
                        auto it = std::find(owned.begin(), owned.end(), objectName);
                        if (it != owned.end()) owned.erase(it);
                        cout << "grant ownership " << message << endl;
                    }
                }
            }
        }
    }
    else if (str_vec[1] == "grant") {
        if (nodeName == getConnectionLink()) owned.push_back(objectName);
        cout << "got ownership of object " << objectName << endl;
    }
}

vector<string> VRSyncNode::getOwnedObjects(string nodeName) {
    return owned;
}

void VRSyncNode::requestOwnership(string objectName){
    string message = "ownership|request|" + getConnectionLink() + "|" + objectName;
    broadcast(message);
}

void VRSyncNode::addOwnedObject(string objectName){
    owned.push_back(objectName);
}

PosePtr VRSyncNode::getRemoteCamPose(string remoteName) {
    if (!remotesCameraPose.count(remoteName)) { /*cout << "Error in VRSyncNode::getRemoteCamPose: " << remoteName << " not in camera poses!" << endl;*/ return 0; }
    return remotesCameraPose[remoteName];
}

PosePtr VRSyncNode::getRemoteMousePose(string remoteName) {
    if (!remotesMousePose.count(remoteName)) { /*cout << "Error in VRSyncNode::getRemoteMousePose: " << remoteName << " not in mouse poses!" << endl;*/ return 0; }
    return remotesMousePose[remoteName];
}
PosePtr VRSyncNode::getRemoteFlystickPose(string remoteName) {
    if (!remotesFlystickPose.count(remoteName)) { /*cout << "Error in VRSyncNode::getRemoteFlystickPose: " << remoteName << " not in flystick poses!" << endl;*/ return 0; }
    return remotesFlystickPose[remoteName];
}

vector<string> VRSyncNode::getRemotes() {
    vector<string> res;
    for (auto r : remotes) res.push_back(r.first);
    return res;
}

//Add remote Nodes to sync with
void VRSyncNode::addRemote(string host, int port) {
    cout << " >>> > > VRSyncNode::addRemote to " << getName() << ": " << name << " at " << host << " on " << port << endl;
    string uri = host + ":" + toString(port);
    if (remotes.count(uri)) return;
    remotes[uri] = VRSyncConnection::create(host, port);

    // sync node ID
    auto nID = getNode()->node->getId();
    remotes[uri]->send("selfmap|"+toString(nID));
    remotes[uri]->send("newConnect|"+getConnectionLink());
    cout << "   send newConnect from " << uri << endl;

    sendTypes();
}

void VRSyncNode::handleNewConnect(string data){
    cout << "VRSyncNode::handleNewConnect" << endl;
    auto remoteData = splitString(data, '|');
    string remoteName = remoteData[1];
    auto uri = splitString(remoteName, ':');
    string ip = uri[0];
    string port = uri[1];

    cout << " handleNewConnect with ip " << remoteName << endl;
    (*onEvent)("connection|"+remoteName); //if not in list then it is a new connection, the add remote

    if (!remotes.count(remoteName)) {
        cout << "  new connection -> add remote" << remoteName << endl;
        VRSyncNode::addRemote(ip, toInt(port));
    }
}

void VRSyncNode::startInterface(int port) {
    server = VRTCPServer::create();
    server->listen(port);
    server->onMessage( bind(&VRSyncNode::handleMessage, this, std::placeholders::_1) );
}

void VRSyncNode::handleWarning(string msg) {
    //cout << "Incomming Warning! " << msg << endl;
    auto data = splitString(msg, '|');
    if (data.size() != 3) { cout << "AAargh" << endl; return; }
    int ID = toInt(data[2]);
    cout << " --> Warning from other SyncNode: " << data[1] << " with fc ID: " << ID;
    if (factory) {
        if (auto fct = factory->getContainer(ID)) {
            cout << " fc type: " << fct->getTypeName() << " (" << fct->getTypeId() << ")";
            if (AttachmentContainer* attc = dynamic_cast<AttachmentContainer*>(fct)) {
                if (auto n = ::getName(attc)) cout << " named: " << n;
                else cout << " unnamed";
            }
        }
    }
    cout << " owned by " << getName() << endl;
}

void VRSyncNode::handleSelfmapRequest(string msg) {
    auto data = splitString(msg, '|');
    int rID = toInt(data[1]);
    addRemoteMapping(selfID, rID);
}

/*void VRSyncNode::handleMessage(void* _args) {
    HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) cout << "AAAARGH" << endl;

    //UInt32 client = args->ws_id;
    string msg = args->ws_data;*/
void VRSyncNode::handleMessage(string msg) {
    //cout << "VRSyncNode::handleMessage " << msg.size() << endl;
    //cout << msg << endl;
    VRUpdateCbPtr job = 0;
    if (startsWith(msg, "message|"));
    else if (startsWith(msg, "addAvatar|")) job = VRUpdateCb::create( "sync-handleAvatar", bind(&VRSyncNode::handleAvatar, this, msg) );
    else if (startsWith(msg, "selfmap|")) handleSelfmapRequest(msg);
    else if (startsWith(msg, "mapping|")) job = VRUpdateCb::create( "sync-handleMap", bind(&VRSyncNode::handleMapping, this, msg) );
    else if (startsWith(msg, "typeMapping|")) job = VRUpdateCb::create("sync-handleTMap", bind(&VRSyncNode::handleTypeMapping, this, msg));
    else if (startsWith(msg, "poses|"))   job = VRUpdateCb::create( "sync-handlePoses", bind(&VRSyncNode::handlePoses, this, msg) );
    else if (startsWith(msg, "ownership|")) job = VRUpdateCb::create( "sync-ownership", bind(&VRSyncNode::handleOwnershipMessage, this, msg) );
    else if (startsWith(msg, "newConnect|")) job = VRUpdateCb::create( "sync-newConnect", bind(&VRSyncNode::handleNewConnect, this, msg) );
    else if (startsWith(msg, "changelistEnd|")) job = VRUpdateCb::create( "sync-finalizeCL", bind(&VRSyncChangelist::deserializeAndApply, changelist.get(), ptr()) );
    else if (startsWith(msg, "warn|")) job = VRUpdateCb::create( "sync-handleWarning", bind(&VRSyncNode::handleWarning, this, msg) );
    //else if (startsWith(msg, "warn|")) handleWarning(msg);
    else job = VRUpdateCb::create( "sync-handleCL", bind(&VRSyncChangelist::gatherChangelistData, changelist.get(), ptr(), msg) );
    if (job) VRScene::getCurrent()->queueJob( job );
}

//broadcast message to all remote nodes
void VRSyncNode::broadcast(string message) {
    for (auto& remote : remotes) {
        if (!remote.second->send(message)) {
            cout << "Failed to send message to remote." << endl;
        }
    }
}

UInt32 VRSyncNode::getRemoteToLocalID(UInt32 id) {
    if (!remoteToLocalID.count(id)) return 0;
    return remoteToLocalID[id];
}

UInt32 VRSyncNode::getLocalToRemoteID(UInt32 id) {
    if (!localToRemoteID.count(id)) return 0;
    return localToRemoteID[id];
}

UInt32 VRSyncNode::getContainerMappedID(UInt32 id) {
    if (!container.count(id)) return -1;
    return container[id];
}

VRObjectPtr VRSyncNode::getVRObject(UInt32 id) {
    if (!nodeToVRObject.count(id)) return 0;
    return nodeToVRObject[id].lock();
}

void printNode(VRObjectPtr obj, string indent = "") {
    cout << indent << "obj " << obj->getName() << " (" << obj->getType() << ")";
    cout << ", nodeID: " << obj->getNode()->node->getId();
    cout << ", nde coreID: " << obj->getNode()->node->getCore()->getId() << ", nde coreType: " << obj->getNode()->node->getCore()->getTypeName();
    cout << ", obj coreID: " << obj->getCore()->core->getId() << ", obj coreType: " << obj->getCore()->core->getTypeName();
    if (auto geo = dynamic_pointer_cast<VRGeometry>(obj)) cout << ", geoNodeID: " << geo->getNode()->node->getChild(0);
    cout << endl;

    for (auto c : obj->getChildren()) printNode(c, indent+" ");
}

void VRSyncNode::analyseSubGraph() {
    cout << "VRSyncRemote::analyseSubGraph" << endl;
    printNode(ptr());
    printRegistredContainers();
}

string VRSyncNode::getConnectionLink() {
    /*ifconf ic;
    ioctl(s, SIOCGIFCONF, ic);
    IP = string(ic.ifc_buf);*/
    string IP = server->getPublicIP();
    int port = server->getPort();
    return IP+":"+toString(port);
}

void VRSyncNode::setCallback(VRMessageCbPtr fkt){
    onEvent = fkt;
    cout << "VRSyncNode::setCallback" << endl;
}

string VRSyncNode::getConnectionStatus(){
    string status = "SyncNode: " + getConnectionLink() + "\n";
    for (auto remote : remotes) {
        status += " (" + remote.first + ") " + remote.second->getStatus();
        status += "\n";
    }
    return status;
}

