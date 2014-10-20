#ifndef NEUTRONENFLUSS_H_INCLUDED
#define NEUTRONENFLUSS_H_INCLUDED

#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMaterialChunk.h>
#include <OpenSG/OSGPolygonChunk.h>
#include <OpenSG/OSGPointChunk.h>
#include <OpenSG/OSGBlendChunk.h>
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGClipPlaneChunk.h>

#include <OpenSG/OSGGeoProperties.h>

//#include "shader/Shader_neutronenfluss.h"
#include "core/objects/material/VRShader.h"
#include "../core/VRClipPlane.h"

using namespace std;

OSG_BEGIN_NAMESPACE;


class neutronenfluss {
    private:
        float mys;
        float vmax;
        float scale;
        bool do_play;
        int size[3];
        int frame_N, tN;
        vector<float> buffer;
        vector<ImageUnrecPtr> images;
        vector<GeometryRecPtr> reactor;
        VRTransform* flytracker;
        VRClipPlane* clipPlane;

        int frame;

        /*NodePtr Nlattice;
        NodePtr Nstate;
        NodePtr anchor;*/
        VRGeometry* Nstate;
        VRObject* anchor;
        //neutron_flux_shader* nfs;
        VRShader* nfs;

        TextureObjChunkRecPtr tex_obj_chunk;
        TextureEnvChunkRecPtr tex_env_chunk;

        //---------------------------GEOMETRIES

        GeometryRecPtr initPointState(bool wire) {//material not both sided lighning
            GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
            GeoUInt32PropertyRefPtr     Length = GeoUInt32Property::create();
            GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
            GeoVec3fPropertyRefPtr      Norms = GeoVec3fProperty::create();
            GeoUInt32PropertyRefPtr     Indices = GeoUInt32Property::create();
            SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
            GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

            int N = size[0]*size[1]*size[2];
            int ind = 0;
            int tco = 0;
            //Type->addValue(GL_TRIANGLES);
            Type->addValue(GL_POINTS);
            Length->addValue(N);

            //positionen und Normalen
            for(int k=0;k<size[2];k++) {
                cout << "\n";
            for(int j=0;j<size[1];j++) {
            for(int i=0;i<size[0];i++) {
                    Vec3f pos = Vec3f(i*mys, j*mys, k*mys);
                    pos -= Vec3f(size[0]*mys/2, size[1]*mys/2, size[2]*mys/2);
                    Pos->addValue(pos);
                    Norms->addValue(Vec3f(0,1,0));

                    ind = i + j*size[0] + size[0]*size[1]*k;
                    tco = i + j*size[0];

                    //-------------------TEX COORDS
                    //Tex->addValue(Vec2f((float)j, (float)i));
                    Tex->addValue(Vec2f(tco, k));

                    //-------------------INDICES
                    Indices->addValue(ind);
                    tN++;
            } } }


            Mat->setDiffuse(Color3f(0.8,0.8,0.6));
            Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
            Mat->setSpecular(Color3f(0.1, 0.1, 0.1));
            //nfs = new neutron_flux_shader(Mat, size[0]*size[1], size[2]);
            nfs = new VRShader(Mat);
            nfs->setFragmentProgram("shader/Shader_neutronenfluss.fp");
            nfs->setVertexProgram("shader/Shader_neutronenfluss.vp");
            nfs->setGeometryProgram("shader/Shader_neutronenfluss.gp");

            //float sw = 1./(size[0]*size[1]);
            //float sh = 1./size[2];
            float s0 = size[0];
            float s1 = size[1];
            float s2 = size[2];
            Vec4f clipplane = Vec4f(0.0, -1.0, 0.0, 0);

            nfs->addParameter("scale", scale);
            nfs->addParameter("displacementMap", 0);
            nfs->addParameter("voxel", mys);
            nfs->addParameter("s0", s0);
            nfs->addParameter("s1", s1);
            nfs->addParameter("s2", s2);
            nfs->addParameter("cpeq", clipplane);


            tex_obj_chunk = TextureObjChunk::create();

            tex_obj_chunk->setMinFilter(GL_NEAREST);
            tex_obj_chunk->setMagFilter(GL_NEAREST);
            tex_obj_chunk->setScale(false);

            //tex_obj_chunk->setTarget(GL_TEXTURE_1D);
            //tex_obj_chunk->setTarget(GL_TEXTURE_RECTANGLE_NV);
            //tex_obj_chunk->setInternalFormat(GL_RGBA32F_ARB);
            //tex_obj_chunk->setInternalFormat(GL_FLOAT_RGBA32_NV);
            tex_obj_chunk->setInternalFormat(GL_RGBA_FLOAT32_ATI);

            tex_env_chunk = TextureEnvChunk::create();
            tex_env_chunk->setEnvMode(GL_NONE);
            Mat->addChunk(tex_env_chunk);//add new texture chunk

            BlendChunkRecPtr blend_chunk = BlendChunk::create();
            blend_chunk->setAlphaValue(1.0);
            blend_chunk->setSrcFactor (GL_SRC_ALPHA);
            blend_chunk->setDestFactor(GL_ONE_MINUS_SRC_ALPHA);

            Mat->addChunk(tex_obj_chunk);//add new texture chunk
            Mat->addChunk(blend_chunk);//add new texture chunk


            GeometryRecPtr geo = Geometry::create();

            geo->setTypes(Type);
            geo->setLengths(Length);
            geo->setIndices(Indices);
            geo->setPositions(Pos);
            geo->setNormals(Norms);
            geo->setTexCoords(Tex);
            geo->setMaterial(Mat);

            return geo;
        }

