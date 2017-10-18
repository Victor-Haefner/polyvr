#ifndef VROBJECTMANAGER_H_INCLUDED
#define VROBJECTMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
#include "core/utils/VRFunctionFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObjectManager : public VRObject {
    public:
        struct Entry : public VRName {
            PosePtr pos;
            string type;

            Entry(string name = "");
            ~Entry();

            static shared_ptr<Entry> create(string name = "");
            void set(PosePtr p, string t);
        };

    private:
        map<VRTransform*, VRTransformPtr> templates;
        map<string, VRTransformPtr> templatesByName;
        map<int, VRTransformWeakPtr> instances;
        map<string, shared_ptr<VRObjectManager::Entry> > entries;

        void setup();

    public:
        VRObjectManager(string name);
        ~VRObjectManager();
        static VRObjectManagerPtr create(string name = "objectManager");

        void addTemplate(VRTransformPtr s, string name = ""); // store object as template
        VRTransformPtr getTemplate(string name);
        vector<VRTransformPtr> getCatalog();

        VRTransformPtr add(VRTransformPtr s); // returns duplicate, first time the object is stored as template
        VRTransformPtr copy(string name, PosePtr p, bool addToStore = true); // returns duplicate
        void rem(VRTransformPtr o);
        void clear(bool clearTemplates = true);
        void updateObject(VRTransformPtr o);
};

OSG_END_NAMESPACE;

#endif // VROBJECTMANAGER_H_INCLUDED
