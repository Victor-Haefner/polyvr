#include "VRCollaboration.h"
#include "core/utils/toString.h"

#include "tcp/VRICEclient.h"
#include "core/objects/sync/VRSyncNode.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/scene/sound/VRMicrophone.h"
#include "core/scene/sound/VRSound.h"
#include "core/scene/VRScene.h"


using namespace OSG;

VRCollaboration::VRCollaboration() {}
VRCollaboration::~VRCollaboration() {}

VRCollaborationPtr VRCollaboration::create() {
    auto c = VRCollaborationPtr( new VRCollaboration() );
    c->init();
    return c;
}

VRCollaborationPtr VRCollaboration::ptr() { return static_pointer_cast<VRCollaboration>(shared_from_this()); }

void VRCollaboration::init() {
    ice = VRICEClient::create();
	ice->onEvent(bind(&VRCollaboration::onIceEvent, this, placeholders::_1));

    syncNode = VRSyncNode::create("syncNode");
	addChild(syncNode);

    mike = VRMicrophone::create();
	mike->pauseStreaming(true);

    sound = VRSound::create();

    initUI();
}

void VRCollaboration::setServer(string uri) {
	ice->setTurnServer(uri);
}

void VRCollaboration::setAvatarDevices(VRTransformPtr head, VRTransformPtr hand, VRTransformPtr handGrab) {
    if (!handGrab) handGrab = hand;
	syncNode->setAvatarBeacons(head, hand, handGrab);
}

void VRCollaboration::setAvatarGeometry(VRTransformPtr torso, VRTransformPtr leftHand, VRTransformPtr rightHand) {
    avatarTorso = torso;
    avatarHandLeft = leftHand;
    avatarHandRight = rightHand;
}

void VRCollaboration::setupAvatar(string rID, string name) {
	auto avatar = VRTransform::create("avatar");
	addChild(avatar);

	auto rightHandContainer = VRTransform::create("rightHandContainer");
	auto anchor = VRTransform::create("anchor");
	rightHandContainer->addChild(anchor);
	avatar->addChild(rightHandContainer);

	if (avatarTorso) {
        auto torso = dynamic_pointer_cast<VRTransform>(avatarTorso->duplicate());
        torso->setFrom(Vec3d(0,5,-0.8));
        avatar->addChild(torso);
	}

    if (avatarHandLeft) {
        auto leftHand = dynamic_pointer_cast<VRTransform>(avatarHandLeft->duplicate());
        leftHand->rotate(1.5707/90*40, Vec3d(1,0,0)) ; // rotate 40° around z-Axis
        leftHand->rotate(1.5707*2, Vec3d(0,1,0));
        leftHand->translate(Vec3d(-2.4,0,0));
        avatar->addChild(leftHand);
    }

    if (avatarHandRight) {
        auto rightHand = dynamic_pointer_cast<VRTransform>(avatarHandRight->duplicate());
        rightHand->rotate(1.5707*2, Vec3d(0,0,1));
        rightHand->translate(Vec3d(2.4,0,0));
        rightHandContainer->addChild(rightHand);
    }

    auto sprite = VRSprite::create("nameTag");
	sprite->setSize(5,1.5);
	sprite->setText(name);
	sprite->setFrom(Vec3d(0,9,0));
	sprite->setBillboard(true);
	avatar->addChild(sprite);

	auto job = [&] {
        syncNode->addRemoteAvatar(rID, avatar, rightHandContainer, anchor);
	};

	VRUpdateCbPtr cb = VRUpdateCb::create("syncNode-addRemoteAvatar", job);
	VRScene::getCurrent()->queueJob(cb);
	//VR->stackCall(VR->syncNode->addRemoteAvatar, 3, [rID, avatar, rightHandContainer, anchor]);
	//syncNode->addRemoteAvatar(rID, avatar, rightHandContainer, anchor);
}

void VRCollaboration::connectTCP(string origin) {
	auto rID = syncNode->addTCPClient(ice->getClient(origin, VRICEClient::SCENEGRAPH));
	auto name = ice->getUserName(origin);
	setupAvatar(rID, name);

	auto audioClient = ice->getClient(origin, VRICEClient::AUDIO);
	mike->startStreamingOver(audioClient);
	mike->pauseStreaming(false);
	sound->playPeerStream(audioClient);
}

void VRCollaboration::onIceEvent(string m) {
	if (m == "users changed") updateUsersWidget();

	if ( startsWith(m, "message|") ) {
        auto data = splitString(m, '|');
		if (data.size() < 2) return;
		auto message = data[1];
        data = splitString(message, '\n');
		if (data.size() < 4) return;
		string origin = data[2];
		string content = data[3];

		if (startsWith(content, "CONREQ") ) {
			connectionInWidget->show();
			connReqOrigin = origin;
		}

		if (startsWith(content, "CONACC") ) {
			sendUI("usersList", "setUserStats|"+origin+"|#2c4");
			ice->connectTo(origin);
			connectTCP(origin);
        }
    }
}

void VRCollaboration::sendUI(string widget, string data) { // TODO
    ;
}

void VRCollaboration::updateUsersWidget() { // TODO
    ;
}

void VRCollaboration::initUI() { // TODO
    connectionInWidget = VRSprite::create("connectionInWidget");
}
