#ifndef VRPARTICLEST_H_INCLUDED
#define VRPARTICLEST_H_INCLUDED

#include "core/objects/material/VRMaterial.h"

typedef boost::recursive_mutex::scoped_lock BLock;

OSG_BEGIN_NAMESPACE;

template<class P>
void VRParticles::resetParticles() {

    VRScene* scene = VRSceneManager::getCurrent();
    if (scene) world = scene->bltWorld();

    {
        // NOTE should fkt be dropped here?
        BLock lock(mtx());
        for (int i=0; i<particles.size(); i++) delete particles[i];
        particles.resize(N, 0);
        for (int i=0; i<particles.size(); i++) particles[i] = new P(world);
    }

    // material
    if (mat) delete mat;
    mat = new VRMaterial("particles");
    mat->setDiffuse(Vec3f(0,0,1));
    mat->setPointSize(5);
    mat->setLit(false);

    // geometry
    GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
    GeoUInt32PropertyRecPtr inds = GeoUInt32Property::create();
    pos = GeoPnt3fProperty::create();
    Length->addValue(N);

    for(int i=0;i<N;i++) pos->addValue(Pnt3f(0,0,0));
    for(int i=0;i<N;i++) inds->addValue(i);

    setType(GL_POINTS);
    setLengths(Length);
    setPositions(pos);
    setIndices(inds);
    setMaterial(mat);
}
// NOTE compiler needs to know which classes can be used for particles
// otherwise you can not link successfully.
// see: http://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
/*OSG_BEGIN_NAMESPACE;
template void VRParticles::resetParticles<SphParticle>();
OSG_END_NAMESPACE;*/
OSG_END_NAMESPACE;

#endif // VRPARTICLEST_H_INCLUDED
