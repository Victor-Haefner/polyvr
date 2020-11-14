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
	FrameBufferObjectRefPtr fbo;
	TextureObjChunkRefPtr   fboTex;
	ImageRefPtr             fboTexImg;
	TextureObjChunkRefPtr   fboDTex;
	ImageRefPtr             fboDTexImg;

	RenderActionRefPtr		ract;
	WindowMTRecPtr			win;
	FBOViewportRecPtr		fboView;

	VRCameraPtr cam;

	MatrixCameraRecPtr mcamL;
	MatrixCameraRecPtr mcamR;
};

void VRHeadMountedDisplay::initFBO() {
	fboData = new FBOData();

	fboData->fboTex = TextureObjChunk::create();
	fboData->fboTex->setMinFilter(GL_NEAREST);
	fboData->fboTex->setMagFilter(GL_NEAREST);
	//fboData->fboTex->setWrapS(GL_REPEAT);
	//fboData->fboTex->setWrapT(GL_REPEAT);
	fboData->fboTex->setWrapS(GL_CLAMP_TO_EDGE);
	fboData->fboTex->setWrapT(GL_CLAMP_TO_EDGE);

	fboData->fboTexImg = Image::create();
	fboData->fboTexImg->set(Image::OSG_RGBA_PF, m_nRenderWidth, m_nRenderHeight);
	//fboData->fboTexImg->set(Image::OSG_RGBA_PF, fboData->fboWidth, fboData->fboHeight, 1, 0, 0, 0, 0, Image::OSG_UINT8_IMAGEDATA);
	//fboData->fboTexImg->set(Image::OSG_RGBA_PF, testTexSize, testTexSize, 1, 0, 0, 0, &testImage[0], Image::OSG_UINT8_IMAGEDATA);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, testTexSize, testTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &testImage[0]);
	fboData->fboTex->setImage(fboData->fboTexImg);
	//fboData->fboTex->setInternalFormat(Image::OSG_UINT8_IMAGEDATA);



	TextureBufferRefPtr texBuf = TextureBuffer::create();
	texBuf->setTexture(fboData->fboTex);

	fboData->fboDTexImg = Image::create();
	fboData->fboDTexImg->set(Image::OSG_RGBA_PF, m_nRenderWidth, m_nRenderHeight);
	fboData->fboDTex = TextureObjChunk::create();
	fboData->fboDTex->setImage(fboData->fboDTexImg);
	fboData->fboDTex->setMinFilter(GL_NEAREST);
	fboData->fboDTex->setMagFilter(GL_NEAREST);
	fboData->fboDTex->setWrapS(GL_CLAMP_TO_EDGE);
	fboData->fboDTex->setWrapT(GL_CLAMP_TO_EDGE);
	fboData->fboDTex->setExternalFormat(GL_DEPTH_COMPONENT);
	fboData->fboDTex->setInternalFormat(GL_DEPTH_COMPONENT24); //24/32
	fboData->fboDTex->setCompareMode(GL_NONE);
	fboData->fboDTex->setCompareFunc(GL_LEQUAL);
	fboData->fboDTex->setDepthMode(GL_INTENSITY);
	TextureBufferRefPtr texDBuf = TextureBuffer::create();
	texDBuf->setTexture(fboData->fboDTex);

	RenderBufferRefPtr depthBuf = RenderBuffer::create();
	depthBuf->setInternalFormat(GL_DEPTH_COMPONENT24);

	fboData->fbo = FrameBufferObject::create();
	fboData->fbo->setColorAttachment(texBuf, 0);
	//data->fbo->setColorAttachment(texDBuf, 1);
	fboData->fbo->setDepthAttachment(texDBuf); //HERE depthBuf/texDBuf
	fboData->fbo->editMFDrawBuffers()->push_back(GL_DEPTH_ATTACHMENT_EXT);
	fboData->fbo->editMFDrawBuffers()->push_back(GL_COLOR_ATTACHMENT0_EXT);
	fboData->fbo->setWidth(m_nRenderWidth);
	fboData->fbo->setHeight(m_nRenderHeight);
	fboData->fbo->setPostProcessOnDeactivate(true);

	texBuf->setReadBack(true);
	texDBuf->setReadBack(true);

	fboData->ract = RenderAction::create();
	fboData->win = PassiveWindow::create();
	fboData->fboView = FBOViewport::create();
	fboData->fboView->setFrameBufferObject(fboData->fbo);

	fboData->win->addPort(fboData->fboView);
	fboData->fboView->setSize(0, 0, 1, 1);
}

