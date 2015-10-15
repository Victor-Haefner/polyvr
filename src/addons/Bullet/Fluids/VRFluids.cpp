#include "VRFluids.h"
#include "../Particles/VRParticle.h"
#include "../Particles/VRParticlesT.h"

#include <cmath>

using namespace OSG;


VRFluids::VRFluids() : VRFluids(true) {}

VRFluids::VRFluids(bool spawnParticles) : VRParticles(false) {
    if (spawnParticles) resetParticles<SphParticle>();
}

shared_ptr<VRFluids> VRFluids::create() {
    return shared_ptr<VRFluids>( new VRFluids() );
}

void VRFluids::update(int from, int to) {
    if (to < 0) to = N;
    {
        BLock lock(mtx());
        for (int i=from; i < to; i++) {
            auto p = particles[i]->body->getWorldTransform().getOrigin();
            pos->setValue(toVec3f(p),i);
        }
    }
    setPositions(pos);
    updateForce(from, to);
}

const float CHAINING = 0.4; // NOTE binding strength between particles (XSPH)
inline void VRFluids::updateForce(int from, int to) {
    SphParticle* p;
    SphParticle* neighbour;
    btVector3 p_origin(0,0,0);
    btVector3 n_origin(0,0,0);
    btVector3 force(0,0,0);

    {
        BLock lock(mtx());
        // calc all densities
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            p_origin = p->body->getWorldTransform().getOrigin();
            p->sphDensity = 0;

            for (int j=from; j < to; j++) {
                neighbour = (SphParticle*) particles[j];
                n_origin = neighbour->body->getWorldTransform().getOrigin();
                p->sphDensity += calc_density(neighbour, p_origin.distance2(n_origin), p->sphArea);
            }
        }
        // calc movement
        for (int i=from; i < to; i++) {
            force.setZero();
            p = (SphParticle*) particles[i];
            p_origin = p->body->getWorldTransform().getOrigin();
            for (int j=from; j < to; j++) {
                neighbour = (SphParticle*) particles[j];
                n_origin = neighbour->body->getWorldTransform().getOrigin();
                force += xsph_calc_movement(p, neighbour);
            }
            force = p->body->getLinearVelocity() + CHAINING * force;
            p->body->setLinearVelocity(force);
            // use normal and color to hand over force and particle size to shaders
            Vec4f color(0,0,0, (*p).sphArea);
            Vec3f normal(force.x(), force.y(), force.z());
            normals->setValue(normal, i);
            colors->setValue(color, i);
        }
        setNormals(normals);
        setColors(colors);
    }
}

// NOTE abgeleitet von der Grundgleichung für die Eigenschaften von SPH-Partikeln
inline float VRFluids::calc_density(SphParticle* n, float distance2, float area) {
    return kernel_poly6(distance2, area) * n->mass;
}

/** XSPH */
inline btVector3 VRFluids::xsph_calc_movement(SphParticle* p, SphParticle* n) {
    float pAverage = 0.5 * (p->sphDensity + n->sphDensity);

    btVector3 p_origin = p->body->getWorldTransform().getOrigin();
    float distance2 = p_origin.distance2(n->body->getWorldTransform().getOrigin());

    btVector3 vDiff = n->body->getLinearVelocity() - p->body->getLinearVelocity();
    return kernel_poly6(distance2, p->sphArea) * n->mass * (vDiff/pAverage);
}

/** Kernel for density (poly6)
 * @inproceedings{muller2003particle,
 *  title={Particle-based fluid simulation for interactive applications},
 *  author={M{\"u}ller, Matthias and Charypar, David and Gross, Markus},
 *  booktitle={Proceedings of the 2003 ACM SIGGRAPH/Eurographics symposium on Computer animation},
 *  pages={154--159},
 *  year={2003},
 *  organization={Eurographics Association}
 * }
 */
inline float VRFluids::kernel_poly6(float distance2, float area) {
    float diff = area*area - distance2;
    if (diff > 0) {
        return (315.0 / (64.0*Pi)) * (1/pow(area,9)) * pow(diff, 3);
    }
    return 0.0;
}

/** Kernel for pressure */
inline float VRFluids::kernel_spiky(float distance, float area)
{
    float diff = area - distance;
    if (diff > 0) {
        return (15 / (Pi * pow(area,6))) * pow(diff,3);
    }
    return 0.0;
}

/** Kernel for viscosity */
inline float VRFluids::kernel_visc(float distance, float area)
{
    float diff = area - distance;
    if (diff > 0) {
        return (45 / (Pi * pow(area,6))) * diff;
    }
    return 0.0;
}
