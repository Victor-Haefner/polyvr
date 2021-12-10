#ifndef VRSYNCCHANGELIST_H_INCLUDED
#define VRSYNCCHANGELIST_H_INCLUDED

#include "core/networking/VRWebSocket.h"
#include <OpenSG/OSGBaseTypes.h>
#include <OpenSG/OSGFieldContainer.h>

class OSGChangeList;
struct SerialEntry;

OSG_BEGIN_NAMESPACE;
using namespace std;

class ChangeList;
class ContainerChangeEntry;

ptrFwd(VRSyncChangelist);
ptrFwd(VRSyncConnection);

class VRSyncChangelist {
    private:
        vector<FieldContainerRecPtr> justCreated; //IDs of the currently created nodes/children
        vector<unsigned char> CLdata;

    public:
        VRSyncChangelist();
        ~VRSyncChangelist();
        static VRSyncChangelistPtr create();

        OSGChangeList* filterChangeList(VRSyncNodePtr node, ChangeList* cl);
        OSGChangeList* filterChanges(VRSyncNodePtr node);

        void printChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl);
        void broadcastChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl, bool doDelete = false);

        string getChangeType(UInt32 uiEntryDesc);
        void sendSceneState(VRSyncNodePtr syncNode, VRSyncConnectionWeakPtr weakRemote);
        void broadcastSceneState(VRSyncNodePtr syncNode);

        vector<UInt32> getFCChildren(FieldContainer* fcPtr, BitVector fieldMask);
        bool filterFieldMask(VRSyncNodePtr syncNode, FieldContainer* fc, SerialEntry& sentry);
        void serialize_entry(VRSyncNodePtr syncNode, ContainerChangeEntry* entry, vector<unsigned char>& data);
        string serialize(VRSyncNodePtr syncNode, ChangeList* clist);

        void checkChildrenChange(FieldContainerRecPtr fcPtr, UInt32 fieldMask);
        void mergeChildrenChange(FieldContainerRecPtr fcPtr, UInt32 fieldMask);
        void fixNullChildren(FieldContainerRecPtr fcPtr, UInt32 fieldMask);
        void fixNullCore(FieldContainerRecPtr fcPtr, UInt32 fieldMask);

        void handleChildrenChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren, VRSyncConnectionWeakPtr weakRemote);
        void handleCoreChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, VRSyncConnectionWeakPtr weakRemote);
        void handleDestructed(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry);
        void handleGenericChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<unsigned char>>& fcData);
        FieldContainerRecPtr getOrCreate(VRSyncNodePtr syncNode, UInt32& id, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren, VRSyncConnectionWeakPtr weakRemote);
        void printDeserializedData(vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData);
        void handleRemoteEntries(VRSyncNodePtr syncNode, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData, VRSyncConnectionWeakPtr weakRemote);
        void deserializeEntries(vector<unsigned char>& data, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData);
        void deserializeChildrenData(vector<unsigned char>& childrenData, map<UInt32,vector<UInt32>>& parentToChildren);
        void deserializeAndApply(VRSyncNodePtr syncNode, VRSyncConnectionWeakPtr weakRemote);
        void gatherChangelistData(VRSyncNodePtr syncNode, string& data);
};

OSG_END_NAMESPACE;

#endif // VRSYNCCHANGELIST_H_INCLUDED
