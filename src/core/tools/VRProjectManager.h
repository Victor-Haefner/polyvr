#ifndef VRPROJECTMANAGER_H_INCLUDED
#define VRPROJECTMANAGER_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include <vector>

OSG_BEGIN_NAMESPACE;

class VRProjectManager : public VRObject {
    private:
        vector<VRStorageWeakPtr> vault;

        int reload_persistency = 1;
        int rebuild_persistency = 2;

    public:
        VRProjectManager();

        static VRProjectManagerPtr create();

        void setPersistencies(int reload, int rebuild);
        void addItem(VRStoragePtr s);

        void save(string path);
        void load(string path);
};

OSG_END_NAMESPACE;

#endif // VRPROJECTMANAGER_H_INCLUDED
