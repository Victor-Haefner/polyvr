#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <map>
#include <set>

using namespace std;

class Network {
    private:
        struct node {
            map<int, node*> out;
            map<int, node*> in;
            void* data;
            int ID;
        };

        map<int, node*> nodes;

    public:
        Network();

        int add();
        void set(int n, void* data);
        void* get(int n);

        void connect(int from, int to);
        void disconnect(int n1, int n2);
        void isolate(int n);
};

#endif // NETWORK_H_INCLUDED
