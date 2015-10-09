#include "VRFluids.h"
#include "../Particles/VRParticle.h"
#include "../Particles/VRParticlesT.h"


using namespace OSG;


VRFluids::VRFluids() : VRFluids(true) {}

VRFluids::VRFluids(bool spawnParticles) : VRParticles(false) {
    if (spawnParticles) resetParticles<SphParticle>();
}
