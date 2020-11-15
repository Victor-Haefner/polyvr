#include "VRHeadMountedDisplay.h"
#include "core/scene/VRScene.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"
#include "core/objects/OSGCamera.h"
#include "core/objects/OSGObject.h"
#include "core/tools/VRTextureRenderer.h"

#include "core/setup/VRSetup.h"
#include "core/setup/windows/VRWindow.h"

#include <openvr.h>
#include <iostream>

#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGVector.h>

#include <OpenSG/OSGGLEXT.h>
#include <OpenSG/OSGFrameBufferObject.h>
#include <OpenSG/OSGTextureBuffer.h>
#include <OpenSG/OSGRenderBuffer.h>
#include <OpenSG/OSGTextureObjChunk.h>
#include <OpenSG/OSGTextureEnvChunk.h>

#include <OpenSG/OSGPassiveWindow.h>
#include <OpenSG/OSGRenderAction.h>
#include <OpenSG/OSGFBOViewport.h>
#include <OpenSG/OSGMatrixCameraDecorator.h>
#include <OpenSG/OSGMatrixCamera.h>

using namespace OSG;

// ------------------------------ texture
#define RED 255,0,0,255
#define GRE 0,255,0,255
#define BLU 0,0,255,255

int testTexSize = 16;
vector<unsigned char> testImage;
unsigned int testTextureID;
unsigned int texIDL;
unsigned int texIDR;
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

VRHeadMountedDisplay::~VRHeadMountedDisplay() {
	cout << "~VRHeadMountedDisplay" << endl;
	if (fboData) delete fboData;
}

VRHeadMountedDisplayPtr VRHeadMountedDisplay::ptr() { return static_pointer_cast<VRHeadMountedDisplay>(shared_from_this()); }
VRHeadMountedDisplayPtr VRHeadMountedDisplay::create() { return VRHeadMountedDisplayPtr(new VRHeadMountedDisplay()); }

bool VRHeadMountedDisplay::checkDeviceAttached() {
	return vr::VR_IsHmdPresent();
}

struct VRHeadMountedDisplay::FBOData {
	VRTextureRendererPtr rendererL;
	VRTextureRendererPtr rendererR;
	MatrixCameraRecPtr mcamL;
	MatrixCameraRecPtr mcamR;
};

void VRHeadMountedDisplay::initFBO() {
	fboData = new FBOData();
}

void VRHeadMountedDisplay::initTexRenderer() {
	//renderer = VRTextureRenderer::create();
	
}

void VRHeadMountedDisplay::setScene() {
	auto scene = VRScene::getCurrent();
	if (!scene) return;
	VRCameraPtr cam = scene->getActiveCamera();
	VRObjectPtr root = VRScene::getCurrent()->getRoot();
	BackgroundRecPtr bg = VRScene::getCurrent()->getBackground();

	if (!fboData->rendererL) fboData->rendererL = VRTextureRenderer::create("hmdL", false);
	if (!fboData->rendererR) fboData->rendererR = VRTextureRenderer::create("hmdR", false);
	if (fboData->rendererL->getParent() == root && fboData->rendererR->getParent() == root) return;
	cout << " --- VRHeadMountedDisplay::setScene renderer" << endl;
	root->addChild(fboData->rendererL);
	root->addChild(fboData->rendererR);

	auto setupMatrixCam = [&](Matrix& m) {
		MatrixCameraRecPtr mcam = MatrixCamera::create();
		mcam->setBeacon(cam->getCam()->cam->getBeacon());
		//mcam->setUseBeacon(true);
		mcam->setProjectionMatrix(m);
		mcam->setNear(m_fNearClip);
		mcam->setFar(m_fFarClip);
		//mcam->setNear(cam->getCam()->cam->getNear());
		//mcam->setFar(cam->getCam()->cam->getFar());
		//mcam->setModelviewMatrix(); // needed or set by beacon?
		return mcam;
	};

	Matrix mL = m_mat4ProjectionLeft;
	mL.mult(m_mat4eyePosLeft);
	Matrix mR = m_mat4ProjectionRight;
	mR.mult(m_mat4eyePosRight);

	fboData->mcamL = setupMatrixCam(mL);
	fboData->mcamR = setupMatrixCam(mR);

	fboData->rendererL->setup(cam, m_nRenderWidth, m_nRenderHeight, true);
	fboData->rendererL->setStageCam(OSGCamera::create(fboData->mcamL));
	fboData->rendererL->updateBackground();
	fboData->rendererL->addLink(root->getChild(0));

	fboData->rendererR->setup(cam, m_nRenderWidth, m_nRenderHeight, true);
	fboData->rendererR->setStageCam(OSGCamera::create(fboData->mcamR));
	fboData->rendererR->updateBackground();
	fboData->rendererR->addLink(root->getChild(0));
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
	SetupTexturemaps();
	initFBO();
	initTexRenderer();

	if (!vr::VRCompositor()) {
		printf("Compositor initialization failed. See log file for details\n");
		return;
	}

	//loadActionSettings();


	valid = true;
}

