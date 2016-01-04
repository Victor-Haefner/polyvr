#ifndef VRSHAREDMEMORY_H_INCLUDED
#define VRSHAREDMEMORY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>
#include <vector>
//#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSharedMemory {
    private:
        struct Segment {
            string name;
            boost::interprocess::managed_shared_memory memory;
        };
        Segment* segment = 0;

    public:
        VRSharedMemory(string segment, bool init = true);
        ~VRSharedMemory();

        void* getPtr(string handle);
        string getHandle(void* data);

        template<class T>
        T* addObject(string name) {
            T* data = segment->memory.construct<T>(name.c_str())(0);
            return data;
        }

        template<class T>
        T getObject(string name) {
            boost::interprocess::managed_shared_memory seg(boost::interprocess::open_only, segment->name.c_str());
            auto data = seg.find<T>(name.c_str());
            if (data.first) return *data.first;
            return T();
        }

        template<class T> vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> >*
        addVector(string name) {
            using memal = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;
            using memvec = vector<T, memal>;
            const memal alloc_inst(segment->memory.get_segment_manager());
            return segment->memory.construct<memvec>(name.c_str())(alloc_inst);
        }

        template<class T> vector<T>
        getVector(string name) {
            using memal = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;
            using memvec = vector<T, memal>;
            boost::interprocess::managed_shared_memory seg(boost::interprocess::open_only, segment->name.c_str());
            auto data = seg.find<memvec>(name.c_str());
            memvec* res = data.first;

            vector<T> vres;
            if (res) {
                vres.reserve(res->size());
                copy(res->begin(),res->end(),back_inserter(vres));
            }
            return vres;
        }

        /*template<class T> T* addObject(string name);
        template<class T> T getObject(string name);

        template<class T> vector<T, boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> >*
        addVector(string name);
        template<class T> vector<T> getVector(string name);*/

        static void test();
};

OSG_END_NAMESPACE;

#endif // VRSHAREDMEMORY_H_INCLUDED
