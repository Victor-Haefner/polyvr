#ifndef VRMANAGER_H_INCLUDED
#define VRMANAGER_H_INCLUDED

#include <map>
#include "VRName.h"

template<class T>
class VRManager : public VRName {
    protected:
        map<int, shared_ptr<T> > data;
        vector<int> getIDs();

    protected:
        int getFreeID();

        shared_ptr<T> get(string name);
        shared_ptr<T> get(int ID);

    public:
        VRManager(string name);

        vector< shared_ptr<T> > getData();
        virtual shared_ptr<T> add(string name = "");
        virtual shared_ptr<T> add(string name, int key);

        void rem(shared_ptr<T> t);
        void rem(string name);
        void rem(int ID);
};

#endif // VRMANAGER_H_INCLUDED
