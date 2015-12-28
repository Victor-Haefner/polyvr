#include "VRFluids.h"
#include "../Particles/VRParticle.h"
#include "../Particles/VRParticlesT.h"

#include <cmath> // pow(), etc. needed for kernels
#include <omp.h> // openMP for parallelization

using namespace OSG;


VRFluids::VRFluids() : VRFluids(true) {}

VRFluids::VRFluids(bool spawnParticles) : VRParticles(false) {
    if (spawnParticles) resetParticles<SphParticle>();
}

VRFluids::~VRFluids() {
    VRScenePtr scene = VRSceneManager::getCurrent();
    if (scene) scene->dropPhysicsUpdateFunction(fluidFkt.get(), this->afterBullet);
}

shared_ptr<VRFluids> VRFluids::create() {
    return shared_ptr<VRFluids>( new VRFluids() );
}

void VRFluids::setFunctions(int from, int to) {
    this->from = from;
    this->to = to;
    {
        BLock lock(mtx());
        VRScenePtr scene = VRSceneManager::getCurrent();
        if (!scene) {
            printf("VRFluids::setFunctions(): No scene found");
            return;
        }
        // enable graphical updates
        scene->dropUpdateFkt(fkt);
        fkt = VRFunction<int>::create("particles_update", boost::bind(&VRParticles::update, this,from,to));
        scene->addUpdateFkt(fkt);
        // enable physic updates
        scene->dropPhysicsUpdateFunction(fluidFkt.get(), this->afterBullet);
        if (this->simulation == SPH) {
            fluidFkt = VRFunction<int>::create("sph_update", boost::bind(&VRFluids::updateSPH, this,from,to));
        } else if (this->simulation == XSPH) {
            fluidFkt = VRFunction<int>::create("xsph_update", boost::bind(&VRFluids::updateXSPH, this,from,to));
        }
        scene->addPhysicsUpdateFunction(fluidFkt.get(), this->afterBullet);
    }
}

void VRFluids::update(int from, int to) {
    if (to < 0) to = N;
    {
        BLock lock(mtx());
        for (int i=from; i < to; i++) {
            auto p = particles[i]->body->getWorldTransform().getOrigin();
            pos->setValue(toVec3f(p),i);

            auto particle = (SphParticle*)particles[i];
            auto d = particle->sphDensity / 2 * this->PRESSURE_REST_DENS;
            if (d > 2.0) {
                colors->setValue(Vec4f(0,0,0,1), i); // way too big -> black
            } else if (d > 1.0) {
                colors->setValue(Vec4f(0,0,1,1), i); // too big -> blue
            } else {
                colors->setValue(Vec4f(d,1-d,0,1), i); // nice range -> green<->red
            }
        }
    }
}

inline void VRFluids::updateSPH(int from, int to) {
    {
        SphParticle* p;
        BLock lock(mtx());

        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            sph_calc_density_pressure(p, from, to);
            // if (p->sphDensity > 1*this->PRESSURE_REST_DENS) {
            //     p->sphDensity = 1*this->PRESSURE_REST_DENS; // NOTE pressure limiter
            // }
        }

        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            sph_calc_pressureForce(p, from, to);
            sph_calc_viscosityForce(p, from, to);
            // update Particle Acceleration:
            btVector3 force = (p->sphPressureForce + (p->sphDensity * p->body->getLinearVelocity()) + p->sphViscosityForce);
            // btVector3 force = (p->sphPressureForce +  p->body->getLinearVelocity() + p->sphViscosityForce);
            p->body->setLinearVelocity(force); // NOTE works kind of...
            //p->body->applyCentralForce(force / p->sphDensity); // NOTE very weird...
            //p->body->applyCentralImpulse(force / p->sphDensity);

            btVector3 pf = p->sphPressureForce; // TODO DEBUG
            btVector3 v = p->body->getLinearVelocity();
            btVector3 vis = p->sphViscosityForce;
            printf("--> (%f,%f,%f) + (%f * (%f,%f,%f)) + (%f,%f,%f)\n",
                    pf[0], pf[1], pf[2], p->sphDensity, v[0], v[1], v[2], vis[0],vis[1],vis[2]);
        }

        // TODO debug foo here
        //int num = (rand() % this->to); // NOTE nimm immer neue stichprobe
        //int num = 42; // NOTE nimm ein bestimmtes partikel
        //p = (SphParticle*) particles[num];
        // btVector3 pf = p->sphPressureForce;
        // btVector3 v = p->body->getLinearVelocity();
        // btVector3 vis = p->sphViscosityForce;
        // if (p->sphDensity > 1.0) {
        //     printf("--> (%f,%f,%f) + (%f * (%f,%f,%f)) + (%f,%f,%f)\n",
        //             pf[0], pf[1], pf[2], p->sphDensity, v[0], v[1], v[2], vis[0],vis[1],vis[2]);
        // }
    }
}

