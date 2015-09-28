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
        VRMaterial* getMat();

        VRObject* selection = 0;
        bool hasSubselection = false;
        map<int, int> subselection;

        void deselect();

    public:
        VRSelector();

        void select(VRObject* obj);
        VRObject* getSelection();

        void subselect(vector<int> verts, bool add);
        void clearSubselection();
        vector<int> getSubselection();

        void setColor(Vec3f c);
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED
