#include "VRFluids.h"
#include "../Particles/VRParticle.h"
#include "../Particles/VRParticlesT.h"

#include <cmath> // pow(), etc. needed for kernels
#include <omp.h> // openMP for parallelization

using namespace OSG;

VRFluids::VRFluids(string name, bool spawnParticles) : VRParticles(name, false) {
    this->collideWithSelf = false;
    if (spawnParticles) {
        resetParticles<SphParticle>();
        this->updateDerivedValues();
    }
}

VRFluids::~VRFluids() {
    VRScenePtr scene = VRScene::getCurrent();
    if (scene) scene->dropPhysicsUpdateFunction(fluidFkt, this->afterBullet);
}

shared_ptr<VRFluids> VRFluids::create(string name) {
    return shared_ptr<VRFluids>( new VRFluids(name) );
}

void VRFluids::setFunctions(int from, int to) {
    this->from = from;
    this->to = to;
    {
        BLock lock(mtx());
        VRScenePtr scene = VRScene::getCurrent();
        if (!scene) {
            printf("VRFluids::setFunctions(): No scene found\n");
            return;
        }
        // enable graphical updates
        scene->dropUpdateFkt(fkt);
        fkt = VRUpdateCb::create("particles_update", bind(&VRFluids::updateParticles, this,from,to));
        scene->addUpdateFkt(fkt);
        // enable physic updates
        scene->dropPhysicsUpdateFunction(fluidFkt, this->afterBullet);
        if (this->simulation == SPH) {
            fluidFkt = VRUpdateCb::create("sph_update", bind(&VRFluids::updateSPH, this,from,to));
        } else if (this->simulation == XSPH) {
            fluidFkt = VRUpdateCb::create("xsph_update", bind(&VRFluids::updateXSPH, this,from,to));
        }
        scene->addPhysicsUpdateFunction(fluidFkt, this->afterBullet);
    }
    printf("VRFluids::setFunctions(from=%i, to=%i)\n", from, to);
}

void VRFluids::disableFunctions() {
    {
        BLock lock(mtx());
        VRScenePtr scene = VRScene::getCurrent();
        scene->dropUpdateFkt(fkt);
        scene->dropPhysicsUpdateFunction(fluidFkt, this->afterBullet);
    }
}

void VRFluids::updateParticles(int from, int to) {
    if (to < 0) to = N;
    {
        BLock lock(mtx());
        for (int i=from; i < to; i++) {
            if (particles[i]->isActive) {
                auto p = particles[i]->body->getWorldTransform().getOrigin();
                pos->setValue(toVec3d(p),i);

                colors->setValue(Vec4d(0,0,1,1), i);
                /*
                auto particle = (SphParticle*)particles[i];
                //auto d = particle->sphDensity / this->REST_DENSITY; // visualize density
                auto d = particle->body->getLinearVelocity().length2() / (3*3); // visualize velocity
                // auto d = particle->sphPressureForce.length() + particle->sphViscosityForce.length() / 20.0;
                if (d > 1.0) {
                    colors->setValue(Vec4d(d-1,2-d,0,1), i); // high range -> green<->red
                } else if (d < 0.00001) {
                    colors->setValue(Vec4d(0,0,0,1), i); // zero or negative -> black
                } else {
                    colors->setValue(Vec4d(0,d,1-d,1), i); // low range -> blue<->green
                }
                */
            }
        }
    }
}

