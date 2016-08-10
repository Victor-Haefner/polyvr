#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <vector>

using namespace std;

class graph {
    public:
        class node;
        class edge;

    private:
        vector<node> nodes;
        vector<edge> edges;

    public:
        graph();
        ~graph();

        int addNode();
        int connect(int i, int j);
};

#endif // GRAPH_H_INCLUDED
