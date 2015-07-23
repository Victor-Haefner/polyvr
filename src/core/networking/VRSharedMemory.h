#ifndef VRSHAREDMEMORY_H_INCLUDED
#define VRSHAREDMEMORY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSharedMemory {
    private:
        struct Segment;
        Segment* segment = 0;

    public:
        VRSharedMemory(string segment, bool init = true);
        ~VRSharedMemory();

        void* getPtr(string handle);
        string getHandle(void* data);

        template<class T>
        T* addObject(string name);
        template<class T>
        T getObject(string name);

        static void test();
};

OSG_END_NAMESPACE;

#endif // VRSHAREDMEMORY_H_INCLUDED
