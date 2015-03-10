#ifndef VRSELECTOR_H_INCLUDED
#define VRSELECTOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMaterial.h>
#include <string>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;
class VRMaterial;
class VRGeometry;

class VRSelector {
    private:
        Vec3f color;

        map<VRGeometry*, VRMaterial*> orig_mats;
        VRObject* selection = 0;

        VRMaterial* getMat();

        void deselect();

    public:
        VRSelector();

        void select(VRObject* obj);
        VRObject* get();

        void setColor(Vec3f c);
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED
