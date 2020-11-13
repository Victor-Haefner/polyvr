
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGL.h>
#include <OpenSG/OSGGLEXT.h>
#include <OpenSG/OSGGLFuncProtos.h>

#include <GL/glut.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstdlib>

#include <openvr.h>

#include <iostream>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGVector.h>

#include "core/utils/system/VRSystem.h"

using namespace std;
using namespace OSG;

class HMD_utils {
	public:
		static void checkGLError(string point) {
			cout << "checkGLError: " << point << endl;
			GLenum err;
			while ((err = glGetError()) != GL_NO_ERROR) {
				std::cout << err << endl;
			}
		}

		static void DumpDevice(const DISPLAY_DEVICE& dd, size_t nSpaceCount) {
			printf("%*sDevice Name: %s\n", nSpaceCount, "", dd.DeviceName);
			printf("%*sDevice String: %s\n", nSpaceCount, "", dd.DeviceString);
			printf("%*sState Flags: %x\n", nSpaceCount, "", dd.StateFlags);
			printf("%*sDeviceID: %s\n", nSpaceCount, "", dd.DeviceID);
			printf("%*sDeviceKey: ...%s\n\n", nSpaceCount, "", dd.DeviceKey + 42);
		}

		static void listSystemDevices() {
			DISPLAY_DEVICE dd;
			dd.cb = sizeof(DISPLAY_DEVICE);
			DWORD deviceNum = 0;
			while (EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {
				DumpDevice(dd, 0);
				DISPLAY_DEVICE newdd = { 0 };
				newdd.cb = sizeof(DISPLAY_DEVICE);
				DWORD monitorNum = 0;
				while (EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0)) {
					DumpDevice(newdd, 4);
					monitorNum++;
				}
				puts("");
				deviceNum++;
			}
			cout << endl;
		}
};

// ------------------------------ texture
#define RED 255,0,0,255
#define GRE 0,255,0,255
#define BLU 0,0,255,255
// ------------------------------

class CMainApplication {
private:
	int texSize = 16;
	vector<unsigned char> image;
	unsigned int textureID;

public:
	CMainApplication(int argc, char* argv[]);
	virtual ~CMainApplication();

	bool BInit();
	bool BInitGL();
	bool BInitCompositor();

	void Shutdown();

	void RenderFrame();

	void SetupTexturemaps();

	void SetupScene();
	void AddCubeToScene(Matrix mat, std::vector<float>& vertdata);
	void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float>& vertdata);

	void SetupCameras();

	void RenderCompanionWindow();
	void RenderScene(vr::Hmd_Eye nEye);

	Matrix GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	Matrix GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose();

	Matrix ConvertSteamVRMatrixToMatrix(const vr::HmdMatrix34_t& matPose);

	GLuint CompileGLShader(const char* pchShaderName, const char* pchVertexShader, const char* pchFragmentShader);
	bool CreateAllShaders();

private:
	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;

	vr::IVRSystem* m_pHMD;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	struct ControllerInfo_t {
		vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
		vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
		vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
		Matrix m_rmat4Pose;
		std::string m_sRenderModelName;
		bool m_bShowController;
	};

	enum EHand {
		Left = 0,
		Right = 1,
	};
	ControllerInfo_t m_rHand[2];

private: // SDL bookkeeping
	//SDL_Window* m_pCompanionWindow;
	int m_pCompanionWindow;
	UInt32 m_nCompanionWindowWidth;
	UInt32 m_nCompanionWindowHeight;

	//SDL_GLContext m_pContext;

private: // OpenGL bookkeeping
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;
	bool m_bShowCubes;
	Vec2f m_vAnalogValue;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip;
	float m_fFarClip;

	GLuint m_iTexture;

	unsigned int m_uiVertcount;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	Matrix m_mat4HMDPose;
	Matrix m_mat4eyePosLeft;
	Matrix m_mat4eyePosRight;

	Matrix m_mat4ProjectionCenter;
	Matrix m_mat4ProjectionLeft;
	Matrix m_mat4ProjectionRight;

	struct VertexDataScene {
		Vec3f position;
		Vec2f texCoord;
	};

	struct VertexDataWindow {
		Vec2f position;
		Vec2f texCoord;

		VertexDataWindow(const Vec2f& pos, const Vec2f tex) : position(pos), texCoord(tex) {	}
	};

	GLuint m_unSceneProgramID;
	GLuint m_unCompanionWindowProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLint m_nSceneMatrixLocation;
	GLint m_nControllerMatrixLocation;
	GLint m_nRenderModelMatrixLocation;

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc);

	UInt32 m_nRenderWidth;
	UInt32 m_nRenderHeight;

	vr::VRActionHandle_t m_actionHideCubes = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHideThisController = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionTriggerHaptic = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionAnalongInput = vr::k_ulInvalidActionHandle;

	vr::VRActionSetHandle_t m_actionsetDemo = vr::k_ulInvalidActionSetHandle;
};

CMainApplication* app = 0;

void display() {
	if (app) app->RenderFrame();
	glutPostRedisplay();
}

