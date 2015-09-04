#ifndef VRFLUIDS_H_INCLUDED
#define VRFLUIDS_H_INCLUDED

#include "../Particles/VRParticles.h"

OSG_BEGIN_NAMESPACE;

class VRFluids : public VRParticles {
    public:
        VRFluids();
        VRFluids(bool spawnParticles);
};

OSG_END_NAMESPACE;

#endif // VRFLUIDS_H_INCLUDED
