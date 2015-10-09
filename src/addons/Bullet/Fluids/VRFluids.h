#ifndef VRFLUIDS_H_INCLUDED
#define VRFLUIDS_H_INCLUDED

#include "../Particles/VRParticles.h"

OSG_BEGIN_NAMESPACE;

class VRFluids : public VRParticles {
protected:
    inline float calc_density(SphParticle* n, float distance2, float area);
    inline btVector3 xsph_calc_movement(SphParticle* p, SphParticle* n);
    inline float kernel_poly6(float distance2, float area);
    inline float kernel_spiky(float distance, float area);
    inline float kernel_visc(float distance, float area);

public:
    VRFluids();
    VRFluids(bool spawnParticles);

    void update(int from, int to) override;
    void updateForce(int from, int to);
};

OSG_END_NAMESPACE;

#endif // VRFLUIDS_H_INCLUDED
