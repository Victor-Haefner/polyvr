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

class VRSyncChangelist {
    private:
        vector<FieldContainerRecPtr> justCreated; //IDs of the currently created nodes/children

    public:
        VRSyncChangelist();
        ~VRSyncChangelist();
        static VRSyncChangelistPtr create();

        OSGChangeList* filterChanges(VRSyncNodePtr node);

        void printChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl);
        void broadcastChangeList(VRSyncNodePtr syncNode, OSGChangeList* cl, bool doDelete = false);

        string getChangeType(UInt32 uiEntryDesc);
        string copySceneState(VRSyncNodePtr syncNode);

        vector<UInt32> getFCChildren(FieldContainer* fcPtr, BitVector fieldMask);
        void filterFieldMask(VRSyncNodePtr syncNode, FieldContainer* fc, SerialEntry& sentry);
        void serialize_entry(VRSyncNodePtr syncNode, ContainerChangeEntry* entry, vector<unsigned char>& data, UInt32 syncNodeID);
        string serialize(VRSyncNodePtr syncNode, ChangeList* clist);

        void handleChildrenChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren);
        void handleCoreChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry);
        void handleGenericChange(VRSyncNodePtr syncNode, FieldContainerRecPtr fcPtr, SerialEntry& sentry, map<UInt32, vector<unsigned char>>& fcData);
        FieldContainerRecPtr getOrCreate(VRSyncNodePtr syncNode, UInt32& id, SerialEntry& sentry, map<UInt32, vector<UInt32>>& parentToChildren);
        void printDeserializedData(vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData);
        void handleRemoteEntries(VRSyncNodePtr syncNode, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData);
        void deserializeEntries(string& data, vector<SerialEntry>& entries, map<UInt32, vector<UInt32>>& parentToChildren, map<UInt32, vector<unsigned char>>& fcData);
        void deserializeChildrenData(vector<unsigned char>& childrenData, map<UInt32,vector<UInt32>>& parentToChildren);
        void deserializeAndApply(VRSyncNodePtr syncNode, string& data);
};

OSG_END_NAMESPACE;

#endif // VRSYNCCHANGELIST_H_INCLUDED
