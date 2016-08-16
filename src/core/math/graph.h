#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <vector>

using namespace std;

template<class T>
class graph {
    public:
        enum CONNECTION {
            SIMPLE,
            HIERARCHY,
            SIBLING
        };

        struct edge {
            int from;
            int to;
            CONNECTION connection;

            edge(int i, int j, CONNECTION c);
        };

    private:
        vector<T> nodes;
        vector<vector<edge>> edges;

    public:
        graph();
        ~graph();

        int addNode();
        int addNode(T t);
        void connect(int i, int j, CONNECTION c = SIMPLE);

        vector<T>& getNodes();
        vector<vector<edge>>& getEdges();
        T& getNode(int i);
};

#endif // GRAPH_H_INCLUDED
