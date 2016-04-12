#ifndef VRPROJECTMANAGER_H_INCLUDED
#define VRPROJECTMANAGER_H_INCLUDED

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include <vector>

OSG_BEGIN_NAMESPACE;

class VRProjectManager : public VRObject {
    private:
        vector<VRStoragePtr> vault_reload;
        vector<VRStoragePtr> vault_rebuild;

    public:
        VRProjectManager();

        static VRProjectManagerPtr create();

        void addItem(VRStoragePtr s, string mode);
        vector<VRStoragePtr> getItems();

        void save(string path);
        void load(string path);
};

OSG_END_NAMESPACE;

#endif // VRPROJECTMANAGER_H_INCLUDED
