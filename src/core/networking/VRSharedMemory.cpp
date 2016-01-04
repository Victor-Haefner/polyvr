#include "VRSharedMemory.h"

#include <boost/interprocess/managed_shared_memory.hpp>
//#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cstdlib>

#include <OpenSG/OSGMatrix.h>

using namespace OSG;
using namespace boost::interprocess;

VRSharedMemory::VRSharedMemory(string segment, bool init) {
    this->segment = new Segment();
    if (!init) return;
    shared_memory_object::remove(segment.c_str());
    this->segment->name = segment;
    this->segment->memory = managed_shared_memory(create_only, segment.c_str(), 65536);
}

VRSharedMemory::~VRSharedMemory() {
    shared_memory_object::remove(segment->name.c_str());
}

void* VRSharedMemory::getPtr(string h) {
    managed_shared_memory seg(open_only, segment->name.c_str());
    managed_shared_memory::handle_t handle = 0;
    stringstream ss; ss << h; ss >> handle;
    return seg.get_address_from_handle(handle);
}

string VRSharedMemory::getHandle(void* data) {
    managed_shared_memory seg(open_only, segment->name.c_str());
    managed_shared_memory::handle_t handle = seg.get_handle_from_address(data);
    stringstream ss; ss << handle;
    return ss.str();
}

void VRSharedMemory::test() {
    string segment = "bla";
    string object = "data";
    string vecn = "vdata";

    // object example
    VRSharedMemory sm(segment);
    float* data = sm.addObject<float>(object);
    *data = 5.6;
    float res = sm.getObject<float>(object);
    cout << "data " << res << endl;

    // vector example
    auto vec = sm.addVector<float>(vecn);
    vec->push_back(1.2);
    vec->push_back(1.3);
    vec->push_back(1.4);

    auto vres = sm.getVector<float>(vecn);
    for (auto v : vres) cout << v << endl;
}


