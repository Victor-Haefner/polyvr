#ifndef GRAPHENE_H_INCLUDED
#define GRAPHENE_H_INCLUDED

#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMaterialChunk.h>
#include <OpenSG/OSGPolygonChunk.h>
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGTextureBackground.h>

#include <OpenSG/OSGGeoProperties.h>
#include "shader/Shader_displacement_color.h"
#include "../navigation/VRNavigator.h"

using namespace std;

OSG_BEGIN_NAMESPACE;


class graphene {
    private:
        float mys;
        bool do_play;
        int rN, cN, fN, tN;
        vector< vector<Real32*> > frames;
        vector<ImageRecPtr> images;
        vector<char*> lattice;//lattice structure, with A and B the sublattices, D the defects
        vector<GeometryRecPtr> grid;


        int frame;
        float scale;

        /*NodePtr Nlattice;
        NodePtr Nstate;
        NodePtr anchor;*/
        VRGeometry* Nlattice;
        VRGeometry* Nstate;
        VRObject* anchor;

        TextureObjChunkRecPtr tex_obj_chunk;
        TextureEnvChunkRecPtr tex_env_chunk;

        map<int, string> extensions;
        int extensions_status;

        //---------------------------GEOMETRIES

        ChunkMaterialRecPtr getWiredMaterial() {

            //Create the PolygonChunk
            PolygonChunkRecPtr pch = PolygonChunk::create();
            pch->setFrontMode(GL_LINE);
            pch->setBackMode(GL_LINE);

            //Create the Chunk Material
            ChunkMaterialRecPtr mat = ChunkMaterial::create();
            mat->addChunk(pch);
            return mat;
        }

        bool CheckExtension(string extN) {
            const char* tmp = (const char*) glGetString(GL_EXTENSIONS);

            if (!tmp) return false;

            string extentions = string(tmp);

            if (string::npos != extentions.find(extN)) {
                return true;
            }
            return false;
        }

        GeometryRecPtr initState(bool wire) {//material not both sided lighning
            GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
            GeoUInt32PropertyRefPtr     Length = GeoUInt32Property::create();
            GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
            GeoVec3fPropertyRefPtr      Norms = GeoVec3fProperty::create();
            GeoUInt32PropertyRefPtr     Indices = GeoUInt32Property::create();
            SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
            GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

            int tN = 0;//(rN-1)*(cN-1)*2;  //count them when making them!
            Type->addValue(GL_TRIANGLES);

            //positionen und Normalen
            for(int i=0;i<rN;i++) {
                for(int j=0;j<cN;j++) {
                    char L = lattice[i][j];

                    char L0, L1, L2, L3; L0=L1=L2=L3='D';//nachbar atome L[012] und L3 das atom auf der anderen seite von hexagon unten rechts
                    if(j>0) L0 = lattice[i][j-1];
                    if(j<cN-1) L1 = lattice[i][j+1];
                    if (L == 'A' and i>0) L2 = lattice[i-1][j];
                    if (L == 'B' and i< rN-1) L2 = lattice[i+1][j];
                    if (L == 'B' and i< rN-1 and j< cN-2) L3 = lattice[i+1][j+2];//L3 nur fuer B definiert

                    Vec3f pos = Vec3f(0,0,0);
                    if (L == 'A') pos = Vec3f(j*sqrt(3)/2.*mys, 0, i*1.5*mys);//y should be 0 for the moment
                    if (L == 'B') pos = Vec3f(j*sqrt(3)/2.*mys, 0, i*1.5*mys + 0.5*mys);
                    Pos->addValue(pos);
                    Norms->addValue(Vec3f(0,1,0));

                    //-------------------TEX COORDS
                    Tex->addValue(Vec2f((float)j, (float)i));

                    //-------------------INDICES

                    if (L == 'D' and i < rN-1 and j < cN -2) {
                        if (L1 == 'A' and lattice[i+1][j] != 'D' and lattice[i+1][j+2] != 'D' and i< rN-1 and j<cN-2)//inter-hexagon triangle
                            { Indices->addValue(cN*i + j+1); Indices->addValue(cN*(i+1) + j); Indices->addValue(cN*(i+1) + j+2); tN++; }
                        continue;
                    }


                    if (L == 'B') {
                        if (L0 != 'D' and L1 != 'D')
                            { Indices->addValue(cN*i + j); Indices->addValue(cN*i + j+1); Indices->addValue(cN*i + j-1); tN++; }

                        if (L0 != 'D' and L2 != 'D')
                            { Indices->addValue(cN*i + j); Indices->addValue(cN*i + j-1); Indices->addValue(cN*(i+1) + j); tN++; }

                        if (L1 != 'D' and L2 != 'D')
                            { Indices->addValue(cN*i + j); Indices->addValue(cN*(i+1) + j); Indices->addValue(cN*i + j+1); tN++; }

                        if (L1 != 'D' and L2 != 'D' and L3 != 'D')
                            { Indices->addValue(cN*i + j+1); Indices->addValue(cN*(i+1) + j); Indices->addValue(cN*(i+1) + j+2); tN++; }
                    }

                }
            }
            Length->addValue(3*tN);

            Mat->setDiffuse(Color3f(0.8,0.8,0.6));
            Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
            Mat->setSpecular(Color3f(0.1, 0.1, 0.1));
            dcShader shd(Mat, cN);


            tex_obj_chunk = TextureObjChunk::create();
            tex_obj_chunk->setMinFilter(GL_NEAREST);
            tex_obj_chunk->setMagFilter(GL_NEAREST);
            //tex_obj_chunk->setWrapS(GL_CLAMP);
            //tex_obj_chunk->setWrapT(GL_CLAMP);
            tex_obj_chunk->setScale(false);

            tex_env_chunk = TextureEnvChunk::create();
            tex_env_chunk->setEnvMode(GL_NONE);
            Mat->addChunk(tex_env_chunk);//add new texture chunk

            Mat->addChunk(tex_obj_chunk);//add new texture chunk

            GeometryRecPtr geo = Geometry::create();

            geo->setTypes(Type);
            geo->setLengths(Length);
            geo->setIndices(Indices);
            geo->setPositions(Pos);
            geo->setNormals(Norms);
            geo->setTexCoords(Tex);
            if (wire) geo->setMaterial(getWiredMaterial());
            else geo->setMaterial(Mat);

            return geo;
        }

