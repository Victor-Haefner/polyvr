#include "SimViDekont.h"
#include "Player.h"
#include "core/setup/devices/VRDevice.h"

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGGeoFunctions.h>

OSG_BEGIN_NAMESPACE;

Color3f SimViDekont::getColor(float v) {
    Color3f c;
    v *= 2;
    if (v < 1) {
        c[0] = 0;   // rot
        c[1] = v;   // gruen
        c[2] = 1-v; // blau
    } else {
        c[0] = v-1;   // rot
        c[1] = 2-v;   // gruen
        c[2] = 0; // blau
    }
    return c;
}

Color3f SimViDekont::getColor2(float v) {
    Color3f c;
    v *= 2;
    if (v < 1) {
        c[0] = 0;   // rot
        c[1] = v;   // gruen
        c[2] = 1-v; // blau
    } else {
        c[0] = v-1;   // rot
        c[1] = 2-v;   // gruen
        c[2] = 0; // blau
    }
    return c;
}

bool SimViDekont::isElementValid(int i, frame* f) {
    // compute bounding box

    Vec3f Min, Max, delta;
    uint id = f->ind->at(i*8); // vert ID

    for (int k=0;k<3;k++) Min[k] = f->pos->at( id ).x[k];
    for (int k=0;k<3;k++) Max[k] = f->pos->at( id ).x[k];

    for (int j =0; j<8; j++) {
        id = f->ind->at(i*8+j); // vert ID
        if (id >= f->pos->size()) continue;


        for (int k=0;k<3;k++) {
            Min[k] = min( f->pos->at(id).x[k], Min[k] );
            Max[k] = max( f->pos->at(id).x[k], Max[k] );
        }
    }

    delta = Max - Min;

    if((abs(delta[0]) > 2.45e-4) or (abs(delta[1])> 2.45e-4)
       or (abs(delta[2])> 2.45e-4) ) return false;

    return true;
}

bool SimViDekont::isElementValid0  (int i, frame* f) {
    // compute bounding box


    if(f->strain->at(i) <= 0.85)
        return true;
    //uint id = f->ind->at(i*8); // vert ID

    //cout << "i that's i'm using now..." << i << endl;
    return false;
}

/**
* Returns a VRGeometry of a given frame.
*/
void SimViDekont::createGeo(frame* f) {
    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    GeoVec3fPropertyRecPtr      colors = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      colors2 = GeoVec3fProperty::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    //GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

    bool doSurface = true;

    //
    if (doSurface)
        Type->addValue(GL_QUADS);  //GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS
    else Type->addValue(GL_POINTS);  //GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS



    // go through the point ids and count the elements they belong to
    map<int, int>* vmap = new map<int, int>();
    map<int, int>* vmap2 = new map<int, int>(); // maps points new to old IDs
    for(uint i=0; i<f->ind->size()/8; i++) { // is element ID
        bool eValid = isElementValid(i,f);

        for (uint j=0; j < 8; j++) { // 8 verts in element
            int id = f->ind->at(i*8+j); // vert ID

            // count vert IDs

            if (vmap->count(id) == 0) (*vmap)[id] = 0; // start counting
            if (eValid) (*vmap)[id]++;
        }
    }

    map<int, float> vertexStress;
    map<int, float> vertexStrain;
    computeVertexColors(f, vmap, vertexStress);
    computeVertexColors(f, vmap, vertexStrain, false);

    //positionen und Normalen
    Vec3f p;
    float k = 200; // scale
    int j = 0;
    for(uint i=0; i < f->pos->size(); i++) {
        if ((*vmap)[i] == 0 or (*vmap)[i] == 8) continue;

        //cout << "\n" << i << flush;
        p[0] = f->pos->at(i).x[0]*k;
        p[1] = f->pos->at(i).x[2]*k;
        p[2] = -f->pos->at(i).x[1]*k;
        //cout << "\n" << p << flush;

        Color3f c = getColor(vertexStress[i]);
        Color3f c2 = getColor2(vertexStrain[i]);
        colors->addValue(c);
        colors2->addValue(c2);


        Pos->addValue(p);
        //Pos->addValue(Vec3f(i/10, i%10, 0));
        Norms->addValue(Vec3f(0,1,0));
        //Norms->addValue(norms[i]);
        if (!doSurface) Indices->addValue(i);
        //Indices->addValue(i);
        //if (texs.size() == pos.size()) Tex->addValue(texs[i]);
        (*vmap2)[i] = j; j++;
    }

    int* id = new int[8];
    for(uint i=0; i < f->ind->size()/8; i++) {
        bool eValid = isElementValid(i,f);
        if (!eValid) continue;

        for (uint j=0; j < 8; j++) id[j] = f->ind->at(i*8+j);

        // element sides
        if (doSurface) {
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 3,2,1,0);
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 4,5,6,7);
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 0,1,5,4);
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 2,3,7,6);
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 3,0,4,7);
            quadOnSurface(vertexStress, vmap, vmap2, id, GeoUInt32PropertyTransitPtr(Indices), 1,2,6,5);
        }
    }

    delete[] id;
    delete vmap;
    delete vmap2;

    Length->addValue(Indices->size());

    cout << endl << "Indices Size: " << Indices->size() << endl;


    Mat->setDiffuse (Color3f(0.8, 0.8, 0.6));
    Mat->setAmbient (Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    PointChunkRecPtr pointChunk = PointChunk::create();
    pointChunk->setSize(3);
    Mat->addChunk(pointChunk);

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setColors(colors);
    geo->setSecondaryColors(colors2);
    //geo->setTexCoords(Tex);
    geo->setMaterial(Mat);

    calcVertexNormals(geo);

    //--- BEGIN Save Geo in .osb binary
        string path = "rslt/geo_frame";
        GeoIO::save(f->id, geo, path);
    //--- END Save Geo in .osb binary

    f->geo = new VRGeometry("myNewMesh");
    f->geo->setMesh(geo);
}

