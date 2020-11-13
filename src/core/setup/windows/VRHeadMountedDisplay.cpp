#include "VRHeadMountedDisplay.h"

#include <openvr.h>
#include <iostream>

#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGVector.h>

using namespace OSG;

// ------------------------------ texture
#define RED 255,0,0,255
#define GRE 0,255,0,255
#define BLU 0,0,255,255

int testTexSize = 16;
vector<unsigned char> testImage;
unsigned int testTextureID;
// ------------------------------


std::string GetViveDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL) {
	UInt32 unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0) return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

VRHeadMountedDisplay::VRHeadMountedDisplay() {}

bool VRHeadMountedDisplay::checkDeviceAttached() {
	return vr::VR_IsHmdPresent();
}

void VRHeadMountedDisplay::initHMD() {
	cout << "VRHeadMountedDisplay: init" << endl;

	m_rTrackedDevicePose.resize(vr::k_unMaxTrackedDeviceCount);
	m_rmat4DevicePose.resize(vr::k_unMaxTrackedDeviceCount);
	m_rDevClassChar.resize(vr::k_unMaxTrackedDeviceCount);

	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None) {
		m_pHMD = NULL;
		cout << "Unable to init VR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << endl;
		return;
	}

	m_strDriver = GetViveDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	m_strDisplay = GetViveDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);


	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	if (!vr::VRCompositor()) {
		printf("Compositor initialization failed. See log file for details\n");
		return;
	}

	//loadActionSettings();

	SetupTexturemaps();

	//wglMakeCurrent(0, 0);
	valid = true;
}

VRHeadMountedDisplay::~VRHeadMountedDisplay() {
	cout << "~VRHeadMountedDisplay" << endl;
}

VRHeadMountedDisplayPtr VRHeadMountedDisplay::ptr() { return static_pointer_cast<VRHeadMountedDisplay>(shared_from_this()); }
VRHeadMountedDisplayPtr VRHeadMountedDisplay::create() { return VRHeadMountedDisplayPtr(new VRHeadMountedDisplay()); }

void VRHeadMountedDisplay::RenderStereoTargets() {
	glClearColor(0.4f, 0.8f, 1.0f, 1.0f);
	/*glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);*/
}

void VRHeadMountedDisplay::render(bool fromThread) {
	if (fromThread) return;

	//wglMakeCurrent(glDevice, glContext);

	if (m_pHMD) {
		RenderStereoTargets();
		//cout << "render to HMD" << endl;
		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)testTextureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)testTextureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	glFlush();
	glFinish();

	//wglMakeCurrent(0, 0);

	// SwapWindow
	//glutSwapBuffers();

	// Clear
	/*glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFlush();
	glFinish();*/

	// Spew out the controller and pose count whenever they change.
	/*if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last) {
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;
		dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
	}*/

	UpdateHMDMatrixPose();
}

void VRHeadMountedDisplay::loadActionSettings() {
	/*string actionManifest = absolute("src\\core\\tests\\vive_actions.json");
	cout << " init vr input actions, manifest: " << actionManifest << endl;
	vr::VRInput()->SetActionManifestPath(actionManifest.c_str());

	vr::VRInput()->GetActionHandle("/actions/demo/in/HideCubes", &m_actionHideCubes);
	vr::VRInput()->GetActionHandle("/actions/demo/in/HideThisController", &m_actionHideThisController);
	vr::VRInput()->GetActionHandle("/actions/demo/in/TriggerHaptic", &m_actionTriggerHaptic);
	vr::VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &m_actionAnalongInput);

	vr::VRInput()->GetActionSetHandle("/actions/demo", &m_actionsetDemo);

	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Left", &m_rHand[Left].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &m_rHand[Left].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Left", &m_rHand[Left].m_actionPose);

	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Right", &m_rHand[Right].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &m_rHand[Right].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &m_rHand[Right].m_actionPose);*/
}

void VRHeadMountedDisplay::SetupTexturemaps() {
	cout << "  setup HMD test texture" << endl;

	glGenTextures(1, &testTextureID);
	glBindTexture(GL_TEXTURE_2D, testTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	testImage = {
		BLU, GRE, RED, GRE,BLU, GRE, RED, GRE,BLU, GRE, RED, GRE,BLU, GRE, RED, GRE,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		RED, GRE, RED, GRE,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
		GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,GRE, RED, GRE, RED,
	};

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, testTexSize, testTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &testImage[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Matrix VRHeadMountedDisplay::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

Matrix VRHeadMountedDisplay::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	matrixObj.invert();
	return matrixObj;
}

Matrix VRHeadMountedDisplay::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye) {
	Matrix matMVP;
	if (nEye == vr::Eye_Left) {
		matMVP.mult(m_mat4ProjectionLeft);
		matMVP.mult(m_mat4eyePosLeft);
	}
	else if (nEye == vr::Eye_Right) {
		matMVP.mult(m_mat4ProjectionRight);
		matMVP.mult(m_mat4eyePosRight);
	}

	matMVP.mult(m_mat4HMDPose);
	return matMVP;
}

void VRHeadMountedDisplay::UpdateHMDMatrixPose() {
	if (!m_pHMD) return;

	vr::VRCompositor()->WaitGetPoses(&m_rTrackedDevicePose[0], vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice) {
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid) {
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0) {
				switch (m_pHMD->GetTrackedDeviceClass(nDevice)) {
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}

Matrix VRHeadMountedDisplay::ConvertSteamVRMatrixToMatrix(const vr::HmdMatrix34_t& matPose) {
	Matrix matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}