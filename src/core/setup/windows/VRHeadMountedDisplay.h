#ifndef VRHEADMOUNTEDDISPLAY_H_INCLUDED
#define VRHEADMOUNTEDDISPLAY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <vector>
#include <map>

#include "../VRSetupFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/tools/VRToolsFwd.h"

#include "core/setup/devices/VRDevice.h"

namespace vr {
	class IVRSystem;
	struct HmdMatrix34_t;
	struct HmdMatrix44_t;
	class TrackedDevicePose_t;
}

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRHeadMountedDisplay : public VRDevice {
	public:
		struct FBOData;

	private:
		FBOData* fboData = 0;

		bool valid = false;
		vr::IVRSystem* m_pHMD;

		string m_strDriver = "No Driver";
		string m_strDisplay = "No Display";

		float m_fNearClip = 0.1f;
		float m_fFarClip = 30.0f;

		UInt32 m_nRenderWidth;
		UInt32 m_nRenderHeight;

		vector<vr::TrackedDevicePose_t> m_rTrackedDevicePose;
		vector<Matrix4d> m_rmat4DevicePose;
		map<int, VRTransformPtr> tracker;
		map<int, VRDevicePtr> devices;

		int m_iValidPoseCount = 0;
		int m_iValidPoseCount_Last = -1;

		Matrix4d m_mat4ProjectionLeft;
		Matrix4d m_mat4ProjectionRight;
		Matrix4d m_mat4eyePosLeft;
		Matrix4d m_mat4eyePosRight;
		Matrix4d calibration;

		VRDeviceCbPtr onCameraChanged;

		void addController(int devID);

		Matrix4d GetHMDMatrixProjectionEye(unsigned int nEye);
		Matrix4d GetHMDMatrixPoseEye(unsigned int nEye);
		Matrix4d convertMatrix(const vr::HmdMatrix34_t& mat);
		Matrix4d convertMatrix(const vr::HmdMatrix44_t& mat);

		void updateTexID(VRTextureRendererPtr renderer, unsigned int& tID);
		void loadActionSettings();
		void initTexRenderer();
		void initFBO();
		void setScene();
		void SetupTexturemaps();

		bool updateCamera();

	public:
		VRHeadMountedDisplay();
		~VRHeadMountedDisplay();

		static VRHeadMountedDisplayPtr create();
		VRHeadMountedDisplayPtr ptr();

		static bool checkDeviceAttached();

		void initHMD();
		void UpdateHMDMatrixPose();
		void handleInput();
		void render(bool fromThread = false);

		VRTransformPtr getTracker(int tID);
		VRDevicePtr getDevice(int dID);
		map<int, VRTransformPtr> getTrackers();
		map<int, VRDevicePtr> getDevices();

		void calibrateOrigin();
};

OSG_END_NAMESPACE;

#endif // VRHEADMOUNTEDDISPLAY_H_INCLUDED
