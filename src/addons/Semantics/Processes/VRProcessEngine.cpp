#include "VRProcessEngine.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const VRProcessEnginePtr& o) { return "ProcessEngine"; }

VRProcessEngine::VRProcessEngine() {}
VRProcessEngine::~VRProcessEngine() {}

VRProcessEnginePtr VRProcessEngine::create() { return VRProcessEnginePtr( new VRProcessEngine() ); }

void VRProcessEngine::setProcess(VRProcessPtr p) { process = p; }
VRProcessPtr VRProcessEngine::getProcess() { return process; }

void VRProcessEngine::reset() {}
void VRProcessEngine::run(float speed) {}
void VRProcessEngine::pause() {}
