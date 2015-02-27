#ifndef VRTREE_H_INCLUDED
#define VRTREE_H_INCLUDED

#include "core/objects/material/VRShader.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTree : public VRGeometry {

    private:

        struct seg_params {
            int iterations; //number of iterations
            int child_number; //number of children

            float n_angle; //angle between n1 && parent n2
            float p_angle; //angle between axis (p1 p2) && parent axis (p1 p2)
            float l_factor; //length diminution factor
            float r_factor; //radius diminution factor

            float n_angle_var; //n_angle variation
            float p_angle_var; //p_angle variation
            float l_factor_var; //l_factor variation
            float r_factor_var; //r_factor variation

            seg_params () {
                iterations = 5;
                child_number = 5;

                n_angle = 0.2;
                p_angle = 0.6;
                l_factor = 0.8;
                r_factor = 0.5;

                n_angle_var = 0.2;
                p_angle_var = 0.4;
                l_factor_var = 0.2;
                r_factor_var = 0.2;
            }
        };

        struct segment {

            Vec3f p1, p2, n1, n2;
            Vec2f params[2];
            segment* parent;
            vector<segment*> children;

            //defaults are for the trunc
            segment(segment* _parent = 0,
                    Vec3f _p1 = Vec3f(0,0,0),
                    Vec3f _n1 = Vec3f(0,1,0),
                    Vec3f _p2 = Vec3f(0,1,0),
                    Vec3f _n2 = Vec3f(0,1,0)) {
                p1 = _p1;
                p2 = _p2;
                n1 = _n1;
                n2 = _n2;

                params[0] = Vec2f(1, 0);
                params[1] = Vec2f(1, 0);

                parent = _parent;
            }
        };

        segment* trunc;
        vector<segment*>* branches;
        VRGeometry* armatureGeo;


        float random (float min, float max) {
            if (max!=min) {
                float c = 1e5;
                return (rand()%(int)(c*max-c*min)+c*min)/c;
            } else return max;
        }

        float variation(float val, float var) { return random(val*(1-var), val*(1+var)); }

        Vec3f randUVec() { return Vec3f(random(-1,1), random(-1,1), random(-1,1)); }

        //rotate a vector with angle 'a' in a random direction
        Vec3f randomRotate(Vec3f v, float a) {
            if (a == 0) return v;

            Vec3f x, d;

            do x = randUVec();
            while (x.dot(v) > 1e-3);

            d = v.cross(x);
            d.normalize();

            Quaternion q(d, a);
            q.multVec(v, v);
            return v;
        }

        void grow(seg_params* sp, segment* p, int iteration = 0) {
            if (sp == 0) return;
            if (p == 0) return;
            if (iteration == sp->iterations) return;

            for (int i=0;i<sp->child_number;i++) {
                segment* c = new segment(p, p->p2);
                branches->push_back(c);
                c->p2 = randomRotate(p->p2 - p->p1, variation(sp->p_angle, sp->p_angle_var));
                c->p2 *= variation(sp->l_factor, sp->l_factor_var);
                c->p2 += p->p2;

                c->n2 = c->p2 - c->p1;
                c->n2.normalize();
                c->n1 = p->n2 + (p->n2 - c->n2)*variation(sp->n_angle, sp->n_angle_var);

                c->params[0] = Vec2f(pow(sp->r_factor,iteration), 0);
                c->params[1] = Vec2f(pow(sp->r_factor,iteration+1), 0);

                p->children.push_back(c);
            }

            for (int i=0;i<sp->child_number;i++) {
                grow(sp, p->children[i], iteration+1);
            }
        }

        void setMaterial(VRGeometry* geo) {
            SimpleMaterialRecPtr mat = SimpleMaterial::create();

            mat->setDiffuse(Color3f(0.8,0.8,0.6));
            mat->setAmbient(Color3f(0.4, 0.4, 0.2));
            mat->setSpecular(Color3f(0.1, 0.1, 0.1));

            VRShader* nfs = new VRShader(mat);
            string wdir = VRSceneManager::get()->getOriginalWorkdir();
            nfs->setFragmentProgram(wdir+"/shader/Trees/Shader_tree_base.fp");
            nfs->setVertexProgram(wdir+"/shader/Trees/Shader_tree_base.vp");
            nfs->setGeometryProgram(wdir+"/shader/Trees/Shader_tree_base.gp");


            TextureEnvChunkRecPtr tex_env_chunk = TextureEnvChunk::create();
            TextureObjChunkRecPtr tex_obj_chunk = TextureObjChunk::create();
            ImageRecPtr img = Image::create();
            img->read("textures/terasse.jpg");
            tex_obj_chunk->setImage(img);
            nfs->addParameter("texture", 0);

            mat->addChunk(tex_obj_chunk);//add new texture chunk
            mat->addChunk(tex_env_chunk);//add new texture chunk*/

            geo->setMaterial(mat);
        }

        VRGeometry* createArmatureGeo() {
            VRGeometry* geo = new VRGeometry("armature");

            vector<Vec3f> pos, norms;
            vector<Vec2f> texs;
            vector<int> inds;

            for (uint i=0;i<branches->size();i++) {
                segment* s = branches->at(i);
                pos.push_back(s->p1);
                pos.push_back(s->p2);
                norms.push_back(s->n1);
                norms.push_back(s->n2);
                inds.push_back(2*i);
                inds.push_back(2*i+1);
                texs.push_back(s->params[0]);
                texs.push_back(s->params[1]);
            }

            geo->create(GL_LINES, pos, norms, inds, texs);
            return geo;
        }

        void generateArmature() {
            srand(time(0));
            trunc = new segment();
            branches = new vector<segment*>();
            branches->push_back(trunc);
            seg_params* sp = new seg_params();
            grow(sp, trunc);
            armatureGeo = createArmatureGeo();
            setMaterial(armatureGeo);
            addChild(armatureGeo);
            //armatureGeo->hide();
        }

    public:
        VRTree() : VRGeometry("tree") {
            trunc = 0;
            branches = 0;
            generateArmature();
        }
};

OSG_END_NAMESPACE;

#endif // VRTREE_H_INCLUDED
