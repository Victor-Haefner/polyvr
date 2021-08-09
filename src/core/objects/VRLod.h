#ifndef VRLOD_H_INCLUDED
#define VRLOD_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class DistanceLOD; OSG_GEN_CONTAINERPTR(DistanceLOD);

ptrFctFwd( VRLod, VRLodEventPtr );

class VRLodEvent {
    private:
        int current;
        int last;
        VRLodWeakPtr lod;

    public:
        VRLodEvent(int c, int l, VRLodPtr lod);
        VRLodEvent() {}

        static VRLodEventPtr create(int c, int l, VRLodPtr lod);

        int getCurrent();
        int getLast();
        VRLodPtr getLod();
};

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

        shared_ptr<function<void(int,int)>> onLODSwitchCb;
        VRLodCbPtr userCb;

        void setup();
        void loadSetup(VRStorageContextPtr context);
        void decimateGeometries(VRObjectPtr o, float f);
        void onLODSwitch(int current, int last);

    protected:
        VRObjectPtr copy(vector<VRObjectPtr> childs) override;

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
        float getDistance(unsigned int i);
        void addDistance(float dist);
        vector<float> getDistances();

        void setCallback(VRLodCbPtr cb);

        void addEmpty();

        void setDecimate(bool b, int N);
        bool getDecimate();
        int getDecimateNumber();
};

OSG_END_NAMESPACE;

#endif // VRLOD_H_INCLUDED
