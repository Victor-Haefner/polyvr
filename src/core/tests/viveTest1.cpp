
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

#if defined(POSIX)
#include "unistd.h"
#endif

#ifndef _WIN32
#define APIENTRY
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}

void checkGLError(string point) {
	cout << "checkGLError: " << point << endl;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cout << err << endl;
	}
}

void DumpDevice(const DISPLAY_DEVICE& dd, size_t nSpaceCount) {
	printf("%*sDevice Name: %s\n", nSpaceCount, "", dd.DeviceName);
	printf("%*sDevice String: %s\n", nSpaceCount, "", dd.DeviceString);
	printf("%*sState Flags: %x\n", nSpaceCount, "", dd.StateFlags);
	printf("%*sDeviceID: %s\n", nSpaceCount, "", dd.DeviceID);
	printf("%*sDeviceKey: ...%s\n\n", nSpaceCount, "", dd.DeviceKey + 42);
}

void listSystemDevices() {
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

// ------------------------------ texture
#define RED 255,0,0,255
#define GRE 0,255,0,255
#define BLU 0,0,255,255

int texSize = 16;
vector<unsigned char> image;
unsigned int textureID;
// ------------------------------

static bool g_bPrintf = true;

//-----------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
class CMainApplication {
public:
	CMainApplication(int argc, char* argv[]);
	virtual ~CMainApplication();

	bool BInit();
	bool BInitGL();
	bool BInitCompositor();

	void Shutdown();

	void RunMainLoop();
	bool HandleInput();
	void ProcessVREvent(const vr::VREvent_t& event);
	void RenderFrame();

	void SetupTexturemaps();

	void SetupScene();
	void AddCubeToScene(Matrix mat, std::vector<float>& vertdata);
	void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float>& vertdata);

	bool SetupStereoRenderTargets();
	void SetupCameras();

	void RenderStereoTargets();
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


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a rising edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr) {
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath) {
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive) {
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo))) {
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a falling edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && !actionData.bState;
}


//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf(const char* fmt, ...)
{
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	if (g_bPrintf)
		printf("%s", buffer);

	OutputDebugStringA(buffer);
}

CMainApplication* app = 0;

void display() {
	if (app) app->RenderFrame();
	glutPostRedisplay();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
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

	for (int i = 1; i < argc; i++)
	{
		if (!stricmp(argv[i], "-gldebug"))
		{
			m_bDebugOpenGL = true;
		}
		else if (!stricmp(argv[i], "-verbose"))
		{
			m_bVerbose = true;
		}
		else if (!stricmp(argv[i], "-novblank"))
		{
			m_bVblank = false;
		}
		else if (!stricmp(argv[i], "-noglfinishhack"))
		{
			m_bGlFinishHack = false;
		}
		else if (!stricmp(argv[i], "-noprintf"))
		{
			g_bPrintf = false;
		}
		else if (!stricmp(argv[i], "-cubevolume") && (argc > i + 1) && (*argv[i + 1] != '-'))
		{
			m_iSceneVolumeInit = atoi(argv[i + 1]);
			i++;
		}
	}
	// other initialization tasks are done in BInit
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));

	glutInit(&argc, argv);
};


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication()
{
	// work is done in Shutdown
	dprintf("Shutdown");
}


//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
// Purpose: Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf("GL Error: %s\n", message);
}