void VRHeadMountedDisplay::RenderStereoTargets() {
	glClearColor(0.4f, 0.8f, 1.0f, 1.0f);
}

void VRHeadMountedDisplay::findTestImg(VRTextureRendererPtr renderer, unsigned int& tID) {
	auto scene = VRScene::getCurrent();
	if (!scene || !renderer) return;
	/*auto obj = dynamic_pointer_cast<VRGeometry>( scene->get("cube") );
	if (!obj) return;
	auto mat = obj->getMaterial();*/
	auto mat = renderer->getMaterial();
	if (!mat) return;
	auto tChunk = mat->getTextureObjChunk();
	if (!tChunk) return;
	auto setup = VRSetup::getCurrent();
	if (!setup) return;
	auto vrwin = setup->getWindow("screen");
	if (!vrwin) return;
	auto win = vrwin->getOSGWindow();
	if (!win) return;
	unsigned int texID = win->getGLObjectId(tChunk->getGLId());
	if (texID != tID && texID > 0) {
		tID = texID;
		cout << " --- YAY " << texID << endl;
	}
}

void VRHeadMountedDisplay::render(bool fromThread) {
	if (fromThread || fboData == 0) return;

	setScene(); // TODO: put this in callback when new scene

	if (m_pHMD) {
		RenderStereoTargets();
		findTestImg(fboData->rendererL, texIDL);
		findTestImg(fboData->rendererR, texIDR);
		Matrix mcW = toMatrix4f(fboData->rendererL->getCamera()->getWorldMatrix());
		Matrix mvm = m_mat4HMDPose;
		mvm.mult(mcW);
		fboData->mcamL->setModelviewMatrix(mvm);
		fboData->mcamR->setModelviewMatrix(mvm);
		vr::Texture_t leftEyeTexture  = { (void*)(uintptr_t)texIDL, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)texIDR, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left , &leftEyeTexture);
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	glFlush();
	glFinish();

	UpdateHMDMatrixPose(); // maybe put this in front
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

	texIDL = testTextureID;
	texIDR = testTextureID;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (int i = 0; i < m_nRenderHeight; i++) {
		for (int j = 0; j < m_nRenderWidth; j++) {
			if (i == j) { testImage.push_back(0); testImage.push_back(0); testImage.push_back(255); testImage.push_back(255); }
			else if (i%2 == 0) { testImage.push_back(0); testImage.push_back(255); testImage.push_back(0); testImage.push_back(255); }
			else { testImage.push_back(255); testImage.push_back(0); testImage.push_back(0); testImage.push_back(255); }
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_nRenderWidth, m_nRenderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &testImage[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Matrix VRHeadMountedDisplay::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();
	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);
	return convertMatrix(mat);
}

Matrix VRHeadMountedDisplay::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();
	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix matrixObj = convertMatrix(matEyeRight);
	matrixObj.invert();
	return matrixObj;
}

void VRHeadMountedDisplay::UpdateHMDMatrixPose() {
	if (!m_pHMD) return;

	vr::VRCompositor()->WaitGetPoses(&m_rTrackedDevicePose[0], vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice) {
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid) {
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = convertMatrix(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
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

Matrix VRHeadMountedDisplay::convertMatrix(const vr::HmdMatrix34_t& mat) {
	return Matrix(
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		0          ,           0,           0,           1
	);
}

Matrix VRHeadMountedDisplay::convertMatrix(const vr::HmdMatrix44_t& mat) {
	return Matrix(
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]
	);
}