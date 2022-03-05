#ifndef VRCOLLABORATION_H_INCLUDED
#define VRCOLLABORATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "VRNetworkingFwd.h"
#include "core/scene/sound/VRSoundFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCollaboration : public VRObject {
	private:
	    VRICEClientPtr ice;
	    VRSyncNodePtr syncNode;
	    VRMicrophonePtr mike;
	    VRSoundPtr sound;

        VRTransformPtr avatarTorso;
        VRTransformPtr avatarHandLeft;
        VRTransformPtr avatarHandRight;

	    VRSpritePtr connectionInWidget;

	    string connReqOrigin;

	    void init();
	    void connectTCP(string origin);
	    void setupAvatar(string rID, string name);
	    void onIceEvent(string m);

	    void initUI();
	    void sendUI(string widget, string data);
	    void updateUsersWidget();

	public:
		VRCollaboration();
		~VRCollaboration();

		static VRCollaborationPtr create();
		VRCollaborationPtr ptr();

		void setServer(string uri);
		void setAvatarDevices(VRTransformPtr head, VRTransformPtr hand, VRTransformPtr handGrab = 0);
		void setAvatarGeometry(VRTransformPtr torso, VRTransformPtr leftHand = 0, VRTransformPtr rightHand = 0);
};

OSG_END_NAMESPACE;

#endif //VRCOLLABORATION_H_INCLUDED
