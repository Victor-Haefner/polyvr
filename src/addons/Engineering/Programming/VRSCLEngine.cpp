#include "VRSCLEngine.h"
#include "core/utils/system/VRSystem.h"

using namespace OSG;

VRSCLScript::VRSCLScript() {}
VRSCLScript::~VRSCLScript() {}

VRSCLScriptPtr VRSCLScript::create() { return VRSCLScriptPtr( new VRSCLScript() ); }
VRSCLScriptPtr VRSCLScript::ptr() { return static_pointer_cast<VRSCLScript>(shared_from_this()); }

string VRSCLScript::getScl() { return scl; }
string VRSCLScript::getPython() { return python; }

void VRSCLScript::readSCL(string path) {
    scl = readFileContent(path, false);
}

void VRSCLScript::convert() {
    ;
}




VRSCLEngine::VRSCLEngine() {}
VRSCLEngine::~VRSCLEngine() {}

VRSCLEnginePtr VRSCLEngine::create() { return VRSCLEnginePtr( new VRSCLEngine() ); }
VRSCLEnginePtr VRSCLEngine::ptr() { return static_pointer_cast<VRSCLEngine>(shared_from_this()); }

void VRSCLEngine::setElectricEngine(VRElectricSystemPtr e) { elSystem = e; }
VRSCLScriptPtr VRSCLEngine::getScript(string name) { return scripts.count(name) ? scripts[name] : 0; }

VRSCLScriptPtr VRSCLEngine::readSCL(string name, string path) { // TODO
    auto script = VRSCLScript::create();
    script->readSCL(path);
    script->convert();
    scripts[name] = script;
    return script;
}

void VRSCLEngine::iterate() { // TODO
    ;
}