inline void VRFluids::updateSPH(int from, int to) {
    {
        SphParticle* p;
        BLock lock(mtx());

        // clear and fill octree
        ocparticles->clear();
        for (int i=from; i < to; i++) {
            if (particles[i]->isActive == true) {
                p = (SphParticle*) particles[i];
                btVector3 p_origin = p->body->getWorldTransform().getOrigin();
                ocparticles->add(Vec3d(p_origin[0],p_origin[1],p_origin[2]),p);
            }
        }

        #pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            if (particles[i]->isActive == true) {
                p = (SphParticle*) particles[i];
                sph_calc_properties(p);
            }
        }

        #pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            if (particles[i]->isActive == true) {
                p = (SphParticle*) particles[i];
                sph_calc_forces(p);
                btVector3 force = (p->sphPressureForce + p->sphViscosityForce);
                p->body->applyCentralForce(force);

                // btVector3 pf = p->sphPressureForce; // NOTE very ressource heavy debug foo here
                // btVector3 vis = p->sphViscosityForce;
                // printf("--> (%f,%f,%f) + (%f,%f,%f) << (%f <dens-press> %f), mass(%f)\n",
                //         pf[0], pf[1], pf[2], vis[0],vis[1],vis[2], p->sphDensity/REST_DENSITY, p->sphPressure, p->mass);
            }
        }

        // NOTE ressource heavy debug foo here
        // int num = (rand() % this->to); // NOTE nimm immer neue stichprobe
        // int num = 42; // NOTE nimm ein bestimmtes partikel
        // p = (SphParticle*) particles[num];
        // btVector3 pf = p->sphPressureForce;
        // btVector3 vis = p->sphViscosityForce;
        // printf("--> (%f,%f,%f) + (%f,%f,%f) << (%f <dens-press> %f), mass(%f)\n",
        //         pf[0], pf[1], pf[2], vis[0],vis[1],vis[2], p->sphDensity/REST_DENSITY, p->sphPressure, p->mass);
    }
}

const float XSPH_CHAINING = 0.3; // binding strength between particles (XSPH)
inline void VRFluids::updateXSPH(int from, int to) {
    SphParticle* p;
    btVector3 force(0,0,0);

    {
        BLock lock(mtx());
        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            sph_calc_properties(p);
        }

        //#pragma omp parallel for private(p, n) shared(from, to)
        for (int i=from; i < to; i++) {
            xsph_calc_movement(p, from, to);
            p->body->setLinearVelocity(p->sphPressureForce);
            // simulation done.
            // use normal and color to hand over force and particle size to shaders
            Vec4d color(0,0,0, (*p).sphArea);
            Vec3d normal(force.x(), force.y(), force.z());
            normals->setValue(normal, i);
            colors->setValue(color, i);
        }
    }
}

const float NOTHING2 = 0.01;
/**
 * Calculates density, pressure and neighbors and stores them in SphParticle p.
 */
inline void VRFluids::sph_calc_properties(SphParticle* p) {
    p->sphDensity = 0.0;
    btVector3 p_origin = p->body->getWorldTransform().getOrigin();
    //float p_speed = p->body->getLinearVelocity().length2();

    p->neighbors = ocparticles->radiusSearch(Vec3d(p_origin[0],p_origin[1],p_origin[2]),p->sphArea);
    auto it = p->neighbors.begin();

    while (it != p->neighbors.end()) {
        btVector3 n_origin = ((SphParticle*) *it)->body->getWorldTransform().getOrigin();
        float kernel = kernel_poly6(p_origin - n_origin, p->sphArea);
        if (kernel == 0.0) {
            it = p->neighbors.erase(it);
        } else {
            p->sphDensity += ((SphParticle*) *it)->mass * kernel;
            it++;
        }
    }
    p->sphPressure = PRESSURE_KAPPA * (p->sphDensity - REST_DENSITY);
}

inline void VRFluids::sph_calc_forces(SphParticle* p) {
    p->sphPressureForce.setZero();
    p->sphViscosityForce.setZero();
    btVector3 p_origin = p->body->getWorldTransform().getOrigin();
    btVector3 p_speed = p->body->getLinearVelocity();

    for (auto np : p->neighbors) {
        SphParticle* n = (SphParticle*) np;
        btVector3 n_origin = n->body->getWorldTransform().getOrigin();
        btVector3 n_speed = n->body->getLinearVelocity();
        // calc pressure force
        float ptrick = (p->sphPressure + n->sphPressure) / (2 * n->sphDensity); // makes forces symmetric
        btVector3 pKernel = kernel_spiky_gradient(p_origin - n_origin, p->sphArea);
        p->sphPressureForce -= n->mass * ptrick * pKernel;
        // calc viscosity force
        btVector3 vtrick = (n_speed - p_speed) / n->sphDensity;
        p->sphViscosityForce += n->mass * vtrick * kernel_visc_laplacian(p_origin - n_origin, p->sphArea);
    }
    p->sphViscosityForce *= VISCOSITY_MU;
}

