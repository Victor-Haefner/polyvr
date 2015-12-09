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
        }
    }
    setPositions(pos);

    // TODO DEBUG
    {
        //BLock lock(mtx());
        //btVector3 v = ((SphParticle*) particles[0])->sphPressureForce;
        //printf("--> P0: %f,%f,%f", v.x(),v.y(),v.z());
    }
}

inline void VRFluids::updateSPH(int from, int to) {
    {
        SphParticle* p;
        BLock lock(mtx());

        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            sph_calc_density(p, p->body->getWorldTransform().getOrigin(), from, to);
        }
        //printf("-->Density[0]: %f\n", ((SphParticle*) particles[0])->sphDensity); // TODO debug

        //#pragma omp parallel for private(p) shared(from, to)
        for (int i=from; i < to; i++) {
            p = (SphParticle*) particles[i];
            //if (p->sphDensity > 0.0001) // TODO how to handle zero density?
            {
                sph_calc_pressure(p, p->body->getWorldTransform().getOrigin(), from, to);
                //sph_calc_viscosity(p, p->body->getWorldTransform().getOrigin(), from, to);
                // update Particle Acceleration:
                btVector3 force = (p->sphPressureForce + (p->sphDensity * p->body->getTotalForce()) + p->sphViscosityForce);
                p->body->setLinearVelocity(btVector3(0,0,0));
                p->body->applyCentralForce(force / p->sphDensity); // TODO this does not work with Impulse
                //if (i==0) printf("-->Force[0]: (%f,%f,%f)\n", force[0],force[1],force[2]); // TODO debug
                btVector3 pos = p->body->getWorldTransform().getOrigin();
                //if (i==0) printf("-->Pos[0]: (%f,%f,%f)\n", pos[0],pos[1],pos[2]);
            }
        }

        // TODO debug foo here
        p = (SphParticle*) particles[42];
        btVector3 pf = p->sphPressureForce;
        btVector3 v = p->body->getTotalForce();
        btVector3 vis = p->sphViscosityForce;

        printf("force= (%f,%f,%f) + (%f * (%f,%f,%f)) + (%f,%f,%f)\n",
                pf[0], pf[1], pf[2], p->sphDensity, v[0], v[1], v[2], vis[0],vis[1],vis[2]);
    }
}

const float XSPH_CHAINING = 0.3; // NOTE binding strength between particles (XSPH)
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
            force = p->body->getLinearVelocity() + XSPH_CHAINING * force;
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

inline void VRFluids::sph_calc_density(SphParticle* p, btVector3 p_origin, int from, int to) {
    p->sphDensity = 0;
    btVector3 q_origin;
    for (int i=from; i < to; i++) {
            q_origin = particles[i]->body->getWorldTransform().getOrigin();
            p->sphDensity += particles[i]->mass * kernel_poly6(p_origin.distance2(q_origin), p->sphArea);
    }
}

inline void VRFluids::sph_calc_pressure(SphParticle* p, btVector3 p_origin, int from, int to) {
    p->sphPressureForce.setZero();
    float p_pressure = PRESSURE_KAPPA * (p->sphDensity - PRESSURE_REST);
    SphParticle* q;
    btVector3 q_origin;
    float q_pressure = 0;
    float trick = 0; // variable to store trick to make force symmetrical
    btVector3 kernel(0,0,0);

    for (int i=from; i < to; i++) {
            q = (SphParticle*) particles[i];
            q_origin = q->body->getWorldTransform().getOrigin();
            q_pressure = PRESSURE_KAPPA * (p->sphDensity - PRESSURE_REST);
            trick = (p_pressure + q_pressure) / (2 * q->sphDensity); // TODO what if density is 0?
            //btVector3 kernel = kernel_spiky_gradient(p_origin - q_origin, p->sphArea);
            //printf("-->Force(%f), Mass(%f), Pressure(%f), Kernel(%f)\n", p->sphPressureForce[1], p->mass, trick, kernel[1]); // TODO debug
            kernel = kernel_spiky_gradient(p_origin - q_origin, p->sphArea);
            if (kernel.length() > 0) p->sphPressureForce -= q->mass * trick * kernel;
            //printf("-->(%f, %f, %f)\n", q->mass, trick, (kernel_spiky_gradient(p_origin - q_origin, p->sphArea))[2]); // TODO debug
            //printf("-->(%f)\n", (q->mass * trick * kernel_spiky_gradient(p_origin - q_origin, p->sphArea))[2]); // TODO debug
    }
    //printf("--> PressureF: (%f,%f,%f)\n", p->sphPressureForce[0],p->sphPressureForce[1],p->sphPressureForce[2]);
}

inline void VRFluids::sph_calc_viscosity(SphParticle* p, btVector3 p_origin, int from, int to) {
    p->sphViscosityForce.setZero();
    btVector3 p_speed = p->body->getLinearVelocity();
    SphParticle* q;
    btVector3 q_origin;
    btVector3 q_speed;

    for (int i=from; i < to; i++) {
        if (p != particles[i]) {
            q = (SphParticle*) particles[i];
            q_origin = q->body->getWorldTransform().getOrigin();
            q_speed = q->body->getLinearVelocity();
            q_speed = (p_speed - q_speed) / q->sphDensity;
            p->sphViscosityForce += q->mass * q_speed * kernel_visc_laplacian(p_origin.distance2(q_origin), p->sphArea);
        }
    }
    p->sphViscosityForce *= VISCOSITY_MU;
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
    // TODO h and r2 are supposed to be positive (unsigned). Or use btScalar.
    float diff = h*h - r2;
    if (diff >= 0) {
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

/** Kernel for pressure */
inline btVector3 VRFluids::kernel_spiky_gradient(btVector3 r, float h) {
    float rNorm = r.length();
    //if (std::isnan(rNorm) || rNorm <= 0) rNorm = 0.0001;
    float diff = h - rNorm;
    if (diff > 0 && rNorm > 0) {
        return (45 / (Pi * pow(h,6))) * (r/rNorm) * pow(diff,2);
    }
    return btVector3(0,0,0);
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
