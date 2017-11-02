#include "virtuoseAPI.h"

#include <iostream>
#include <unistd.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace std;
using namespace boost::interprocess;

class SharedMemory {
    private:
        struct Segment {
            string name;
            managed_shared_memory memory;
        };
        Segment* segment = 0;
        bool init = false;

    public:
        SharedMemory(string segment, bool init = true) {
            this->segment = new Segment();
            this->segment->name = segment;
            this->init = init;
            if (!init) return;
            shared_memory_object::remove(segment.c_str());
            this->segment->memory = managed_shared_memory(create_only, segment.c_str(), 65536);
            unlock();
        }

        ~SharedMemory() {
            if (!init) return;
            shared_memory_object::remove(segment->name.c_str());
        }

        void* getPtr(string h) {
            managed_shared_memory seg(open_only, segment->name.c_str());
            managed_shared_memory::handle_t handle = 0;
            stringstream ss; ss << h; ss >> handle;
            return seg.get_address_from_handle(handle);
        }

        string getHandle(void* data) {
            managed_shared_memory seg(open_only, segment->name.c_str());
            managed_shared_memory::handle_t handle = seg.get_handle_from_address(data);
            stringstream ss; ss << handle;
            return ss.str();
        }

        void lock() {
            try {
                named_mutex mtx{ open_or_create, (segment->name+"_mtx").c_str() };
                mtx.lock();
            } catch(interprocess_exception e) { cout << "SharedMemory::lock failed with: " << e.what() << endl; }
        }

        void unlock() {
            try {
                named_mutex mtx{ open_or_create, (segment->name+"_mtx").c_str() };
                mtx.unlock();
            } catch(interprocess_exception e) { cout << "SharedMemory::unlock failed with: " << e.what() << endl; }
        }

        template<class T>
        T* addObject(string name) {
            lock();
            T* data = segment->memory.construct<T>(name.c_str())();
            unlock();
            return data;
        }

        template<class T>
        T getObject(string name, T t) {
            lock();
            try {
                managed_shared_memory seg(open_only, segment->name.c_str());
                auto data = seg.find<T>(name.c_str());
                if (data.first) {
                    T res = *data.first;
                    unlock();
                    return res;
                }
            } catch(interprocess_exception e) { cout << "SharedMemory::getObject failed with: " << e.what() << endl; }
            unlock();
            return t;
        }

        template<class T>
        T getObject(string name) {
            lock();
            try {
                managed_shared_memory seg(open_only, segment->name.c_str());
                auto data = seg.find<T>(name.c_str());
                if (data.first) {
                    T res = *data.first;
                    unlock();
                    return res;
                }
            } catch(interprocess_exception e) { cout << "SharedMemory::getObject failed with: " << e.what() << endl; }
            unlock();
            return T();
        }

        template<class T>
        void setObject(string name, T t) {
            lock();
            try {
                managed_shared_memory seg(open_only, segment->name.c_str());
                auto data = seg.find<T>(name.c_str());
                if (data.first) {
                    *data.first = t;
                    unlock();
                } else {
                    auto o = addObject<T>(name);
                    *o = t;
                }
            } catch(interprocess_exception e) { cout << "SharedMemory::getObject failed with: " << e.what() << endl; }
            unlock();
        }

        template<class T>
        bool hasObject(const string& name) {
            lock();
            try {
                boost::interprocess::managed_shared_memory seg(boost::interprocess::open_only, segment->name.c_str());
                auto data = seg.find<T>(name.c_str());
                if (data.first) { unlock(); return true; }
            } catch(boost::interprocess::interprocess_exception e) {}
            unlock();
            return false;
        }

        template<class T> vector<T, boost::interprocess::allocator<T, managed_shared_memory::segment_manager> >*
        addVector(string name) {
            using memal = boost::interprocess::allocator<T, managed_shared_memory::segment_manager>;
            using memvec = vector<T, memal>;
            const memal alloc_inst(segment->memory.get_segment_manager());
            return segment->memory.construct<memvec>(name.c_str())(alloc_inst);
        }

        template<class T> vector<T>
        getVector(string name) {
            using memal = boost::interprocess::allocator<T, managed_shared_memory::segment_manager>;
            using memvec = vector<T, memal>;
            vector<T> vres;

            lock();
            try {
                managed_shared_memory seg(open_only, segment->name.c_str());
                auto data = seg.find<memvec>(name.c_str());
                memvec* res = data.first;
                if (res) {
                    vres.reserve(res->size());
                    copy(res->begin(),res->end(),back_inserter(vres));
                }
            } catch(interprocess_exception e) { cout << "getVector failed with: " << e.what() << endl; }
            unlock();

            return vres;
        }
};

