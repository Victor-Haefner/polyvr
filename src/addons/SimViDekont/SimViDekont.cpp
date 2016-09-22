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

    if((abs(delta[0]) > 2.45e-4) || (abs(delta[1])> 2.45e-4)
       || (abs(delta[2])> 2.45e-4) ) return false;

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



    // go through the point ids && count the elements they belong to
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
        if ((*vmap)[i] == 0 || (*vmap)[i] == 8) continue;

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

    GeometryMTRecPtr geo = Geometry::create();
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

    f->geo = VRGeometry::create("myNewMesh");
    f->geo->setMesh( OSGGeometry::create(geo) );
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
    if (f->stress->size() == 0 || f->strain->size() == 0 ) return;
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
    frames[last]->geo->hide();
    frames[i]->geo->show();
    last = i;
    //cout << "\nGoto " << i << "  " << frames->size() << flush;
}

void SimViDekont::initFrameGeometries() {
    for (auto frame : frames) {
        cout << "\nCONSTRUCT SURFACE " << frame->id << endl;

        string path = "rslt/geo_frame";

        bool file = false;
        file =  GeoIO::load(frame, path);

        if(!file) createGeo(frame);

        root->addChild(frame->geo);
        frame->geo->hide();
        frame->geo->translate(Vec3f(0,1,0));
    }
}

//--- BEGIN Player binding

//--- BEGIN Player callbacks

void SimViDekont::playForward(VRDevicePtr dev) { player->play(); }
void SimViDekont::playBackward(VRDevicePtr dev) { player->play(true); }
void SimViDekont::playStop(VRDevicePtr dev) { player->stop(); }
void SimViDekont::playStepForward(VRDevicePtr dev) { player->step(); }
void SimViDekont::playStepBackward(VRDevicePtr dev) { player->step(true); }

void SimViDekont::toggleColors(VRDevicePtr dev) {
    for (auto frame : frames) {
        GeometryMTRecPtr geo = frame->geo->getMesh()->geo;
        GeoVec3fPropertyRecPtr colors1 = dynamic_cast<GeoVec3fProperty*>(geo->getColors());
        GeoVec3fPropertyRecPtr colors2 = dynamic_cast<GeoVec3fProperty*>(geo->getSecondaryColors());
        geo->setColors(colors2);
        geo->setSecondaryColors(colors1);
    }
}


//--- END Player callbacks
void SimViDekont::initPlayer() {
    VRFunction<int>* cb = new VRFunction<int>("showFrame", boost::bind(&SimViDekont::showFrame, this, _1));
    player = new Player(cb, 0, frames.size() );
}

//--- END Player binding


SimViDekont::SimViDekont() {

    /* Workflow:

    1: Load all frames from Binary file in frames vector
    2: Get a VRGeometry for each frame
    3: Get device signals, keyboard && mouse/flystick
    4: start Player
    5: Enjoy

    */

    root = VRObject::create("SimViDekont");
    loadAbaqusFile(0, 100); // load odb file && parse data to frames
    initFrameGeometries(); // create geometries for each frame && hide it

    // Frame Player
    initPlayer();
    player->jump(0);
}

Player* SimViDekont::getPlayer() { return player; }

VRObjectPtr SimViDekont::getAnchor() { return root; }

OSG_END_NAMESPACE;