/**
* Compute the surface of the volume
* Abaqus element: SC8R, 8-node hexahedron, general-purpose, finite membrane strains
*
*        7--------6
*                /
*    4----------5
*
*        3--------2
*                /
*    0----------1
*/

void SimViDekont::quadOnSurface(map<int, float>& Vstress, map<int, int>* vmap, map<int, int>* vmap2, int* id, GeoUInt32PropertyTransitPtr Indices, int a, int b, int c, int d) {
    if ( (*vmap)[ id[a] ] == 8) return;
    if ( (*vmap)[ id[b] ] == 8) return;
    if ( (*vmap)[ id[c] ] == 8) return;
    if ( (*vmap)[ id[d] ] == 8) return;
    if ( (*vmap)[ id[a] ] == 0) return;
    if ( (*vmap)[ id[b] ] == 0) return;
    if ( (*vmap)[ id[c] ] == 0) return;
    if ( (*vmap)[ id[d] ] == 0) return;

    Indices->addValue((*vmap2)[ id[a] ]);
    Indices->addValue((*vmap2)[ id[b] ]);
    Indices->addValue((*vmap2)[ id[c] ]);
    Indices->addValue((*vmap2)[ id[d] ]);
}

void SimViDekont::computeVertexColors(frame* f, map<int, int>* vmap, map<int, float>& VsValue, bool stress) {
    if (f->stress->size() == 0 or f->strain->size() == 0 ) return;
    //float max = f->stress->at(0);   //1.22e9;
    float max = (stress) ? f->stress->at(0) : f->strain->at(0);
    for(uint i=0; i < f->ind->size()/8; i++) { // go through elements
        //float stress = f->stress->at(i+1)/max;
        float sValue = (stress) ? (f->stress->at(i+1)/max) : (f->strain->at(i+1)/max);

        for (uint j=0; j < 8; j++) { // 8 vertices per element
            int id = f->ind->at(i*8+j);

            VsValue[id] += sValue/float((*vmap)[id]);
        }
    }
}



void SimViDekont::showFrame(int i) {
    static int last = 0;
    frames->at(last)->geo->hide();
    frames->at(i)->geo->show();
    last = i;
    //cout << "\nGoto " << i << "  " << frames->size() << flush;
}

void SimViDekont::initFrameGeometries() {
    for (uint i=0; i < frames->size(); i++) {
        frame* f = frames->at(i);
        cout << "\nCONSTRUCT SURFACE " << f->id << endl;

        string path = "rslt/geo_frame";

        bool file = false;
        file =  GeoIO::load(f, path);

        if(!file) createGeo(f);

        root->addChild(f->geo);
        f->geo->hide();
        f->geo->translate(Vec3f(0,1,0));
    }
}

//--- BEGIN Player binding

//--- BEGIN Player callbacks

void SimViDekont::playForward(VRDevice* dev) { player->play(); }
void SimViDekont::playBackward(VRDevice* dev) { player->play(true); }
void SimViDekont::playStop(VRDevice* dev) { player->stop(); }
void SimViDekont::playStepForward(VRDevice* dev) { player->step(); }
void SimViDekont::playStepBackward(VRDevice* dev) { player->step(true); }

void SimViDekont::toggleColors(VRDevice* dev) {

    for(uint frameNum = 0; frameNum < frames->size(); frameNum++){

        GeometryRecPtr geo = frames->at(frameNum)->geo->getMesh();
        GeoVec3fPropertyRecPtr colors1 = dynamic_cast<GeoVec3fProperty*>(geo->getColors());
        GeoVec3fPropertyRecPtr colors2 = dynamic_cast<GeoVec3fProperty*>(geo->getSecondaryColors());
        geo->setColors(colors2);
        geo->setSecondaryColors(colors1);

    }

}