        GeometryRecPtr initLattice() {
            //use positions vector from state geometry
            //red or blue hexagons, use lines primitives with wired material
            return Geometry::create();
        }

        //---------------------------DATA HANDLING

        bool load(string path) {
            cout << "\nload file : " << path << flush;
            ifstream file(path.c_str(), fstream::in | fstream::binary);
            if (!file.is_open()) {
                cout << "\nERROR: can not find : " << path << "\n";
                return false;
            }

            //get framecount
            file.seekg(0, ios::end);  // position get-ptr 0 bytes from end
            fN = file.tellg();  // get-ptr position is now same as file size

            file.seekg(ios::beg);
            file.read((char*)&rN, sizeof(int));//rows
            file.read((char*)&cN, sizeof(int));//collums

            fN -= 2*sizeof(int) + cN*rN*sizeof(char);
            fN /= cN*rN*sizeof(float);

            cout << "\nfN : " << fN;
            cout << "\nrN : " << rN;
            cout << "\ncN : " << cN;

            //load lattice
            cout << "\nLoad lattice\n";
            for (int i=0;i<rN;i++) {
                char* tmp = new char[cN];
                file.read((char*)tmp, cN*sizeof(char));
                lattice.push_back(tmp);
            }

            cout << "\nLoad data\n"; //NEED ONLY ONE BUFFER, DATA IS STORED IN IMAGE!
            frames.push_back(vector<float*>());//tmp frame
            for (int j=0;j<rN;j++) frames[0].push_back(new float[cN]);//rows

            for (int i=0;i<fN;i++) {
                for (int j=0;j<rN;j++) file.read((char*)frames[0][j], cN*sizeof(float));//load frames
                createImage();
            }

            for (int j=0;j<rN;j++) delete[] frames[0][j];

            cout << "\nLoading complete\n";
            return true;
        }

