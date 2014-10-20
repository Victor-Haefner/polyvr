#ifndef VRGROUP_H_INCLUDED
#define VRGROUP_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <OpenSG/OSGTransform.h>             // For geometry transform
#include <OpenSG/OSGNode.h>                  // Using NodeRecPtr

#include "core/objects/object/VRObject.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGroup : public VRObject {
    private:
        string group;
        bool active;
        static map<string, vector<VRGroup*>* > groups;
        static map<string, VRObject* > templates;

        VRObject* copy(vector<VRObject*> children);

    protected:
        void saveContent(xmlpp::Element* e);
        void loadContent(xmlpp::Element* e);

    public:
        VRGroup(string name);
        ~VRGroup();

        /** Returns the group **/
        void setGroup(string g);
        string getGroup();

        void destroy();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);

        bool getActive();
        void setActive(bool b);

        void sync();
        void apply();

        static vector<string> getGroups();
        static void clearGroups();

        vector<VRGroup*>* getGroupObjects();
};

OSG_END_NAMESPACE;

#endif // VRGROUP_H_INCLUDED