        //---------------------------DATA HANDLING

        void load(string path) {
            cout << "\nload file : " << path << flush;
            ifstream file(path.c_str(), fstream::in);
            if (!file.is_open()) return;


            char tmp;
            char line[128];

            //PARSE HEADER------------------------------------------------------------------------

            do file >> tmp; while(tmp != '=');
            file >> size[0];
            do file >> tmp; while(tmp != '=');
            file >> size[1];
            do file >> tmp; while(tmp != '=');
            file >> size[2];

            file.getline(line, 128);//end current line
            file.getline(line, 128);//end current line
            //do file.getline(line, 128); while(line[0] == '#');//skip all other coments
            while (file.get() == '#') file.getline(line, 128);//skip all other coments

            int N = size[0]*size[1]*size[2];

            cout << "\nPARSE: " << size[0] << " " << size[1] << " " << size[2] << "\n";
            //cout << "\nPARSE: " << line << "\n";
            //PARSE HEADER------------------------------------------------------------------------
            //PARSE DATA------------------------------------------------------------------------


            cout << "\nLoad data\n"; //NEED ONLY ONE BUFFER, DATA IS STORED IN IMAGES!
            buffer = vector<float>(N);

            frame_N = 0;
            while (!file.eof()) {
                for (int j=0;j<N;j++) {
                    file >> buffer[j];//load frames
                    if (file.eof()) return;
                    //cout << "\n " << buffer[j];
                    if (vmax<buffer[j]) {
                        vmax=buffer[j];
                    }
                }
                createImage();//create image with buffer data
                frame_N++;
            }

            file.close();
            cout << "\nLoading complete\n";
        }

        void createImage() {
            //ImageRecPtr createImage(int f) {
            int N = size[0]*size[1]*size[2];

            UChar8* data = new UChar8[N*4];
            for(int i=0;i<N;i++) {
                if (buffer[i] > 1) buffer[i] = 1;

                data[i*4] = buffer[i]*255;
                data[i*4+1] = buffer[i]*255*255 - 255*data[i*4];
                data[i*4+2] = buffer[i]*255*255*255 - 255*data[i*4+1] - 255*255*data[i*4];
                data[i*4+3] = 0;


                //cout << "\n " << buffer[i];
                //cout << "   " << (float)data[i*4] << " " << (float)data[i*4+1] << " " << (float)data[i*4+2];
                //cout << "   " << data[i*4] + data[i*4+1]*0.00392156862745 + data[i*4+2]*1.53787004998e-05;
            }

            ImageUnrecPtr img = Image::create();
            img->set( Image::OSG_RGBA_PF, size[0]*size[1], size[2], 1, 1, 1, 0, data);
            //img->read("data/tex/test.jpg");
            images.push_back(img);

            //delete[] data;
        }

        void updateState(int f) {
            cout << "\nUpdate state frame " << f << endl;
            tex_obj_chunk->setImage(images[f]);//bug
        }

        void navigation(VRScene* scene, VRTransform* cam, VRTransform* flystick, VRDevice* dev) {
            int key = dev->key();
            float d = 0;

            int button = false;
            dev->b_state(key,&button);
            if (button) return;//perhapst invert?

            dev->s_state(key, &d);
            float d_abs = abs(d);

            if (scene != VRSceneManager::get()->getActiveScene()) return;

            Vec3f dir = flystick->getWorldDirection();
            dir.normalize();

            switch(key) {
                case 0:
                    cam->rotate(-d*d_abs*0.04*mys);
                    break;
                case 1:
                    cam->translate(-dir*d*d_abs*2*mys);
                    break;
            }
        }

        void multScale(float s) {scale *= s; nfs->setParameter("scale", scale); cout << "\nMULT: " << scale; }

