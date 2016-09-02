#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class graph_base {
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

    protected:
        vector< vector<edge> > edges;

    public:
        graph_base();
        ~graph_base();

        static graph_basePtr create();

        void connect(int i, int j, CONNECTION c = SIMPLE);
        vector< vector<edge> >& getEdges();
};

template<class T>
class graph : public graph_base {
    private:
        vector<T> nodes;

    public:
        graph();
        ~graph();

        int addNode();
        int addNode(T t);

        vector<T>& getNodes();
        T& getNode(int i);
};

OSG_END_NAMESPACE;

#endif // GRAPH_H_INCLUDED
