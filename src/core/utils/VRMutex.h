#ifndef VRMUTEX_H_INCLUDED
#define VRMUTEX_H_INCLUDED

namespace OSG {

struct InnerMutex;

class VRMutex {
    private:
        InnerMutex* mutex = 0;

    public:
        VRMutex();
        ~VRMutex();

        InnerMutex* mtx();
};

class VRLock {
    private:
        InnerMutex* mutex = 0;

    public:
        VRLock(VRMutex& m);
        ~VRLock();
};

}

#endif // VRMUTEX_H_INCLUDED
