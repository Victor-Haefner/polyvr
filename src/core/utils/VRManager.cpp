#include "VRManager.h"

template<class T>
VRManager<T>::VRManager(string name) {
    setNameSpace("Manager");
    setName(name);
}

template<class T>
vector< shared_ptr<T> > VRManager<T>::getData() {
    vector< shared_ptr<T> > res;
    for (auto n : data) res.push_back(n.second);
    return res;
}

template<class T> shared_ptr<T> VRManager<T>::add(string name) {
    auto t = T::create(name);
    data[t->getID()] = t;
    return t;
}

template<class T>
shared_ptr<T> VRManager<T>::get(string name) { for (auto d : data) if (d.second->getName() == name) return d.second; return 0; }
template<class T>
shared_ptr<T> VRManager<T>::get(int ID) { return data.count(ID) ? data[ID] : 0; }

template<class T>
void VRManager<T>::rem(shared_ptr<T> t) { rem(t->getID()); }
template<class T>
void VRManager<T>::rem(string name) { for (auto d : data) if (d.second->getName() == name) rem(d.first); }
template<class T>
void VRManager<T>::rem(int ID) { if (data.count(ID)) data.remove(ID); }
