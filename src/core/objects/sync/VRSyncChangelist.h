#ifndef VRSYNCCHANGELIST_H_INCLUDED
#define VRSYNCCHANGELIST_H_INCLUDED

#include "core/networking/VRWebSocket.h"
#include <OpenSG/OSGBaseTypes.h>

class OSGChangeList;

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRSyncChangelist);

class VRSyncChangelist {
    private:

    public:
        VRSyncChangelist();
        ~VRSyncChangelist();
        static VRSyncChangelistPtr create();

        OSGChangeList* filterChanges(VRSyncNodePtr node);

        void printChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl);
        void broadcastChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl, bool doDelete = false);

        string getChangeType(UInt32 uiEntryDesc);
        string copySceneState(VRSyncNodePtr syncNode);
};

OSG_END_NAMESPACE;

#endif // VRSYNCCHANGELIST_H_INCLUDED
