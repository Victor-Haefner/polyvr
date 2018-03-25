#ifndef VRPROJECTSLIST_H_INCLUDED
#define VRPROJECTSLIST_H_INCLUDED

#include "VRSceneFwd.h"
#include "core/utils/VRStorage.h"
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRProjectEntry : public VRStorage {
    private:
        string path;
        string timestamp;

    public:
        VRProjectEntry(string path, string timestamp);
        ~VRProjectEntry();
        static VRProjectEntryPtr create(string path, string timestamp);
        static VRProjectEntryPtr create(string path);

        void set(string path, string timestamp);

        string getName();
        string getPath();
        long getTimestamp();

        void setTimestamp(string t);
};

class VRProjectsList : public VRStorage {
    private:
        map<string, VRProjectEntryPtr> entries;

    public:
        VRProjectsList();
        ~VRProjectsList();
        static VRProjectsListPtr create();

        int size();
        void clear();

        void addEntry(VRProjectEntryPtr e);
        bool hasEntry(string path);
        void remEntry(string path);
        vector<string> getPaths();
        VRProjectEntryPtr getEntry(string path);
        map<string, VRProjectEntryPtr> getEntries();
        vector<VRProjectEntryPtr> getEntriesByTimestamp();
};

OSG_END_NAMESPACE;

#endif // VRPROJECTSLIST_H_INCLUDED
