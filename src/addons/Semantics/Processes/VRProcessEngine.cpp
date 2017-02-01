#include "VRProcessEngine.h"

using namespace OSG;

VRProcessEngine::VRProcessEngine() {}
VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; }
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {}
void VRProcessEngine::run(float speed) {}
void VRProcessEngine::pause() {}
