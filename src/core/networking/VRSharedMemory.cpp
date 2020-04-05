#include "VRSharedMemory.h"

#include <boost/interprocess/managed_shared_memory.hpp>
//#include <boost/interprocess/containers/vector.hpp>
//#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <cstdlib>

#include <OpenSG/OSGMatrix.h>

using namespace boost::interprocess;

struct Barrier {
    int threshold = 0;
    int count = 0;
    boost::interprocess::interprocess_mutex mutex;
    boost::interprocess::interprocess_condition condition;

    Barrier(string name, int count) : threshold(count), count(count) {}
    ~Barrier() {}

    void wait() {
        scoped_lock<interprocess_mutex> lock(mutex);
        if (--count == 0) {
            count = threshold;
            condition.notify_all();
        } else condition.wait(lock);
    }
};

VRSharedMemory::VRSharedMemory(string segment, bool init, bool remove) :
    mtx( boost::interprocess::open_or_create, (segment+"_mtx").c_str() ) {

    this->segment = new Segment();
    this->segment->name = segment;
    this->init = init;
    if (!init) return;
    if (remove) shared_memory_object::remove(segment.c_str());
    this->segment->memory = managed_shared_memory(open_or_create, segment.c_str(), 65536);
    unlock();
    int U = getObject<int>("__users__", 0);
    cout << "Access shared memory, segment: '" << segment << "', user ID: " << U << endl;
    setObject<int>("__users__", U+1);
}

VRSharedMemory::~VRSharedMemory() {
    if (!init) return;
    int U = getObject<int>("__users__")-1;
    setObject<int>("__users__", U);
    if (U == 0) {
        cout << " clear shared memory, segment: '" << segment->name << "'" << endl;
        shared_memory_object::remove(segment->name.c_str());
    }
}


void VRSharedMemory::lock() {
    try { mtx.lock(); }
    catch(interprocess_exception e) { cout << "VRSharedMemory::lock failed with: " << e.what() << endl; }
}

void VRSharedMemory::unlock() {
    try { mtx.unlock(); }
    catch(interprocess_exception e) { cout << "VRSharedMemory::unlock failed with: " << e.what() << endl; }
}

struct TestInt {
    int threshold = 0;
    int count = 0;
    int generation = 0;
    string name = "testint";

    TestInt(string n, int i) : count(i) { ; }
    ~TestInt() {}
};

void VRSharedMemory::addBarrier(string name, int count) {
    lock();
    segment->memory.construct<Barrier>(name.c_str())(name, count);
    unlock();
}

void VRSharedMemory::waitAt(string name) {
    lock();
    try {
        auto data = segment->memory.find<Barrier>(name.c_str());
        if (data.first) data.first->wait();
    } catch(interprocess_exception e) { cout << "SharedMemory::getObject failed with: " << e.what() << endl; }
    unlock();
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
    auto res = sm.getObject<float>(object);
    cout << "data " << res << endl;

    // vector example
    auto vec = sm.addVector<float>(vecn);
    vec->push_back(1.2);
    vec->push_back(1.3);
    vec->push_back(1.4);

    auto vres = sm.getVector<float>(vecn);
    for (auto v : vres) cout << v << endl;
}


