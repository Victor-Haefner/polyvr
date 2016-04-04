#include "VRManager.h"
#include "VRStorage_template.h"

template<class T>
VRManager<T>::VRManager(string name) {
    setNameSpace("Manager");
    setName(name);

    storeMap("", &data);
}

template<class T>
int VRManager<T>::getFreeID() {
    int ID = 0;
    for (auto n : data) if (n.first == ID) ID++;
    return ID;
}

template<class T>
vector<int> VRManager<T>::getIDs() {
    vector<int> res;
    for (auto n : data) res.push_back(n.first);
    return res;
}

template<class T>
vector< shared_ptr<T> > VRManager<T>::getData() {
    vector< shared_ptr<T> > res;
    for (auto n : data) res.push_back(n.second);
    return res;
}

template<class T> shared_ptr<T> VRManager<T>::add(string name) {
    auto t = T::create(name);
    data[getFreeID()] = t;
    return t;
}

template<class T> shared_ptr<T> VRManager<T>::add(string name, int key) {
    auto t = T::create(name);
    data[key] = t;
    return t;
}

template<class T>
shared_ptr<T> VRManager<T>::get(string name) { for (auto d : data) if (d.second->getName() == name) return d.second; return 0; }
template<class T>
shared_ptr<T> VRManager<T>::get(int ID) { return data.count(ID) ? data[ID] : 0; }

template<class T>
void VRManager<T>::rem(shared_ptr<T> t) { rem(t->getID()); }
template<class T>
void VRManager<T>::rem(int ID) { if (data.count(ID)) data.erase(ID); }
template<class T>
void VRManager<T>::rem(string name) {
    vector<int> ids;
    for (auto d : data) if (d.second->getName() == name) ids.push_back(d.first);
    for (auto i : ids) rem(i);
}
