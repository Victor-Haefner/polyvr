#ifndef VRLOD_H_INCLUDED
#define VRLOD_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class DistanceLOD; OSG_GEN_CONTAINERPTR(DistanceLOD);

class VRLod : public VRObject {
    private:
        DistanceLODRecPtr lod;
        bool decimate = false;
        Vec3d center;
        double scale = 1.0;
        string distances_string;
        unsigned int decimateNumber = 0;
        map<unsigned int, float> distances;
        map<unsigned int, VRObjectPtr> decimated;
        map<unsigned int, float> decimation;

        void setup();
        void loadSetup(VRStorageContextPtr context);
        void decimateGeometries(VRObjectPtr o, float f);

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> childs);

    public:
        /** initialise **/
        VRLod(string name = "0");
        ~VRLod();

        static VRLodPtr create(string name = "None");
        VRLodPtr ptr();

        void setCenter(Vec3d c);
        Vec3d getCenter();
        void setScale(double s);
        double getScale();
        void setDistance(unsigned int i, float dist);
        void addDistance(float dist);
        vector<float> getDistances();

        void addEmpty();

        void setDecimate(bool b, int N);
        bool getDecimate();
        int getDecimateNumber();
};

OSG_END_NAMESPACE;

#endif // VRLOD_H_INCLUDED