//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 
//-----------------------------------------------------------------------------
bool CMainApplication::BInitGL() {
	SetupTexturemaps();
	cout << "  setup scene" << endl;
	//SetupScene();
	SetupCameras();
	SetupStereoRenderTargets();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::Shutdown()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	/*if (m_pContext) {
		if (m_bDebugOpenGL)
		{
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
			glDebugMessageCallback(nullptr, nullptr);
		}
		glDeleteBuffers(1, &m_glSceneVertBuffer);

		if (m_unSceneProgramID)
		{
			glDeleteProgram(m_unSceneProgramID);
		}
		if (m_unControllerTransformProgramID)
		{
			glDeleteProgram(m_unControllerTransformProgramID);
		}
		if (m_unRenderModelProgramID)
		{
			glDeleteProgram(m_unRenderModelProgramID);
		}
		if (m_unCompanionWindowProgramID)
		{
			glDeleteProgram(m_unCompanionWindowProgramID);
		}

		glDeleteRenderbuffers(1, &leftEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nResolveFramebufferId);

		glDeleteRenderbuffers(1, &rightEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nResolveFramebufferId);

		if (m_unCompanionWindowVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unCompanionWindowVAO);
		}
		if (m_unSceneVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unSceneVAO);
		}
		if (m_unControllerVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unControllerVAO);
		}
	}*/

	if (m_pCompanionWindow)
	{
		//SDL_DestroyWindow(m_pCompanionWindow);
		glutDestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput() {
	bool bRet = false;
	/*SDL_Event sdlEvent;

	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		if (sdlEvent.type == SDL_QUIT)
		{
			bRet = true;
		}
		else if (sdlEvent.type == SDL_KEYDOWN)
		{
			if (sdlEvent.key.keysym.sym == SDLK_ESCAPE
				|| sdlEvent.key.keysym.sym == SDLK_q)
			{
				bRet = true;
			}
			if (sdlEvent.key.keysym.sym == SDLK_c)
			{
				m_bShowCubes = !m_bShowCubes;
			}
		}
	}*/

	// Process SteamVR events
	vr::VREvent_t event;
	while (m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}

	// Process SteamVR action state
	// UpdateActionState is called each frame to update the state of the actions themselves. The application
	// controls which action sets are active with the provided array of VRActiveActionSet_t structs.
	vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = m_actionsetDemo;
	vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	m_bShowCubes = !GetDigitalActionState(m_actionHideCubes);

	vr::VRInputValueHandle_t ulHapticDevice;
	if (GetDigitalActionRisingEdge(m_actionTriggerHaptic, &ulHapticDevice))
	{
		if (ulHapticDevice == m_rHand[Left].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Left].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
		if (ulHapticDevice == m_rHand[Right].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Right].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
	}

	vr::InputAnalogActionData_t analogData;
	if (vr::VRInput()->GetAnalogActionData(m_actionAnalongInput, &analogData, sizeof(analogData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && analogData.bActive)
	{
		m_vAnalogValue[0] = analogData.x;
		m_vAnalogValue[1] = analogData.y;
	}

	m_rHand[Left].m_bShowController = true;
	m_rHand[Right].m_bShowController = true;

	vr::VRInputValueHandle_t ulHideDevice;
	if (GetDigitalActionState(m_actionHideThisController, &ulHideDevice))
	{
		if (ulHideDevice == m_rHand[Left].m_source)
		{
			m_rHand[Left].m_bShowController = false;
		}
		if (ulHideDevice == m_rHand[Right].m_source)
		{
			m_rHand[Right].m_bShowController = false;
		}
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CMainApplication::RunMainLoop() {
	cout << " start glut main loop" << endl;
	glutMainLoop();

	/*bool bQuit = false;

	SDL_StartTextInput();
	SDL_ShowCursor(SDL_DISABLE);

	while (!bQuit)
	{
		bQuit = HandleInput();

		RenderFrame();
	}

	SDL_StopTextInput();*/
}


//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent(const vr::VREvent_t& event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		dprintf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		dprintf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame() {
	//cout << "render frame" << endl;
	// for now as fast as possible
	if (m_pHMD) {
		RenderStereoTargets();
		RenderCompanionWindow();

		//cout << " submit to compositor left" << endl;
		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)textureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		//cout << " submit to compositor right" << endl;
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)textureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	if (m_bVblank && m_bGlFinishHack) {
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		//cout << " glFinish" << endl;
		glFinish();
	}

	// SwapWindow
	{
		//cout << " swap buffer" << endl;
		glutSwapBuffers();
	}

	// Clear
	{
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		//cout << " glClear" << endl;
		glClearColor(1, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Flush and wait for swap.
	if (m_bVblank)
	{
		//cout << " glFlush" << endl;
		glFlush();
		//cout << " glFinish" << endl;
		glFinish();
	}

	// Spew out the controller and pose count whenever they change.
	if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last)
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;

		dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
	}

	//cout << " update HMD matrix" << endl;
	UpdateHMDMatrixPose();
	//cout << " render frame done" << endl;
}


//-----------------------------------------------------------------------------
// Purpose: Compiles a GL shader program and returns the handle. Returns 0 if
//			the shader couldn't be compiled for some reason.
//-----------------------------------------------------------------------------
GLuint CMainApplication::CompileGLShader(const char* pchShaderName, const char* pchVertexShader, const char* pchFragmentShader)
{
	/*cout << "    compile shaderrr " << pchShaderName << endl;
	GLuint unProgramID = glCreateProgram();
	cout << "    A01 " << unProgramID << endl;

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	cout << "    A02" << endl;
	glShaderSource(nSceneVertexShader, 1, &pchVertexShader, NULL);
	cout << "    A03" << endl;
	glCompileShader(nSceneVertexShader);

	cout << "    A1" << endl;
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile vertex shader %d!\n", pchShaderName, nSceneVertexShader);
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneVertexShader);
		return 0;
	}
	cout << "    A2" << endl;
	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader(nSceneFragmentShader);

	cout << "    A3" << endl;
	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		dprintf("%s - Unable to compile fragment shader %d!\n", pchShaderName, nSceneFragmentShader);
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneFragmentShader);
		return 0;
	}

	cout << "    A4" << endl;
	glAttachShader(unProgramID, nSceneFragmentShader);
	glDeleteShader(nSceneFragmentShader); // the program hangs onto this once it's attached

	glLinkProgram(unProgramID);

	cout << "    A5" << endl;
	GLint programSuccess = GL_TRUE;
	glGetProgramiv(unProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE)
	{
		dprintf("%s - Error linking program %d!\n", pchShaderName, unProgramID);
		glDeleteProgram(unProgramID);
		return 0;
	}

	cout << "    A6" << endl;
	glUseProgram(unProgramID);
	glUseProgram(0);

	cout << "    A7" << endl;
	return unProgramID;*/
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
bool CMainApplication::CreateAllShaders()
{
	/*
	cout << "   compile first shader" << endl;
	m_unSceneProgramID = CompileGLShader(
		"Scene",

		// Vertex Shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVcoordsIn;\n"
		"layout(location = 2) in vec3 v3NormalIn;\n"
		"out vec2 v2UVcoords;\n"
		"void main()\n"
		"{\n"
		"	v2UVcoords = v2UVcoordsIn;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// Fragment Shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"in vec2 v2UVcoords;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture(mytexture, v2UVcoords);\n"
		"}\n"
	);
	cout << "   get matrix uniform" << endl;
	m_nSceneMatrixLocation = glGetUniformLocation(m_unSceneProgramID, "matrix");
	if (m_nSceneMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in scene shader\n");
		return false;
	}

	cout << "   compile second shader" << endl;
	m_unControllerTransformProgramID = CompileGLShader(
		"Controller",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3ColorIn;\n"
		"out vec4 v4Color;\n"
		"void main()\n"
		"{\n"
		"	v4Color[0]yz = v3ColorIn; v4Color.a = 1.0;\n"
		"	gl_Position = matrix * position;\n"
		"}\n",

		// fragment shader
		"#version 410\n"
		"in vec4 v4Color;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = v4Color;\n"
		"}\n"
	);
	cout << "   get matrix uniform" << endl;
	m_nControllerMatrixLocation = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");
	if (m_nControllerMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in controller shader\n");
		return false;
	}

	cout << "   compile third shader" << endl;
	m_unRenderModelProgramID = CompileGLShader(
		"render model",

		// vertex shader
		"#version 410\n"
		"uniform mat4 matrix;\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec3 v3NormalIn;\n"
		"layout(location = 2) in vec2 v2TexCoordsIn;\n"
		"out vec2 v2TexCoord;\n"
		"void main()\n"
		"{\n"
		"	v2TexCoord = v2TexCoordsIn;\n"
		"	gl_Position = matrix * vec4(position[0]yz, 1);\n"
		"}\n",

		//fragment shader
		"#version 410 core\n"
		"uniform sampler2D diffuse;\n"
		"in vec2 v2TexCoord;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = texture( diffuse, v2TexCoord);\n"
		"}\n"

	);
	cout << "   get matrix uniform" << endl;
	m_nRenderModelMatrixLocation = glGetUniformLocation(m_unRenderModelProgramID, "matrix");
	if (m_nRenderModelMatrixLocation == -1)
	{
		dprintf("Unable to find matrix uniform in render model shader\n");
		return false;
	}

	cout << "   compile fourth shader" << endl;
	m_unCompanionWindowProgramID = CompileGLShader(
		"CompanionWindow",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVIn;\n"
		"noperspective out vec2 v2UV;\n"
		"void main()\n"
		"{\n"
		"	v2UV = v2UVIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"
		"noperspective in vec2 v2UV;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"		outputColor = texture(mytexture, v2UV);\n"
		"}\n"
	);

	cout << "   shader compilation done" << endl;
	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unCompanionWindowProgramID != 0;*/
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
// Purpose: create a sea of cubes
//-----------------------------------------------------------------------------
void CMainApplication::SetupScene()
{
	/*if (!m_pHMD) return;

	std::vector<float> vertdataarray;

	Matrix matTransform;
	matTransform.setTranslate(
		-((float)m_iSceneVolumeWidth * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeHeight * m_fScaleSpacing) / 2.f,
		-((float)m_iSceneVolumeDepth * m_fScaleSpacing) / 2.f);

	Matrix mat;
	mat.setScale(Vec3f(m_fScale, m_fScale, m_fScale));
	mat.mult(matTransform);

	Matrix T1, T2, T3;
	T1.setTranslate(m_fScaleSpacing, 0, 0);
	T2.setTranslate(-((float)m_iSceneVolumeWidth) * m_fScaleSpacing, m_fScaleSpacing, 0);
	T3.setTranslate(0, -((float)m_iSceneVolumeHeight) * m_fScaleSpacing, m_fScaleSpacing);

	for (int z = 0; z < m_iSceneVolumeDepth; z++) {
		for (int y = 0; y < m_iSceneVolumeHeight; y++) {
			for (int x = 0; x < m_iSceneVolumeWidth; x++) {
				AddCubeToScene(mat, vertdataarray);
				mat.mult(T1);
			}
			mat.mult(T2);
		}
		mat.mult(T3);
	}
	m_uiVertcount = vertdataarray.size() / 5;

	glGenVertexArrays(1, &m_unSceneVAO);
	glBindVertexArray(m_unSceneVAO);

	glGenBuffers(1, &m_glSceneVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);

	GLsizei stride = sizeof(VertexDataScene);
	uintptr_t offset = 0;

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

	offset += sizeof(Vec3f);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)offset);

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);*/

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, std::vector<float>& vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::AddCubeToScene(Matrix mat, std::vector<float>& vertdata)
{
	// Matrix mat( outermat.data() );

	Vec4f A, B, C, D, E, F, G, H;
	mat.mult(Vec4f(0, 0, 0, 1), A);
	mat.mult(Vec4f(1, 0, 0, 1), B);
	mat.mult(Vec4f(1, 1, 0, 1), B);
	mat.mult(Vec4f(0, 1, 0, 1), C);
	mat.mult(Vec4f(0, 0, 1, 1), D);
	mat.mult(Vec4f(1, 0, 1, 1), E);
	mat.mult(Vec4f(1, 1, 1, 1), F);
	mat.mult(Vec4f(0, 1, 1, 1), G);

	// triangles instead of quads
	AddCubeVertex(E[0], E[1], E[2], 0, 1, vertdata); //Front
	AddCubeVertex(F[0], F[1], F[2], 1, 1, vertdata);
	AddCubeVertex(G[0], G[1], G[2], 1, 0, vertdata);
	AddCubeVertex(G[0], G[1], G[2], 1, 0, vertdata);
	AddCubeVertex(H[0], H[1], H[2], 0, 0, vertdata);
	AddCubeVertex(E[0], E[1], E[2], 0, 1, vertdata);

	AddCubeVertex(B[0], B[1], B[2], 0, 1, vertdata); //Back
	AddCubeVertex(A[0], A[1], A[2], 1, 1, vertdata);
	AddCubeVertex(D[0], D[1], D[2], 1, 0, vertdata);
	AddCubeVertex(D[0], D[1], D[2], 1, 0, vertdata);
	AddCubeVertex(C[0], C[1], C[2], 0, 0, vertdata);
	AddCubeVertex(B[0], B[1], B[2], 0, 1, vertdata);

	AddCubeVertex(H[0], H[1], H[2], 0, 1, vertdata); //Top
	AddCubeVertex(G[0], G[1], G[2], 1, 1, vertdata);
	AddCubeVertex(C[0], C[1], C[2], 1, 0, vertdata);
	AddCubeVertex(C[0], C[1], C[2], 1, 0, vertdata);
	AddCubeVertex(D[0], D[1], D[2], 0, 0, vertdata);
	AddCubeVertex(H[0], H[1], H[2], 0, 1, vertdata);

	AddCubeVertex(A[0], A[1], A[2], 0, 1, vertdata); //Bottom
	AddCubeVertex(B[0], B[1], B[2], 1, 1, vertdata);
	AddCubeVertex(F[0], F[1], F[2], 1, 0, vertdata);
	AddCubeVertex(F[0], F[1], F[2], 1, 0, vertdata);
	AddCubeVertex(E[0], E[1], E[2], 0, 0, vertdata);
	AddCubeVertex(A[0], A[1], A[2], 0, 1, vertdata);

	AddCubeVertex(A[0], A[1], A[2], 0, 1, vertdata); //Left
	AddCubeVertex(E[0], E[1], E[2], 1, 1, vertdata);
	AddCubeVertex(H[0], H[1], H[2], 1, 0, vertdata);
	AddCubeVertex(H[0], H[1], H[2], 1, 0, vertdata);
	AddCubeVertex(D[0], D[1], D[2], 0, 0, vertdata);
	AddCubeVertex(A[0], A[1], A[2], 0, 1, vertdata);

	AddCubeVertex(F[0], F[1], F[2], 0, 1, vertdata); //Right
	AddCubeVertex(B[0], B[1], B[2], 1, 1, vertdata);
	AddCubeVertex(C[0], C[1], C[2], 1, 0, vertdata);
	AddCubeVertex(C[0], C[1], C[2], 1, 0, vertdata);
	AddCubeVertex(G[0], G[1], G[2], 0, 0, vertdata);
	AddCubeVertex(F[0], F[1], F[2], 0, 1, vertdata);
}

void CMainApplication::SetupCameras() {
	cout << "  setup cameras" << endl;
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc) {
	cout << "  CreateFrameBuffer" << endl;
	/*glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);
	
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
	return true;
}

bool CMainApplication::SetupStereoRenderTargets() {
	cout << "  setup render targets" << endl;
	if (!m_pHMD) return false;

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);
	cout << "  HMD recomended render size: " << m_nRenderWidth << " " << m_nRenderHeight << endl;
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);
	return true;
}

void CMainApplication::RenderStereoTargets() {
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	/*glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

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

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);*/
}

void drawSide(float x, float y, float z, float ux, float uy, float uz, float vx, float vy, float vz) {
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);  glVertex3f(x - ux - vx, y - uy - vy, z - uz - vz);
	glTexCoord2f(0.0, 1.0);  glVertex3f(x - ux + vx, y - uy + vy, z - uz + vz);
	glTexCoord2f(1.0, 1.0);  glVertex3f(x + ux + vx, y + uy + vy, z + uz + vz);
	glTexCoord2f(1.0, 0.0);  glVertex3f(x + ux - vx, y + uy - vy, z + uz - vz);
	glEnd();
}

void drawCube(float x, float y, float z) {
	float s = 0.2;
	drawSide(x - s, y, z, 0, 0, s, 0, s, 0);
	drawSide(x + s, y, z, 0, 0, -s, 0, s, 0);
	drawSide(x, y - s, z, 0, 0, s, s, 0, 0);
	drawSide(x, y + s, z, 0, 0, -s, -s, 0, 0);
	drawSide(x, y, z - s, s, 0, 0, 0, s, 0);
	drawSide(x, y, z + s, -s, 0, 0, 0, s, 0);
}

void CMainApplication::RenderScene(vr::Hmd_Eye nEye) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (m_bShowCubes) {
		glMultMatrixf(GetCurrentViewProjectionMatrix(nEye).getValues());

		glBindTexture(GL_TEXTURE_2D, textureID);
		glEnable(GL_TEXTURE_2D);

		for (int i = -5; i < 5; i++) {
			for (int j = -5; j < 5; j++) {
				for (int k = -5; k < 5; k++) {
					drawCube(i, j, k);
				}
			}
		}

		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glPopMatrix();
}

void CMainApplication::RenderCompanionWindow() {
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	glBindTexture(GL_TEXTURE_2D, textureID);
	//glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);  glVertex3f(-1, -1, 0.0);
	glTexCoord2f(0.0, 1.0);  glVertex3f(-1,  1, 0.0);
	glTexCoord2f(1.0, 1.0);  glVertex3f( 0,  1, 0.0);
	glTexCoord2f(1.0, 0.0);  glVertex3f( 0, -1, 0.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textureID);
	//glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
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
	if (!m_pHMD)
		return Matrix();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

Matrix CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye) {
	if (!m_pHMD)
		return Matrix();

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

	pMainApplication->RunMainLoop();
	cout << " shutdown" << endl;
	pMainApplication->Shutdown();
}