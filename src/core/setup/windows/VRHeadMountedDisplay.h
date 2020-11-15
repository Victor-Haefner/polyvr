#ifndef VRHEADMOUNTEDDISPLAY_H_INCLUDED
#define VRHEADMOUNTEDDISPLAY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <vector>

#include "../VRSetupFwd.h"
#include "core/tools/VRToolsFwd.h"

namespace vr {
	class IVRSystem;
	struct HmdMatrix34_t;
	struct HmdMatrix44_t;
	class TrackedDevicePose_t;
	enum EVREye;
}

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRHeadMountedDisplay : public std::enable_shared_from_this<VRHeadMountedDisplay> {
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
		vector<Matrix> m_rmat4DevicePose;
		vector<char> m_rDevClassChar;

		int m_iValidPoseCount = 0;
		int m_iValidPoseCount_Last = -1;

		Matrix m_mat4HMDPose;
		Matrix m_mat4ProjectionLeft;
		Matrix m_mat4ProjectionRight;
		Matrix m_mat4eyePosLeft;
		Matrix m_mat4eyePosRight;

		Matrix GetHMDMatrixProjectionEye(vr::EVREye nEye);
		Matrix GetHMDMatrixPoseEye(vr::EVREye nEye);
		void UpdateHMDMatrixPose();
		Matrix convertMatrix(const vr::HmdMatrix34_t& mat);
		Matrix convertMatrix(const vr::HmdMatrix44_t& mat);

		void updateTexID(VRTextureRendererPtr renderer, unsigned int& tID);
		void loadActionSettings();
		void initTexRenderer();
		void initFBO();
		void setScene();
		void SetupTexturemaps();

	public:
		VRHeadMountedDisplay();
		~VRHeadMountedDisplay();

		static VRHeadMountedDisplayPtr create();
		VRHeadMountedDisplayPtr ptr();

		static bool checkDeviceAttached();

		void initHMD();

		void render(bool fromThread = false);
};

OSG_END_NAMESPACE;

#endif // VRHEADMOUNTEDDISPLAY_H_INCLUDED