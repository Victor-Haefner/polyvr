#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <string>
#include <thread>

using namespace std;

class Thread {
    private:
        string name;
        thread t;
        bool* stopFlag = 0;

    public:
        template<typename Callable, typename... Args>
        explicit Thread(const string& n, Callable&& func, Args&&... args)
         : t(forward<Callable>(func), forward<Args>(args)...) {
            name = n;
        }

        void manageFlag(bool& flag) { stopFlag = &flag; }

        Thread();
        ~Thread();

        Thread(Thread&& __t) noexcept { swap(__t); }
        void swap(Thread& __t) noexcept { t.swap(__t.t); }
        Thread& operator=(Thread&& __t) noexcept { return *this; }

        bool joinable();
        void join();
        void detach();

        static void sleepMilli(int ms);
};

#endif // THREAD_H_INCLUDED