/** XSPH */
inline void VRFluids::xsph_calc_movement(SphParticle* p, int from, int to) {
    p->sphPressureForce.setZero();
    for (int i = from; i < to; i++) {
        SphParticle* n = (SphParticle*) particles[i];
        btVector3 p_origin = p->body->getWorldTransform().getOrigin();
        btVector3 n_origin = n->body->getWorldTransform().getOrigin();

        float pressureAvg = 0.5 * (p->sphDensity + n->sphDensity);
        btVector3 vDiff = n->body->getLinearVelocity() - p->body->getLinearVelocity();
        p->sphPressureForce += kernel_poly6(n_origin - p_origin, p->sphArea) * n->mass * (vDiff/pressureAvg);
    }
    p->sphPressureForce *= XSPH_CHAINING;
    p->sphPressureForce += p->body->getLinearVelocity();
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
inline float VRFluids::kernel_poly6(btVector3 v, float h) {
    float r2 = v.length2();
    float h2 = h*h;
    if ( r2 <= h2 ) {
        float diff = h2 - r2;
        return (315.0 / (64.0*Pi)) * (1/pow(h,9)) * diff*diff*diff;
    } else {
        return 0.0;
    }
}

/** Kernel for pressure */
inline float VRFluids::kernel_spiky(btVector3 v, float h) {
    float r = v.length();
    float diff = h - r;
    if (diff > 0) {
        return (15.0 / (Pi * pow(h,6))) * diff*diff*diff;
    } else {
        return 0.0;
    }
}

/** Kernel for pressure */
inline btVector3 VRFluids::kernel_spiky_gradient(btVector3 v, float h) {
    float r = v.length();
    float diff = h - r;
    if (diff > 0 && r > 0) { // NOTE > should be >= but things won't work then for some reason
        return (-45.0 / (Pi * pow(h,6))) * (v/r) * diff*diff;
    } else {
        return btVector3(0,0,0);
    }
}

/** Kernel for viscosity */
inline float VRFluids::kernel_visc(btVector3 v, float h) {
    float r = v.length();
    if (r <= h) {
        float h2 = h*h;
        float h3 = h2*h;
        float r2 = r*r;
        float r3 = r2*r;
        float a = -1*r3 / (h3+h3);
        float b = r2 / h2;
        float c = h / (r+r);
        return (15.0 / (2*Pi*h3)) * (a + b + c - 1);
    } else {
        return 0.0;
    }
}

/**
 *  Kernel_visc laplacian function for viscosity
 *  Source: mueller
 */
inline float VRFluids::kernel_visc_laplacian(btVector3 v, float h) {
    float r = v.length();
    if (r <= h && r >= 0) {
        float diff = h - r;
        return (45.0 / (Pi * pow(h,6))) * diff;
    } else {
        return 0.0;
    }
}

void VRFluids::setSimulation(SimulationType t, bool forceChange) {
    this->simulation = t;
    if (forceChange) this->setFunctions(this->from, this->to);
}

void VRFluids::setSphRadius(float newRadius) {
    this->sphRadius = newRadius;
    this->updateDerivedValues();

    int i;
    {
        BLock lock(mtx());
        for (i=0; i<N; i++) {
            ((SphParticle*) particles[i])->sphArea = newRadius;
        }
    }
}

void VRFluids::setMass(float newMass, float variation) {
    this->particleMass = newMass;
    this->updateDerivedValues();
    VRParticles::setMass(newMass, variation); //updates also sph particles
}

void VRFluids::setViscosity(float factor) {
    this->VISCOSITY_MU = factor;
}

void VRFluids::setRestDensity(float density) {
    this->REST_DENSITY = density;
}

void VRFluids::setRestDensity(int rN, float rDIS) {
    this->REST_N = rN;
    this->REST_DIS = rDIS;
    this->updateDerivedValues();
}

void VRFluids::updateDerivedValues() {
    btVector3 avgDistance(this->sphRadius * REST_DIS, 0, 0);
    btVector3 selfDistance(0,0,0);
    float selfDensity = this->particleMass * kernel_poly6(selfDistance, this->sphRadius);
    this->REST_DENSITY = selfDensity + REST_N * this->particleMass * kernel_poly6(avgDistance, this->sphRadius);
    this->particleVolume = 2*this->sphRadius*2*this->sphRadius*2*this->sphRadius;
    this->PRESSURE_KAPPA = 8.314 * 296.0 * this->particleMass *0.00005;
    printf("SPH Radius: %f\n", this->sphRadius);
}
