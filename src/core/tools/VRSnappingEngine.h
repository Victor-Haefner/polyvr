#ifndef VRSNAPPINGENGINE_H_INCLUDED
#define VRSNAPPINGENGINE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGPlane.h>
#include <map>


using namespace std;
OSG_BEGIN_NAMESPACE;

class Octree;
class VRObject;
class VRTransform;
class VRGeometry;

class VRSnappingEngine {
    public:
        enum PRESET {
            SIMPLE_ALIGNMENT,
        };

    private:
        map<VRTransform*, Matrix> objects; // map objects to reference matrix
        Octree* positions = 0; // objects by positions
        Octree* distances = 0; // 0D snap
        Octree* lines = 0; // 1D snap
        Octree* planes = 0; // 2D snap
        Octree* orientations = 0; // objects by positions
        VRGeometry* hintGeo = 0;

        float influence_radius = 1000;
        float distance_snap = 0.05;
        bool doOrientation = false;
        bool showHints = false;

    public:
        VRSnappingEngine();

        void clear();

        void addObject(VRTransform* obj, float weight = 1);
        void addTree(VRObject* obj, float weight = 1);

        // snap object's position
        void addDistance(float dist, bool local = true, float weight = 1);
        void addAxis(Line line, bool local = true, float weight = 1);
        void addPlane(Plane plane, bool local = true, float weight = 1);

        // snap object's orientation
        void setOrientation(bool b, bool local = true, float weight = 1);

        void setVisualHints(bool b = true);
        void setPreset(PRESET preset);

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSNAPPINGENGINE_H_INCLUDED
