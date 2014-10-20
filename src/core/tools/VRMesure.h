#ifndef VRMESURE_H_INCLUDED
#define VRMESURE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMaterial.h>
#include <string>

template<class T> class VRFunction;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBillboard;
class VRGeometry;
class VRScene;
class VRDevice;
class VRSignal;

class VRMesure {
    private:
        VRGeometry* s1;//spheren die mit drag and drop plaziert werden
        VRGeometry* s2;
        VRGeometry* l;

        VRBillboard* display;
        //VRSprite* sprite;
        VRFunction<int>* fkt;

        VRScene* scene;

        //VRTransform* dummy;

        string convertToString(float f, int p);

        void processBar(Vec3f p1, Vec3f p2);

        void processLabel(Vec3f p1, Vec3f p2, Vec3f cpos);

        void check();

        MaterialRecPtr setTransMat();

        void _kill();

    public:
        VRMesure();

        void setKillSignal(VRDevice* dev, VRSignal* sig);

        void addToScene(VRScene* _scene);

        void setPosition(Vec3f pos);

        void kill(VRDevice* dev = 0);
};

OSG_END_NAMESPACE;

#endif // VRMESURE_H_INCLUDED