        void createImage() {
            //ImageRecPtr createImage(int f) {
            UChar8* data = new UChar8[rN*cN*4];

            int k = 0;
            for(int i=0;i<rN;i++)
                for(int j=0;j<cN;j++) {
                    k = i*cN+j;

                    data[k*4] = frames[0][i][j]*255;//frames is from 0 to 1
                    data[k*4+1] = frames[0][i][j]*255*255 - 255*data[k*4];
                    data[k*4+2] = frames[0][i][j]*255*255*255 - 255*data[k*4+1] - 255*255*data[k*4];
                    data[k*4+3] = scale;

                    //cout << data[k*4]-data[k*4+1] << " ";
                }

            ImageUnrecPtr img = Image::create();
            img->set( Image::OSG_RGBA_PF, cN, rN, 1, 1, 1, 0, data);
            //img->set( Image::OSG_RGB_PF, cN, rN, 1, 1, 1, 0, data);
            //img->set( Image::OSG_RGBA_PF, cN, rN, 1, 1, 1, 0, reinterpret_cast<OSG::UInt8*>(data), OSG::Image::OSG_FLOAT32_IMAGEDATA);
            //img->read("data/tex/test2.jpg");
            images.push_back(img);

            delete[] data;
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


    public:
        graphene(VRDevice* fly, VRTransform* flytracker) {
            extensions[GL_EXT_bgra] = "GL_EXT_bgra";
            extensions[GL_RGBA_FLOAT32_ATI] = "GL_RGBA_FLOAT32_ATI";
            extensions[GL_RGBA32F_ARB] = "GL_RGBA32F_ARB";
            extensions[GL_ATI_texture_float] = "GL_ATI_texture_float";

            VRScene* scene = new VRScene("Graphene");

            ImageRecPtr img = Image::create();
            img->read("data/graphene/graphene.jpg");
            TextureObjChunkRecPtr tex = TextureObjChunk::create();
            tex->setImage(img);
            TextureBackgroundRecPtr tbg = TextureBackground::create();
            tbg->setTexture(tex);

            SolidBackgroundRecPtr sbg = SolidBackground::create();
            sbg->setColor(Color3f(0,0,0));

            scene->setBackground(tbg);

            do_play = 0;
            frame = 0;
            tN = 0;
            extensions_status = 0;
            //mys = 0.5;
            mys = 1;

            VRTransform* cam = scene->addCamera("main_cam");
            scene->add(cam);

            cam->setPose(Vec3f(-50*mys,50*mys,-50*mys), Vec3f(50*mys,50*mys,50*mys), Vec3f(0,1,0));
            cam->setFixed(false);
            //VRNavigator::get()->setKeyboardNavigationCentred(scene, cam);

            //headlight
            VRLight* headlight = scene->addLight("head_light");
            headlight->setType("point");
            cam->addChild(headlight);

            Nstate = new VRGeometry("gr_state");
            Nlattice = new VRGeometry("gr_lattice");
            anchor = new VRObject("gr");
            anchor->addChild(Nstate);
            anchor->addChild(Nlattice);
            scene->add(anchor);

            scale = VROptions::get()->getOption<float>("graphene_scale")*mys;
            if (scale >255) scale = 255*mys;
            if (!load(VROptions::get()->getOption<string>("graphene_path"))) return;

            scene->addUpdateFkt(new VRFunction<int>("Graphene", boost::bind(&graphene::update, this)));

            VRDevice* keyb = VRKeyboard::get();

            VRDevCb* play = new VRDevCb("Graphene", boost::bind(&graphene::setPlay, this, true));
            VRDevCb* stop = new VRDevCb("Graphene", boost::bind(&graphene::setPlay, this, false));
            VRDevCb* toggle = new VRDevCb("Graphene", boost::bind(&graphene::toggle_play, this, _1));
            VRDevCb* reset = new VRDevCb("Graphene", boost::bind(&graphene::reset, this, _1));

            keyb->addSignal( 'a', 1)->add( play );
            keyb->addSignal( 's', 1)->add( stop );
            keyb->addSignal( 'd', 1)->add( reset );


            fly = VRSceneManager::get()->getDeviceFlystick();
            if (fly) {
                fly->addSignal( 4, 1)->add( toggle );
                //VRNavigator::get()->setupNavigationCentred(fly, scene, cam, fly->getBeacon(), 0.5);
            }

            VRSceneManager::get()->addScene(scene);

            Nstate->setMesh(initState(VROptions::get()->getOption<bool>("graphene_wired")));
            Nlattice->setMesh(initLattice());
            update(0);

            cout << "\nInitiation complete\n";

            VRSceneManager::get()->getRenderAction()->setFrustumCulling(false);
        }

        void update(int i) { updateState(i); }

        void update() {
            if (extensions_status == 0) {
                extensions_status = -1;
                for(map<int, string>::const_iterator itr = extensions.begin(); itr != extensions.end(); ++itr) {
                    if (CheckExtension((*itr).second)) {
                        //tex_obj_chunk->setInternalFormat((*itr).first);//TO DEBUG
                        tex_obj_chunk->setInternalFormat(GL_RGBA_FLOAT32_ATI);//TODO
                        extensions_status = 1;
                        cout << "\nUSE GL EXTENSION : " << (*itr).second << "\n";
                        break;
                    }
                }
            }

            if (extensions_status == -1) cout << "\nNO GL EXTENSION FOUND!!\n";

            if (do_play == true and extensions_status == 1) {
                frame++;
                if (frame>=fN) frame = 0;
                update(frame);
                osgSleep(50);
            }
        }

        void setPlay(bool b) { do_play = b; }

        //void toggle_play(VRDevice* dev) { int s=0; dev->b_state(3, &s); if(s == 1) do_play = !do_play; }
        void toggle_play(VRDevice* dev) { do_play = !do_play; }

        void reset(VRDevice* dev) { frame = 0; update(0); }
};

OSG_END_NAMESPACE;

#endif // GRAPHENE_H_INCLUDED
