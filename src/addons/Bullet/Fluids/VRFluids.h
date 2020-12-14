#ifndef VRFLUIDS_H_INCLUDED
#define VRFLUIDS_H_INCLUDED

#include "../Particles/VRParticles.h"

OSG_BEGIN_NAMESPACE;

class VRFluids : public VRParticles {

    public:
        enum SimulationType { SPH, XSPH };

        VRFluids(string name, bool spawnParticles = true);
        ~VRFluids();
        static shared_ptr<VRFluids> create(string name = "fluid");

        void updateParticles(int from, int to) override;
        void updateSPH(int from, int to);
        void updateXSPH(int from, int to);

        void setSimulation(SimulationType t, bool forceChange=false);
        void setSphRadius(float newRadius);
        void setViscosity(float factor);
        void setMass(float newMass, float variation=0.0) override;
        void setRestDensity(float density);
        void setRestDensity(int rN, float rDIS);


    protected:
        VRUpdateCbPtr fluidFkt;
        SimulationType simulation = SPH;

        /* Calculate after bullets physics cycle? */
        const bool afterBullet = false;
        /*
         * Pressure multiplier, derived from ideal gas law
         * P = N*(gas const)*(temperature) * (1/V)
         */
        float PRESSURE_KAPPA = 0; // just some init value, see updateDerivedValues()
        /* Number of particles around a resting particle */
        int REST_N = 1;
        /* Average distance of particles around resting particle */
        float REST_DIS = 0.7;
        /*
         * Density where particles should rest.
         * (re-)calculate using updateDerivedValues();
         */
        float REST_DENSITY  = 0; // just some init value, see updateDerivedValues()
        /* Simple viscosity multiplier */
        float VISCOSITY_MU   = 0.01;
        /* The number Pi given precisely to five decimal places */
        const float Pi = 3.14159;
        /* The average sph radius of a particle. */
        float sphRadius = 1;
        /* The average mass of a particle. */
        float particleMass = 1;
        /* The average volume of a particle */
        float particleVolume = 1;

        inline void xsph_calc_movement(SphParticle* p, int from, int to);

        inline float kernel_poly6(btVector3 distance_vector, float area) /*__attribute__((always_inline))*/;
        inline float kernel_spiky(btVector3 distance_vector, float area) /*__attribute__((always_inline))*/;
        inline btVector3 kernel_spiky_gradient(btVector3 distance_vector, float h) /*__attribute__((always_inline))*/;
        inline float kernel_visc(btVector3 distance_vector, float area) /*__attribute__((always_inline))*/;
        inline float kernel_visc_laplacian(btVector3 distance_vector, float area) /*__attribute__((always_inline))*/;

        inline void sph_calc_properties(SphParticle* p) /*__attribute__((always_inline))*/;
        inline void sph_calc_forces(SphParticle* p) /*__attribute__((always_inline))*/;

        void setFunctions(int from, int to) override;
        void disableFunctions() override;
        void updateDerivedValues();
};

typedef shared_ptr<VRFluids> VRFluidsPtr;

OSG_END_NAMESPACE;

#endif // VRFLUIDS_H_INCLUDED