    public:
        neutronenfluss(VRDevice* fly, VRTransform* _flytracker) {
            VRScene* scene = new VRScene("Neutronenfluss");

            vmax = 0;
            do_play = 0;
            frame = 0;
            tN = 0;
            mys = 0.02;
            scale = 1;
            flytracker = 0;

            VRTransform* cam = scene->addCamera("main_cam");
            scene->add(cam);

            if (fly) cam->setPose(Vec3f(-1,-1.5,-1), Vec3f(0,-1.5,0), Vec3f(0,1,0));
            else cam->setPose(Vec3f(-50*mys,0,-50*mys), Vec3f(0,0,0), Vec3f(0,1,0));
            //cam->setPose(Vec3f(-50*mys,0,-50*mys), Vec3f(0,0,0), Vec3f(0,1,0));

            cam->setFixed(false);
            //VRNavigator::get()->setKeyboardNavigationCentred(scene, cam);
            //if (fly) VRNavigator::get()->setupNavigationCentred(fly, scene, cam, _flytracker, 0.2);

            //headlight
            VRLight* headlight = scene->addLight("head_light");
            headlight->setType("point");
            cam->addChild(headlight);

            //----------------------------------------------------
            Nstate = new VRGeometry("gr_state");
            anchor = new VRObject("gr");
            anchor->addChild(Nstate);
            scene->add(anchor);

            //scene->add(makeBox(0.3,0.3,0.3,1,1,1));


            load("data/neutronenfluss/plot_file.dat");
            //load("data/neutronenfluss/plot_file_test.dat");
            cout << "\n VMAX: " << vmax;
            Nstate->setMesh(initPointState(VROptions::get()->getOption<bool>("graphene_wired")));
            Nstate->setPickable(true);
            update(0);

            //-------------------------------------------------------
            //Clip ebene

            clipPlane = new VRClipPlane();
            if (fly) clipPlane->setBeacon(_flytracker);
            else clipPlane->setBeacon(VRMouse::get()->getBeacon());
            clipPlane->setEquation(Vec4f(0,-1,0,0));
            clipPlane->addShader(nfs);//make sure shader has been init.
            scene->addUpdateFkt(new VRFunction<int>("Neutronenfluss", boost::bind(&VRClipPlane::update, clipPlane)));


            //------------------------------------


            VRDevice* keyb = VRKeyboard::get();

            VRDevCb* play = new VRDevCb("Neutronenfluss", boost::bind(&neutronenfluss::setPlay, this, true));
            VRDevCb* stop = new VRDevCb("Neutronenfluss", boost::bind(&neutronenfluss::setPlay, this, false));
            VRDevCb* toggle = new VRDevCb("Neutronenfluss", boost::bind(&neutronenfluss::toggle_play, this, _1));
            //VRFunction* reset = new VRFunction(boost::bind(&neutronenfluss::reset, this, _1));
            VRDevCb* clipActiveToggle = new VRDevCb("Neutronenfluss", boost::bind(&VRClipPlane::toggleActive, clipPlane));

            VRDevCb* multSup = new VRDevCb("Neutronenfluss", boost::bind(&neutronenfluss::multScale, this, 1.2));
            VRDevCb* multSdown = new VRDevCb("Neutronenfluss", boost::bind(&neutronenfluss::multScale, this, 0.8));

            keyb->addSignal( 'a', 1)->add( play );
            keyb->addSignal( 's', 1)->add( stop );

            if (fly) {
                flytracker = _flytracker;
                fly->addSignal( 4, 0)->add( toggle );
                fly->addSignal( 1, 0)->add( clipActiveToggle );

                fly->addSignal( 2, 0)->add( multSup );
                fly->addSignal( 3, 0)->add( multSdown );

                //fly->addSignal( 0, 0, boost::bind(&neutronenfluss::navigation, this, scene, cam, _flytracker, _1));
            }

            scene->addUpdateFkt(new VRFunction<int>("Neutronenfluss", boost::bind(&neutronenfluss::update, this)));


            VRSceneManager::get()->addScene(scene);
            cout << "\nInitiation complete\n";
        }

        void update(int i) { updateState(i); }

        void update() {
            if (do_play == true) {
                frame++;
                if (frame>frame_N) frame = 0;
                update(frame);
                osgSleep(50);
            }
        }

        void setPlay(bool b) { do_play = b; }

        void toggle_play(VRDevice* dev) { int s; dev->b_state(3, &s); if(s == 1) do_play = !do_play; }

        void reset(VRDevice* dev) { frame = 0; update(0); }
};

OSG_END_NAMESPACE;

#endif // NEUTRONENFLUSS_H_INCLUDED
