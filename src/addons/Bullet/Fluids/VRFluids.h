#ifndef VRFLUIDS_H_INCLUDED
#define VRFLUIDS_H_INCLUDED

#include "../Particles/VRParticles.h"

OSG_BEGIN_NAMESPACE;

class VRFluids : public VRParticles {

    public:
        enum SimulationType { SPH, XSPH };

        VRFluids();
        VRFluids(bool spawnParticles);
        ~VRFluids();
        static shared_ptr<VRFluids> create();

        void update(int from, int to) override;
        void updateSPH(int from, int to);
        void updateXSPH(int from, int to);

        void setSimulation(SimulationType t, bool forceChange=false);
        void setSphRadius(float newRadius, float variation);

    protected:
        VRUpdatePtr fluidFkt;
        SimulationType simulation = SPH;

        const bool afterBullet = false;
        const float PRESSURE_KAPPA = /*N*/ /*8.4*/ 1.38 * 296.0 * 0.0000001; // ideal gas law: N*(gas const)*(temperature)
        // const float PRESSURE_KAPPA = .0000001; // NOTE from some web source
        const float REST_DENSITY  = 4.0; // density where particles should rest
        const float VISCOSITY_MU   = 0.1;
        const float Pi = 3.14159;

        inline void xsph_calc_movement(SphParticle* p, int from, int to);

        inline float kernel_poly6(btVector3 distance_vector, float area);
        inline float kernel_spiky(btVector3 distance_vector, float area);
        inline btVector3 kernel_spiky_gradient(btVector3 distance_vector, float h);
        inline float kernel_visc(btVector3 distance_vector, float area);
        inline float kernel_visc_laplacian(btVector3 distance_vector, float area);

        inline void sph_calc_density_pressure(SphParticle* p, int from, int to);
        inline void sph_calc_pressureForce(SphParticle* p, int from, int to);
        inline void sph_calc_viscosityForce(SphParticle* p, int from, int to);

        void setFunctions(int from, int to) override;
};

OSG_END_NAMESPACE;

#endif // VRFLUIDS_H_INCLUDED