const float XSPH_CHAINING = 0.3; // NOTE binding strength between particles (XSPH)
inline void VRFluids::updateXSPH(int from, int to) {
    SphParticle* p;
    SphParticle* n;
    btVector3 p_origin(0,0,0);
    btVector3 n_origin(0,0,0);
    btVector3 force(0,0,0);

    {
        BLock lock(mtx());
        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            sph_calc_density_pressure(p, from, to);
        }

        //#pragma omp parallel for private(p, n) shared(from, to)
        for (int i=from; i < to; i++) {
            xsph_calc_movement(p, from, to);
            p->body->setLinearVelocity(p->sphPressureForce);
            // simulation done.
            // use normal and color to hand over force and particle size to shaders
            Vec4f color(0,0,0, (*p).sphArea);
            Vec3f normal(force.x(), force.y(), force.z());
            normals->setValue(normal, i);
            colors->setValue(color, i);
        }
    }
}

inline void VRFluids::sph_calc_density_pressure(SphParticle* p, int from, int to) {
    p->sphDensity = 0.000000001;
    btVector3 p_origin = p->body->getWorldTransform().getOrigin();

    for (int i=from; i < to; i++) {
            btVector3 n_origin = particles[i]->body->getWorldTransform().getOrigin();
            float kernel = kernel_poly6(n_origin - p_origin, p->sphArea);
            p->sphDensity += particles[i]->mass * kernel;
    }
    p->sphPressure = PRESSURE_KAPPA * (p->sphDensity - REST_DENSITY);
}

inline void VRFluids::sph_calc_pressureForce(SphParticle* p, int from, int to) {
    p->sphPressureForce.setZero();
    btVector3 p_origin = p->body->getWorldTransform().getOrigin();

    for (int i=from; i < to; i++) {
            SphParticle* n = (SphParticle*) particles[i];
            btVector3 n_origin = n->body->getWorldTransform().getOrigin();
            float trick = (p->sphPressure + n->sphPressure) / (2 * n->sphDensity); // makes forces symmetric
            //printf("-->Force(%f), Mass(%f), Pressure(%f), Kernel(%f)\n", p->sphPressureForce[1], p->mass, trick, kernel[1]); // TODO debug
            btVector3 kernel = kernel_spiky_gradient(n_origin - p_origin, p->sphArea);
            p->sphPressureForce -= n->mass * trick * kernel;
            //printf("-->(%f, %f, %f)\n", n->mass, trick, (kernel_spiky_gradient(n_origin - p_origin, p->sphArea))[2]); // TODO debug
            //printf("-->(%f)\n", (n->mass * trick * kernel_spiky_gradient(n_origin - p_origin, p->sphArea))[2]); // TODO debug
    }
    //printf("--> PressureF: (%f,%f,%f)\n", p->sphPressureForce[0],p->sphPressureForce[1],p->sphPressureForce[2]);
}

inline void VRFluids::sph_calc_viscosityForce(SphParticle* p, int from, int to) {
    p->sphViscosityForce.setZero();
    btVector3 p_origin = p->body->getWorldTransform().getOrigin();
    btVector3 p_speed = p->body->getLinearVelocity();

    for (int i=from; i < to; i++) {
        if (p != particles[i]) {
            SphParticle* n = (SphParticle*) particles[i];
            btVector3 n_origin = n->body->getWorldTransform().getOrigin();
            btVector3 n_speed = n->body->getLinearVelocity();
            n_speed = (p_speed - n_speed) / n->sphDensity;
            p->sphViscosityForce += n->mass * n_speed * kernel_visc_laplacian(n_origin - p_origin, p->sphArea);
        }
    }
    p->sphViscosityForce *= VISCOSITY_MU;
}

/** XSPH */
inline btVector3 VRFluids::xsph_calc_movement(SphParticle* p, int from, int to) {
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
    float diff = h*h - r2;
    if (diff >= 0) {
        return (315.0 / (64.0*Pi)) * (1/pow(h,9)) * diff*diff*diff;
    }
    return 0.0;
}

/** Kernel for pressure */
inline float VRFluids::kernel_spiky(btVector3 v, float h) {
    float r = v.length();
    float diff = h - r;
    if (diff > 0) {
        return (15.0 / (Pi * pow(h,6))) * diff*diff*diff;
    }
    return 0.0;
}

/** Kernel for pressure */
inline btVector3 VRFluids::kernel_spiky_gradient(btVector3 v, float h) {
    float r = v.length();
    //if (std::isnan(r) || r <= 0) r = 0.0001;
    float diff = h - r;
    if (diff > 0 && r > 0) {
        return (45.0 / (Pi * pow(h,6))) * (v/r) * diff*diff;
    }
    return btVector3(0,0,0);
}

/** Kernel for viscosity */
inline float VRFluids::kernel_visc(btVector3 v, float h) {
    float r = v.length();
    float diff = h - r;
    if (diff > 0) {
        float h2 = h*h;
        float h3 = h2*h;
        float r2 = r*r;
        float r3 = r2*r;
        float a = -1*r3 / (h3+h3);
        float b = r2 / h2;
        float c = h / (r+r);
        return (15.0 / (2*Pi*h3)) * (a + b + c - 1);
    }
    return 0.0;
}

/**
 *  Kernel_visc laplacian function for viscosity
 *  Source: mueller
 */
inline float VRFluids::kernel_visc_laplacian(btVector3 v, float h) {
    float r = v.length();
    float diff = h - r;
    if (diff > 0) {
        return (45.0 / (Pi * pow(h,6))) * diff;
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
