#ifndef OCTREE_H_INCLUDED_COLL
#define OCTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "boundingbox.h"
#include "Partitiontree.h"

using namespace std;

OSG_BEGIN_NAMESPACE

template<class T>
class OctreeNode : public PartitiontreeNode<T> {
    public:
        typedef OctreeNode<T> Node;
        using PartitiontreeNode<T>::center;
        using PartitiontreeNode<T>::size;
        using PartitiontreeNode<T>::resolution;
        using PartitiontreeNode<T>::level;
        using PartitiontreeNode<T>::data;
        using PartitiontreeNode<T>::points;

    private:
        weak_ptr<Octree<T>> tree;
        Node* parent = 0;
        Node* children[8] = {0,0,0,0,0,0,0,0};

    public:
        OctreeNode(shared_ptr<Octree<T>> t, float r, float s = 10, int l = 0) : PartitiontreeNode<T>(r, s, l), tree(t) {}
        ~OctreeNode() { for (auto c : children) if (c) delete c; }

        Node* getParent();
        Node* getRoot();
        vector<Node*> getAncestry();
        void set(Node* node, Vec3d p, T data);
        Vec3d getLocalCenter();

        Node* add(Vec3d p, T data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        Node* extend(Vec3d p, int targetLevel = -1, bool checkPosition = true);
        Node* get(Vec3d p, bool checkPosition = true);
        Node* getChild(int i);

        bool isLeaf();

        vector<Node*> getChildren();
        vector<Node*> getSubtree();
        vector<Node*> getLeafs();
        vector<Node*> getPathTo(Vec3d p);

        vector<T> getAllData();

        int dataSize();
        size_t countNodes();
        int getDepth();

        int getOctant(Vec3d p);
        Vec3d lvljumpCenter(float s2, Vec3d rp);
        bool inBox(Vec3d p, Vec3d c, float size);

        //void destroy(Node* guard);
        void findInSphere(Vec3d p, float r, int d, vector<T>& res);
        void findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll);
        void findInBox(const Boundingbox& b, int d, vector<T>& res);

        void print(int indent = 0);
};

template<class T>
class Octree : public Partitiontree {
    public:
        typedef OctreeNode<T> Node;

    private:
        Node* root = 0;

    public:
        Octree(float r, float s = 10, string n = "") : Partitiontree(r, s, n) {}
        ~Octree() { if (root) delete root; }

        static shared_ptr<Octree<T>> create(float resolution, float size = 10, string name = "");
        shared_ptr<Octree<T>> ptr();

        Node* getRoot();
        Node* get(Vec3d p, bool checkPosition = true);
        vector<Node*> getAllLeafs();
        double getLeafSize();
        size_t getNodesCount();

        void setResolution(float res);
        float getSize();
        int getDepth();
        void clear();
        void updateRoot();

        void addBox(const Boundingbox& b, T data, int targetLevel = -1, bool checkPosition = true);
        Node* add(Vec3d p, T data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        Node* extend(Vec3d p, int targetLevel = -1, bool checkPosition = true);

        vector<T> getAllData();
        vector<T> radiusSearch(Vec3d p, float r, int d = -1);
        vector<Vec3d> radiusPointSearch(Vec3d p, float r, int d = -1, bool getAll = true);
        vector<T> boxSearch(const Boundingbox& b, int d = -1);

        void test();

        VRGeometryPtr getVisualization(bool onlyLeafes = false);
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
