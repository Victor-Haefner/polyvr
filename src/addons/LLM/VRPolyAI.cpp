#include "VRPolyAI.h"
#include "VRLLM.h"
#include "core/scene/VRScene.h"
#include "core/gui/VRGuiManager.h"

using namespace OSG;

string PolyVR_API_instructions = R"API(
You are an assistant integrated into PolyVR, a VR/VE development environment.
You should assume the identity of PolyAI and the point of view of the PolyVR system.
Help the user create, debug, and understand PolyVR scenes and Python scripts.
The provided vector store contains the PolyVR API documentation and GUI description.
Use that documentation when answering PolyVR questions.
Do not invent API methods that are not supported by the documentation.
Do not consider the files as being from the user, those files are provided by PolyVR over the OpenAI API.
)API";

VRPolyAI::VRPolyAI() {
    llm = VRLLM::create();
    llm->setInstructions(PolyVR_API_instructions);
    setupKnowledgeAssets();

    sceneChangedCb = VRDeviceCb::create("PolyAI_sceneChanged", bind(&VRPolyAI::on_scene_changed, this) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( sceneChangedCb );
}

VRPolyAI::~VRPolyAI() {}

VRPolyAIPtr VRPolyAI::create() { return VRPolyAIPtr( new VRPolyAI() ); }
VRPolyAIPtr VRPolyAI::ptr() { return static_pointer_cast<VRPolyAI>(shared_from_this()); }

bool VRPolyAI::checkKey(const string& key, string& what) { return llm->checkKey(key, what); }
void VRPolyAI::setApiKey(string s) { llm->setApiKey(s); }
void VRPolyAI::setModel(string s) { llm->setModel(s); }
void VRPolyAI::setWordWrap(int N) { llm->setWordWrap(N); }
void VRPolyAI::setMsgCallback(VRMessageCbPtr cb) { llm->setMsgCallback(cb); }
void VRPolyAI::sendPyAPI() { llm->sendKnowledgeAssets(); }
void VRPolyAI::sendRequest(string req, string conv, string effort) { llm->sendRequest(req, conv, effort); }

bool VRPolyAI::on_scene_changed() {
    setupKnowledgeAssets();
    return true;
}

void VRPolyAI::setupKnowledgeAssets() {
string PolyVR_GUI = R"API(
# PolyVR GUI layout
- Toolbar at the top
    - Buttons: New, Open, Save, Save.., Close, Exit, Profiler, Export, About
    - Checkbox "Fotomode"
    - Radiobuttons for GUI Theme: "Light", "Dark"
    - Buttons for GUI Fontsize: +, -, 1
- 3D view on the right, below the toolbar
- Toolbar related to 3d view below the 3d view
    - Combobox to change the active camera
    - Combobox with checkboxes to control active navigation systems
    - Combobox with checkboxes to control debug layers
        - Cameras
        - Lights
        - Pause window
        - Physics
        - Objects
        - Setup
        - Statistics
        - Stencil
    - Buttons: Fullscreen, See All
- Output consoles, below the 3D view and its toolbar
    - Tabs for the different output consoles
        - tabs: "Console", "Errors", "Syntax", "Search results", "Reasoning", "Tracking", "Collaboration"
    - A clear button that wipes all output consoles
    - A pause checkbox to toggle the auto scroll of the output consoles
- A panel below the main toolbar and left of the 3d view
    - Tabs: Apps, Setup, Scene
        - Apps Panel:
            - entry to filter displayed projects
            - Two tabs: Projects, Examples
                - Same for each tab, a list of PolyVR projects to run
                    - For each project entry: a button "run" and a button "run without scripts" hidden in a drop down "advanced"
        - Setup Panel:
            - configuration of hardware environments
            - TODO ;)
        - Scene Panel:
            - Tabs: Rendering, Scenegraph, Scripting, Network
            - TODO ;)
)API";

string PolyVR_API_modules = R"API(
Here follows a list of all modules and objects available to the user in the PolyVR scripting environment.

)API";

    auto scene = VRScene::getCurrent();
    if (scene) {
        for (auto mod : scene->getPyVRModules()) {
            PolyVR_API_modules += "# Module " + mod + "\n";
            for (auto tpe : scene->getPyVRTypes(mod)) {
                string dcr = scene->getPyVRDescription(mod, tpe);
                PolyVR_API_modules += " ## Class " + tpe + "\n";
                PolyVR_API_modules += "     " + dcr + "\n";
                for (auto mth : scene->getPyVRMethods(mod, tpe)) {
                    string mdoc = scene->getPyVRMethodDoc(mod, tpe, mth);
                    PolyVR_API_modules += " ### Method " + mth + "\n";
                    PolyVR_API_modules += "      " + mdoc + "\n";
                }
            }
        }
    }