void VRHeadMountedDisplay::initTexRenderer() {
	//renderer = VRTextureRenderer::create();
	
}

// TODO
//  - camera matrix decorator
//  - left and right eye

void VRHeadMountedDisplay::setScene() {
	auto scene = VRScene::getCurrent();
	if (!scene) return;
	VRCameraPtr cam = scene->getActiveCamera();
	VRObjectPtr root = VRScene::getCurrent()->getRoot();
	BackgroundRecPtr bg = VRScene::getCurrent()->getBackground();
	/*if (fboData) {
		if (cam == fboData->cam) return;
		cout << " --- VRHeadMountedDisplay::setScene fboData" << endl;
		fboData->cam = cam;
		fboData->fboView->setBackground(bg);
		fboData->fboView->setCamera(cam->getCam()->cam);
		fboData->fboView->setRoot(root->getNode()->node);
	}*/

	if (!rendererL) rendererL = VRTextureRenderer::create("hmdL", false);
	if (!rendererR) rendererR = VRTextureRenderer::create("hmdR", false);
	if (rendererL->getParent() == root && rendererR->getParent() == root) return;
	cout << " --- VRHeadMountedDisplay::setScene renderer" << endl;
	root->addChild(rendererL);
	root->addChild(rendererR);

	auto setupMatrixCam = [&](Matrix& m) {
		MatrixCameraRecPtr mcam = MatrixCamera::create();
		mcam->setBeacon(cam->getCam()->cam->getBeacon());
		//mcam->setUseBeacon(true);
		mcam->setProjectionMatrix(m);
		mcam->setNear(0.1);
		mcam->setFar(1000);
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

	rendererL->setup(cam, m_nRenderWidth, m_nRenderHeight, true);
	rendererL->setStageCam(OSGCamera::create(fboData->mcamL));
	rendererL->updateBackground();
	rendererL->addLink(root->getChild(0));

	rendererR->setup(cam, m_nRenderWidth, m_nRenderHeight, true);
	rendererR->setStageCam(OSGCamera::create(fboData->mcamR));
	rendererR->updateBackground();
	rendererR->addLink(root->getChild(0));
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
	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	if (!vr::VRCompositor()) {
		printf("Compositor initialization failed. See log file for details\n");
		return;
	}

	//loadActionSettings();


	valid = true;
}

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

void replaceTexGLID(unsigned int& tID, VRHeadMountedDisplay::FBOData* fboData) {
	if (!fboData->fboTex || !fboData->win) return;
	/*auto setup = VRSetup::getCurrent();
	if (!setup) return;
	auto vrwin = setup->getWindow("screen");
	if (!vrwin) return;
	auto win = vrwin->getOSGWindow();
	if (!win) return;*/
	unsigned int texID = fboData->win->getGLObjectId(fboData->fboTex->getGLId());
	if (texID != tID && texID > 0) {
		tID = texID;
		cout << " --- YAY " << tID << endl;
	}
}

void VRHeadMountedDisplay::render(bool fromThread) {
	if (fromThread || fboData == 0) return;

	setScene(); // TODO: put this in callback when new scene
	//fboData->win->render(fboData->ract);


	/*ImageMTRecPtr img = Image::create();
	img->set(fboData->fboTexImg);
	img->write("tmp.png");*/

	if (m_pHMD) {
		RenderStereoTargets();
		findTestImg(rendererL, texIDL);
		findTestImg(rendererR, texIDR);
		Matrix mcW = toMatrix4f(rendererL->getCamera()->getWorldMatrix());
		Matrix mvm = m_mat4HMDPose;
		mvm.mult(mcW);
		fboData->mcamL->setModelviewMatrix(mvm);
		fboData->mcamR->setModelviewMatrix(mvm);
		//replaceTexGLID(testTextureID, fboData);
		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)texIDL, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)texIDR, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
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
	return ConvertMatrix(mat);
}

Matrix VRHeadMountedDisplay::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();
	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix matrixObj = ConvertMatrix(matEyeRight);
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
			m_rmat4DevicePose[nDevice] = ConvertMatrix(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
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

Matrix VRHeadMountedDisplay::ConvertMatrix(const vr::HmdMatrix34_t& mat) {
	Matrix matrixObj(
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		0, 0, 0, 1
	);
	return matrixObj;
}

Matrix VRHeadMountedDisplay::ConvertMatrix(const vr::HmdMatrix44_t& mat) {
	Matrix matrixObj(
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]
	);
	return matrixObj;
}