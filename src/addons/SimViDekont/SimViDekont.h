#ifndef SIMVIDEKONT_H_INCLUDED
#define SIMVIDEKONT_H_INCLUDED

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include <OpenSG/OSGSwitch.h>

#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMaterialChunk.h>
#include <OpenSG/OSGPointChunk.h>
#include <OpenSG/OSGGeoProperties.h>

#include "core/scene/VRSceneLoader.h"

#include "VRabq.h"
#include "GeoIO.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Player;
class VRDevice;

class SimViDekont : public VRabq {
    private:
        VRObjectPtr root;
        Player* player;

        Color3f getColor(float v);
        Color3f getColor2(float v);

        bool isElementValid(int i, frame* f);
        bool isElementValid0(int i, frame* f);
        /**
        * Returns a VRGeometry of a given frame.
        */
        void createGeo(frame* f);
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

        void quadOnSurface(map<int, float>& Vstress, map<int, int>* vmap, map<int, int>* vmap2, int* id, GeoUInt32PropertyTransitPtr Indices, int a, int b, int c, int d);
        void computeVertexColors(frame* f, map<int, int>* vmap, map<int, float>& VsValue, bool stress = true);

        void showFrame(int i);
        void initFrameGeometries();

        void playForward(VRDevicePtr dev);
        void playBackward(VRDevicePtr dev);
        void playStop(VRDevicePtr dev);
        void playStepForward(VRDevicePtr dev);
        void playStepBackward(VRDevicePtr dev);

        void toggleColors(VRDevicePtr dev);
        void initPlayer();

    public:
        SimViDekont();
        Player* getPlayer();
        VRObjectPtr getAnchor();
};

OSG_END_NAMESPACE;

#endif // SIMVIDEKONT_H_INCLUDED
