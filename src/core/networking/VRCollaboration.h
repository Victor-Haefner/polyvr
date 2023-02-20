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
	    map<string, VRSoundPtr> voices;

        VRTransformPtr avatarTorso;
        VRTransformPtr avatarHandLeft;
        VRTransformPtr avatarHandRight;
        float avatarScale = 1.0;

	    VRSpritePtr userNameWidget;
	    VRSpritePtr userlist;
	    VRSpritePtr connectionInWidget;
	    static string uiCSS;
	    static string userNameSite;
	    static string userlistSite;
	    static string connectionInSite;
	    VRDeviceCbPtr ui_dev_cb;
	    map<string, int> gui_sites;

	    string userName;
	    map<string, string> connReqSystem;
	    map<string, vector<string> > connReqNet;

	    void init();
	    string getSubnet();
	    vector<string> parseSubNet(string net);
	    void connectTCP(string origin, bool isWindows);
	    void acceptConnection(string origin, bool isWindows);
	    void finishConnection(string origin, bool isWindows, vector<string> net);
	    void setupUserAvatar(string rID, string name);
	    void onIceEvent(string m);
	    void onSyncNodeEvent(string e);

	    void initUI();
	    void sendUI(string widget, string data);
	    bool handleUI(VRDeviceWeakPtr dev);
	    void updateUsersWidget();

	public:
		VRCollaboration(string name = "Collab");
		~VRCollaboration();

		static VRCollaborationPtr create(string name = "Collab");
		VRCollaborationPtr ptr();

        void addChild(VRObjectPtr child, bool osg = true, int place = -1) override;
        void subChild(VRObjectPtr child, bool osg = true) override;

		void setServer(string uri);
		void setupLocalServer();
		void setAvatarDevices(VRTransformPtr head, VRTransformPtr hand, VRTransformPtr handGrab = 0);
		void setAvatarGeometry(VRTransformPtr torso, VRTransformPtr leftHand = 0, VRTransformPtr rightHand = 0, float scale = 1.0);

		void debugAvatars(); // temporary for debugging purposes
};

OSG_END_NAMESPACE;

#endif //VRCOLLABORATION_H_INCLUDED