string PolyVR_API = R"API(
PolyVR is a IDE for developing virtual environments.
It has a focus on engineering applications and includes many interfaces and simulation modules.
The application logic is created with Python scripts, shader can be written with GLSL, and UI elements with websites.

The API is provided as separate files:
    - PolyVR_GUI.txt contains a description of the GUI as presented to the user
    - PolyVR_API_modules.txt contains a list of all modules available to the user when scripting the application logic

Further examples are provided:
    - PolyVR_API_examples_scenegraph

# Scripting basics:

The user adds scripts with the UI, every script appears as a list entry in the GUI and its content can be edited in a integrated text editor.
Such a script is internally a python function object, the user can execute the function using a button or define trigger like
- on_scene_load, execute the script when scene is loaded
- on_timeout, entry for timeout N in ms, executes the script every N ms
- on_device, combobox with devices like mouse, keyboard, flystick, server1, then a signal key, then a signal state like pressed or released, executes the script if the device is firing a signal.

Every script starts with
    import VR

VR is the main module where everything else lives.
- Every module described in PolyVR_API_modules.txt
- Every script of the current PolyVR project.

Every new project starts with a default "init" script.

    import VR
    VR.scene = VR.Object('scene', 'light')

This creates a new object called scene and attaches it to the default light object.

)API";

string PolyVR_API_examples_scenegraph = R"API(
    import VR

    obj = VR.Object('newObject')
    VR.scene.addChild(obj)

    trans = VR.Transform('newTransform')
    VR.scene.addChild(trans)

    trans.setFrom([1,0,0])

    cube = VR.Geometry('cube')
    cube.setPrimitive('Box 0.3 0.3 0.3 4 4 4')
    cube.setTransform([0,1,0], [1,0,0], [0,0.5,0.5])
    trans.addChild(cube)
)API";

    string PolyVR_scriptGuide = R"API(
A script has a name, core and parameters.
The script is handled internally as a python function.
The name has to to be a valid python function name.
The core is technically the content of the python function.
Every line of the core needs to be indented one level.
The convention for all indentation is to use tabs, not spaces.
The core does not contain the function head, the function head is generated based on the script name.
To define Python classes in PolyVR just define it in a script core and return an instance directly from the script, use the script as a factory.
)API";

    string storeName = "PolyVR_PythonAPI";
    llm->addKnowledgeAsset("PolyVR_API", storeName, PolyVR_API);
    llm->addKnowledgeAsset("PolyVR_GUI", storeName, PolyVR_GUI);
    llm->addKnowledgeAsset("PolyVR_scriptGuide", storeName, PolyVR_scriptGuide);
    if (scene) llm->addKnowledgeAsset("PolyVR_API_modules", storeName, PolyVR_API_modules);
    llm->addKnowledgeAsset("PolyVR_API_examples_scenegraph", storeName, PolyVR_API_examples_scenegraph);

    string createScriptDoc = R"API(
Create a python script in PolyVR.
Pass the function name and function core, follow the guidelines in the file PolyVR_scriptGuide.
)API";

    string createScriptCoreDoc = R"API(
Python function body only.
Do NOT include a 'def ...' function declaration.
Every non-empty line MUST begin with exactly one tab character.
Use tabs for indentation, not spaces.
Example:
\tprint('hello')
)API";

    handleCreateScript = VRServerCb::create("onCreateScript", [this](map<string, string> params) { return on_tool_executed("create_script", params); } );
    handleExecScript   = VRServerCb::create("onExecScript",   [this](map<string, string> params) { return on_tool_executed("execute_script", params); } );
    llm->addTool("create_script", handleCreateScript, createScriptDoc);
    llm->addToolParameter("create_script", "name", "string", "Valid Python function identifier used as the script name");
    llm->addToolParameter("create_script", "core", "string", createScriptCoreDoc);
    llm->addTool("execute_script", handleExecScript, "Execute a python script in PolyVR, pass the function name");
    llm->addToolParameter("execute_script", "name", "string", "Script name to execute");
}

string VRPolyAI::on_tool_executed(string name, map<string, string> params) {
    cout << "executed tool: " << name << ", params: " << toString(params) << endl;

    if (name == "create_script") {
        string name = params["name"];
        string core = params["core"];

        auto scene = VRScene::getCurrent();
        scene->newScript(name, core);
        uiSignal("scripts_update_list");
        return "Script added successfully";
    }

    if (name == "execute_script") {
        string name = params["name"];

        auto scene = VRScene::getCurrent();
        scene->triggerScript(name);
        return "Script exexcuted successfully";
    }

    return "Error: unknown tool";
}






