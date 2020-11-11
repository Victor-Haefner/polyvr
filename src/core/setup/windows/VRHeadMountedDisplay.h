#ifndef VRHEADMOUNTEDDISPLAY_H_INCLUDED
#define VRHEADMOUNTEDDISPLAY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGMatrix.h>
#include <string>
#include <vector>
#include "VRWindow.h"

namespace vr {
	class IVRSystem;
	struct HmdMatrix34_t;
	class TrackedDevicePose_t;

	enum EVREye;
}

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRHeadMountedDisplay : public VRWindow {
	private:
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
		string m_strPoseClasses = "";

		Matrix m_mat4HMDPose;
		Matrix m_mat4ProjectionLeft;
		Matrix m_mat4ProjectionRight;
		Matrix m_mat4eyePosLeft;
		Matrix m_mat4eyePosRight;

		Matrix GetHMDMatrixProjectionEye(vr::EVREye nEye);
		Matrix GetHMDMatrixPoseEye(vr::EVREye nEye);
		Matrix GetCurrentViewProjectionMatrix(vr::EVREye nEye);
		void UpdateHMDMatrixPose();
		Matrix ConvertSteamVRMatrixToMatrix(const vr::HmdMatrix34_t& matPose);

		void loadActionSettings();

		void RenderStereoTargets();
		void SetupTexturemaps();

	public:
		VRHeadMountedDisplay();
		~VRHeadMountedDisplay();

		static VRHeadMountedDisplayPtr create();
		VRHeadMountedDisplayPtr ptr();

		void render();
};

OSG_END_NAMESPACE;

#endif // VRHEADMOUNTEDDISPLAY_H_INCLUDED