CMainApplication::CMainApplication(int argc, char* argv[])
	: m_pCompanionWindow(NULL)
	, m_nCompanionWindowWidth(640)
	, m_nCompanionWindowHeight(320)
	, m_unSceneProgramID(0)
	, m_unCompanionWindowProgramID(0)
	, m_unControllerTransformProgramID(0)
	, m_unRenderModelProgramID(0)
	, m_pHMD(NULL)
	, m_bDebugOpenGL(false)
	, m_bVerbose(false)
	, m_bPerf(false)
	, m_bVblank(false)
	, m_bGlFinishHack(true)
	, m_glControllerVertBuffer(0)
	, m_unControllerVAO(0)
	, m_unSceneVAO(0)
	, m_nSceneMatrixLocation(-1)
	, m_nControllerMatrixLocation(-1)
	, m_nRenderModelMatrixLocation(-1)
	, m_iTrackedControllerCount(0)
	, m_iTrackedControllerCount_Last(-1)
	, m_iValidPoseCount(0)
	, m_iValidPoseCount_Last(-1)
	, m_iSceneVolumeInit(20)
	, m_strPoseClasses("")
	, m_bShowCubes(true)
{
	app = this;
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
	glutInit(&argc, argv);
};

CMainApplication::~CMainApplication() {
	cout << "CMainApplication Shutdown" << endl;
}

std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL) {
	UInt32 unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0) return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit() {
	cout << "CMainApplication::BInit" << endl;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_STENCIL | GLUT_MULTISAMPLE);

	// Loading the SteamVR Runtime
	cout << " init headset" << endl;
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None) {
		m_pHMD = NULL;
		cout << "Unable to init VR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << endl;
		return false;
	}


	int nWindowPosX = 700;
	int nWindowPosY = 100;

	glutInitWindowSize(m_nCompanionWindowWidth, m_nCompanionWindowHeight);
	m_pCompanionWindow = glutCreateWindow("PolyVR");

	/*m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);
	if (m_pContext == NULL) {
		printf("%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}*/

	glGetError(); // to clear the error caused deep in GLEW


	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	m_strDisplay = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

	std::string strWindowTitle = "hellovr - " + m_strDriver + " " + m_strDisplay;
	glutSetWindowTitle(strWindowTitle.c_str());
	glutDisplayFunc(display);
	cout << " " << strWindowTitle << endl;

	// cube array
	m_iSceneVolumeWidth = m_iSceneVolumeInit;
	m_iSceneVolumeHeight = m_iSceneVolumeInit;
	m_iSceneVolumeDepth = m_iSceneVolumeInit;

	m_fScale = 0.3f;
	m_fScaleSpacing = 4.0f;

	m_fNearClip = 0.1f;
	m_fFarClip = 30.0f;

	m_iTexture = 0;
	m_uiVertcount = 0;

	// 		m_MillisecondsTimer.start(1, this);
	// 		m_SecondsTimer.start(1000, this);

	cout << " init GL" << endl;
	if (!BInitGL()) {
		cout << "Unable to initialize OpenGL!" << endl;		
		return false;
	}

	cout << " init compositor" << endl;
	if (!BInitCompositor()) {
		cout << "Failed to initialize VR Compositor!" << endl;
		return false;
	}

	string actionManifest = absolute("src\\core\\tests\\vive_actions.json");
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
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &m_rHand[Right].m_actionPose);

	cout << " init done" << endl;
	return true;
}

bool CMainApplication::BInitGL() {
	SetupTexturemaps();
	cout << "  setup scene" << endl;
	//SetupScene();
	SetupCameras();
	return true;
}

bool CMainApplication::BInitCompositor() {
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor()) {
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}

void CMainApplication::Shutdown() {
	if (m_pHMD) {
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	if (m_pCompanionWindow) {
		glutDestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}
}

void CMainApplication::RenderFrame() {
	RenderCompanionWindow();

	vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)textureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)textureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };

	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

	glFinish();
	glutSwapBuffers();
	glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glFlush();
	glFinish();

	UpdateHMDMatrixPose();
}

void CMainApplication::SetupTexturemaps() {
	cout << "  setup textures" << endl;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	image = {
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CMainApplication::SetupCameras() {
	cout << "  setup cameras" << endl;
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

void CMainApplication::RenderCompanionWindow() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);  glVertex3f(-1, -1, 0.0);
	glTexCoord2f(0.0, 1.0);  glVertex3f(-1,  1, 0.0);
	glTexCoord2f(1.0, 1.0);  glVertex3f( 0,  1, 0.0);
	glTexCoord2f(1.0, 0.0);  glVertex3f( 0, -1, 0.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);  glVertex3f(0, -1, 0.0);
	glTexCoord2f(0.0, 1.0);  glVertex3f(0,  1, 0.0);
	glTexCoord2f(1.0, 1.0);  glVertex3f(1,  1, 0.0);
	glTexCoord2f(1.0, 0.0);  glVertex3f(1, -1, 0.0);
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Matrix CMainApplication::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD) return Matrix();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

Matrix CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
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

Matrix CMainApplication::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye) {
	Matrix matMVP;
	if (nEye == vr::Eye_Left) {
		matMVP.mult(m_mat4ProjectionLeft);
		matMVP.mult(m_mat4eyePosLeft);
	} else if (nEye == vr::Eye_Right) {
		matMVP.mult(m_mat4ProjectionRight);
		matMVP.mult(m_mat4eyePosRight);
	}

	matMVP.mult(m_mat4HMDPose);
	return matMVP;
}

void CMainApplication::UpdateHMDMatrixPose() {
	if (!m_pHMD) return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

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

Matrix CMainApplication::ConvertSteamVRMatrixToMatrix(const vr::HmdMatrix34_t& matPose) {
	Matrix matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}

void startViveTest1(int argc, char** argv) {
	cout << "Start Vive test" << endl;
	//listSystemDevices();

	CMainApplication* pMainApplication = new CMainApplication(argc, argv);

	if (!pMainApplication->BInit()) {
		cout << " shutdown after failed init" << endl;
		pMainApplication->Shutdown();
		return;
	}

	glutMainLoop();
	cout << " shutdown" << endl;
	pMainApplication->Shutdown();
}