SharedMemory interface("virtuose");

struct Vec9 {
    float data[9] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
};

struct Vec7 {
    float data[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
};

struct Vec6 {
    float data[6] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
};

template <typename T>
void print(const T& t, int N) {
    cout << " trans: ";
    for (int i=0; i<N; i++) cout << t.data[i] << " ";
    cout << endl;
}

int main() {
    interface.addObject<Vec7>("position");
    interface.setObject<bool>("run", true);
    auto vc = virtOpen("172.22.151.200");
    //cout << " error code 2: " << virtGetErrorMessage(2) << endl;

    Vec7 identity;

    virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION);
    virtSetSpeedFactor(vc, 1);
    virtSetForceFactor(vc, 1);
    virtSetTimeStep(vc, 0.02);
    virtSetBaseFrame(vc, identity.data);
    virtSetObservationFrame(vc, identity.data);
    virtSetCommandType(vc, COMMAND_TYPE_VIRTMECH);
    virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP);
    virtEnableForceFeedback(vc, 1);
    virtSetPowerOn(vc, 1);

    float K[] = { 10, 10, 10, 1000, 1000, 1000 };
    float B[] = { 0.1, 0.1, 0.1, 10, 10, 10 };
    virtAttachQSVO(vc, K, B);

    Vec6 forces;
    Vec7 position;

    bool attached = false;
    bool run = true;
    do {
        interface.lock();
        run = interface.getObject<bool>("run", false);
		virtGetAvatarPosition(vc, position.data);
		interface.setObject<Vec7>("position", position);

		int shifting = 0;
        int power = 0;
        virtIsInShiftPosition(vc, &shifting);
        virtGetPowerOn(vc, &power);
        interface.setObject<bool>("shifting", bool(shifting || power));
        interface.unlock();

        if (interface.hasObject<Vec6>("targetForces")) {
            auto forces = interface.getObject<Vec6>("targetForces");
            virtAddForce(vc, forces.data);
        }

        //cout << "\nattached " << attached << " hasTargetSpeed " << interface.hasObject<Vec6>("targetSpeed") << " targetPosition " << interface.hasObject<Vec7>("targetPosition");
        if (attached && interface.hasObject<Vec6>("targetSpeed") && interface.hasObject<Vec7>("targetPosition")) {
            auto targetSpeed = interface.getObject<Vec6>("targetSpeed");
            auto targetPosition = interface.getObject<Vec7>("targetPosition");
            float tmpPos[7];
            virtGetPosition(vc, tmpPos);
            for(int i = 0; i < 7 ; i++) tmpPos[i] = targetPosition.data[i] - tmpPos[i];
            virtSetPosition(vc, targetPosition.data);

            float tmpSp[6];
            virtGetSpeed(vc, tmpSp);
            for(int i = 0; i < 6 ; i++) tmpSp[i] = targetSpeed.data[i] - tmpSp[i];
            virtSetSpeed(vc, targetSpeed.data);

            print(targetPosition, 7);

            //pPos = Vec3d(tmpPos[0],tmpPos[1],tmpPos[2]);
            //sPos = Vec3d(tmpSp[0],tmpSp[1],tmpSp[2]);
            //sRot = Vec3d(tmpSp[0],tmpSp[1],tmpSp[2]);
        }

		virtGetForce(vc, forces.data);
		interface.setObject<Vec6>("forces", forces);

        // cmds
        //cout << "\n has doAttach " << interface.hasObject<bool>("doAttach") << endl;
        if (interface.getObject<bool>("doAttach", false)) {
            cout << " --- virtuose attach!! ---\n";
            interface.setObject<bool>("doAttach", false);
            auto mass = interface.getObject<float>("mass");
            auto inertia = interface.getObject<Vec9>("inertia");
            virtAttachVO(vc, mass, inertia.data);
            attached = true;
        }

        //float position[7] = {0.0,0.0,0.0,0.0,0.0,0.0,1.0};
        //float speed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
		//virtSetPosition(vc, position);
		//virtSetSpeed(vc, speed);
		usleep(2000);
    } while (run);

    virtSetPowerOn(vc, 0);
    virtDetachVO(vc);
    virtStopLoop(vc);
    virtClose(vc);
    return 0;
}
