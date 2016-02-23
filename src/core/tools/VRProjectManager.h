#ifndef VRPROJECTMANAGER_H_INCLUDED
#define VRPROJECTMANAGER_H_INCLUDED

#include "core/objects/geometry/VRSprite.h"
#include "core/objects/VRObjectFwd.h"
#include <vector>

OSG_BEGIN_NAMESPACE;

class VRProjectManager : public VRSprite {
    private:
        vector<VRStorageWeakPtr> vault;

        void initSite();

    public:
        VRProjectManager();

        static VRProjectManagerPtr create();

        void addItem(VRStoragePtr s);

        void save(string path);
        void load(string path);
};

OSG_END_NAMESPACE;

#endif // VRPROJECTMANAGER_H_INCLUDED
