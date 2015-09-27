/*
 *
 *  * File is obsolete... keep for stuff in the codes

#ifndef MESHGENERATOR_H
#define MESHGENERATOR_H

#include "core/scene/VRScene.h"
#include "WorldFeatures/WorldFeature.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


class MeshGenerator
{
    public:
        MeshGenerator() {}

        static void addWorld(list<WorldFeature*> features, VRScene* scene) {
            WorldFeature* f = features.front();
            VRTransformPtr ent = VRTransform::create("FOO");

//            for (int i=0; i<1000; i++) {
            BOOST_FOREACH(WorldFeature* f, features) {
                createGeometry(f, ent);
            }

            scene->add(ent);
        }
    protected:
    private:

        static void createGeometry(WorldFeature* feature, VRTransformPtr scene) {
            //if (feature->featureClass != "Building") return;

            SimpleMaterialRecPtr mat = SimpleMaterial::create();
            Color3f white(1, 1, 1);
            mat->setDiffuse(white);

            VRGeometryPtr geo = VRGeometry::create("featureMesh-"+feature->geom->way->id);
            vector<Vec3f> pos;
            vector<Vec3f> norms;
            vector<int> inds;
            vector<Vec2f> texs;

            for (int i=0;i<4;i++) {
                norms.push_back(Vec3f(0,1,0));
                inds.push_back(i);
            }

            pos.push_back(Vec3f(0, 0, 0));
            pos.push_back(Vec3f(0, 0, 1));
            pos.push_back(Vec3f(1, 0, 1));
            pos.push_back(Vec3f(1, 0, 0));

            texs.push_back(Vec2f(0,0));
            texs.push_back(Vec2f(0,1));
            texs.push_back(Vec2f(1,1));
            texs.push_back(Vec2f(1,0));

            geo->create(GL_QUADS, pos, norms, inds, texs);
            geo->setMaterial(mat);
            scene->addChild(geo);
        }
};

OSG_END_NAMESPACE;

#endif // MESHGENERATOR_H
*/
