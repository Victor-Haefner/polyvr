#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRStorage.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Graph : public VRStorage {
    public:
        enum CONNECTION {
            SIMPLE = 0,
            HIERARCHY = 1,
            DEPENDENCY = 2,
            SIBLING = 3
        };

        struct node {
            boundingbox box;
        };

        struct edge {
            int from = 0;
            int to = 0;
            CONNECTION connection = SIMPLE;

            edge(int i = 0, int j = 0, CONNECTION c = SIMPLE);
        };

    protected:
        vector< vector<edge> > edges;
        vector< node > nodes;

    public:
        Graph();
        ~Graph();

        static shared_ptr< Graph > create() { return shared_ptr< Graph >(new Graph()); }

        edge& connect(int i, int j, CONNECTION c = SIMPLE);
        void disconnect(int i, int j);
        node& getNode(int i);
        vector< node >& getNodes();
        vector< vector<edge> >& getEdges();
        int getNEdges();
        int size();
        bool connected(int i1, int i2);
        void setPosition(int i, Vec3f v);
        Vec3f getPosition(int i);

        virtual int addNode();
        virtual void remNode(int i);
        virtual void update(int i, bool changed);
        virtual void clear();
};

OSG_END_NAMESPACE;

#endif // GRAPH_H_INCLUDED