//--- END Player callbacks
void SimViDekont::initPlayer() {

    VRFunction<int>* cb = new VRFunction<int>("showFrame", boost::bind(&SimViDekont::showFrame, this, _1));
    player = new Player(cb, 0, frames->size() );

    /*VRFunction<VRDevice*>* callback1 = new VRFunction<VRDevice*>("playForward", boost::bind(&SimViDekont::playForward, this, _1));
    VRFunction<VRDevice*>* callback2 = new VRFunction<VRDevice*>("playBackward", boost::bind(&SimViDekont::playBackward, this, _1));
    VRFunction<VRDevice*>* callback3 = new VRFunction<VRDevice*>("playStop", boost::bind(&SimViDekont::playStop, this, _1));
    VRFunction<VRDevice*>* callback4 = new VRFunction<VRDevice*>("playStepForward", boost::bind(&SimViDekont::playStepForward, this, _1));
    VRFunction<VRDevice*>* callback5 = new VRFunction<VRDevice*>("playStepBackward", boost::bind(&SimViDekont::playStepBackward, this, _1));
    VRFunction<VRDevice*>* callback6 = new VRFunction<VRDevice*>("toggleColors", boost::bind(&SimViDekont::toggleColors, this, _1));*/

    // signals for key + and -
    /*VRDevice* keyboard = VRKeyboard::get();
    VRSignal* key_plus_pressed = keyboard->addSignal('+',1);
    VRSignal* key_minus_pressed = keyboard->addSignal('-',1);
    VRSignal* key_space_pressed = keyboard->addSignal(' ',1);
    VRSignal* key_six_pressed = keyboard->addSignal('6',1);
    VRSignal* key_four_pressed = keyboard->addSignal('4',1);
    VRSignal* key_zero_pressed = keyboard->addSignal('0',1);*/

    /*scene->bindLink(key_plus_pressed, callback1, "keyboard plus", "simvidekont playforward");
    scene->bindLink(key_minus_pressed, callback2, "keyboard minus", "simvidekont playbackward");
    scene->bindLink(key_space_pressed, callback3, "keyboard space", "simvidekont stop");
    scene->bindLink(key_six_pressed, callback4, "keyboard 6", "simvidekont stepforward");
    scene->bindLink(key_four_pressed, callback5, "keyboard 4", "simvidekont stepbackward");
    scene->bindLink(key_zero_pressed, callback6, "keyboard 0", "simvidekont toggle colors");

    VRDevice* mouse = VRMouse::get();
    scene->bindLink(mouse->addSignal(3, 1), callback1, "mouse button left pressed", "simvidekont playforward");
    scene->bindLink(mouse->addSignal(4, 1), callback2, "mouse button left pressed", "simvidekont playbackward");

    VRDevice* fly = VRSceneManager::get()->getDeviceFlystick();
    if (fly) {
        scene->bindLink(fly->addSignal(0, 1), callback6, "mouse button left pressed", "simvidekont playforward");

        scene->bindLink(fly->addSignal(1, 0), callback3, "mouse button left pressed", "simvidekont playforward");
        scene->bindLink(fly->addSignal(1, 1), callback1, "mouse button left pressed", "simvidekont playbackward");

        scene->bindLink(fly->addSignal(4, 0), callback3, "mouse button left pressed", "simvidekont playbackward");
        scene->bindLink(fly->addSignal(4, 1), callback2, "mouse button left pressed", "simvidekont playbackward");

        scene->bindLink(fly->addSignal(2, 1), callback4, "mouse button left pressed", "simvidekont playbackward");
        scene->bindLink(fly->addSignal(3, 1), callback5, "mouse button left pressed", "simvidekont playbackward");
    }*/
}

//--- END Player binding


SimViDekont::SimViDekont() {
    root = new VRObject("SimViDekont");

    //--- BEGIN Initialize VRScene

        //VROptions::get()->deferredShading = 0;
        //VRSceneManager::get()->showStats(0);
        //scene  = VRSceneLoader::get()->parseSceneFromXML("scene/simvidekont.xml", "Simvidekont");
        //VRCamera* cam = scene->getActiveCamera();
        //scene->get("Headlight_beacon")->switchParent(cam);


    //--- END Initialize VRScene


    /* Workflow:

    1: Load all frames from Binary file in frames vector
    2: Get a VRGeometry for each frame
    3: Get device signals, keyboard and mouse/flystick
    4: start Player
    5: Enjoy

    */

    loadAbaqusFile(0, 100); // load odb file and parse data to frames

    initFrameGeometries(); // create geometries for each frame and hide it

    //writeFrameStress(frames->at(3), "stress_data"); // checking if Stress data ok.



    //--- BEGIN Initialize Frame Player

        //initDevice(); // Device
        initPlayer(); // Player
        player->jump(0);

    //--- END Initialize Frame Player
}

Player* SimViDekont::getPlayer() { return player; }

VRObject* SimViDekont::getAnchor() { return root; }

OSG_END_NAMESPACE;
