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

void VRFluids::setFunctions(int from, int to) {
    this->from = from;
    this->to = to;
    VRScenePtr scene = VRSceneManager::getCurrent();
    {
        BLock lock(mtx());
        // enable graphical updates
        scene->dropUpdateFkt(fkt);
        fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::update, this,from,to));
        scene->addUpdateFkt(fkt);
        // enable physic updates
        scene->dropPhysicsUpdateFunction(fluidFkt.get(), false);
        if (this->simulation == SPH) {
            fluidFkt = VRFunction<int>::create("sph_update", boost::bind(&VRFluids::updateXSPH, this,from,to));
        } else if (this->simulation == XSPH) {
            fluidFkt = VRFunction<int>::create("xsph_update", boost::bind(&VRFluids::updateSPH, this,from,to));
        }
        scene->addPhysicsUpdateFunction(fluidFkt.get(), false);
    }
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
    updateXSPH(from, to);
}

inline void VRFluids::updateSPH(int from, int to) {
    // TODO implement
}

const float CHAINING = 0.8; // NOTE binding strength between particles (XSPH)
inline void VRFluids::updateXSPH(int from, int to) {
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
    return kernel_visc(distance2, area) * n->mass;
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
inline float VRFluids::kernel_poly6(float r2, float h) {
    float diff = h*h - r2;
    if (diff > 0) {
        return (315.0 / (64.0*Pi)) * (1/pow(h,9)) * pow(diff, 3);
    }
    return 0.0;
}

/** Kernel for pressure */
inline float VRFluids::kernel_spiky(float r, float h) {
    float diff = h - r;
    if (diff > 0) {
        return (15 / (Pi * pow(h,6))) * pow(diff,3);
    }
    return 0.0;
}

/** Kernel for viscosity */
inline float VRFluids::kernel_visc(float r, float h) {
    float diff = h - r;
    if (diff > 0) {
        float a = (r*r*r) / (2*h*h*h);
        float b = (r*r)/(h*h);
        float c = h / (r+r);
        return (15 / (2*Pi*h*h*h)) * (-1*a + b + c - 1);
    }
    return 0.0;
}

/** Kernel_visc laplacian function for viscosity */
inline float VRFluids::kernel_visc_laplacian(float r, float h) {
    float diff = h - r;
    if (diff > 0) {
        return (45 / (Pi * pow(h,6))) * diff;
    }
    return 0.0;
}

void VRFluids::setSimulation(SimulationType t, bool forceChange) {
    this->simulation = t;
    if (forceChange) this->setFunctions(this->from, this->to);
}

void VRFluids::setSphRadius(float newRadius, float variation) {
    int i;
    float result;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            result = newRadius;
            result += (2 * variation * float(rand()) / RAND_MAX );
            result -= variation;
            ((SphParticle*) particles[i])->sphArea = result;
        }
    }
}
