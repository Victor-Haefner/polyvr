#ifndef VRSHAREDMEMORY_H_INCLUDED
#define VRSHAREDMEMORY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <vector>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSharedMemory {
    private:
        size_t size = 0;

    public:
        VRSharedMemory();

        void allocate(size_t N);
        void deallocate();

        void* get(size_t offset);
        void write(size_t offset, void* data);
};

OSG_END_NAMESPACE;

#endif // VRSHAREDMEMORY_H_INCLUDED
