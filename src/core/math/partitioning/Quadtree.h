#ifndef QUADTREE_H_INCLUDED_COLL
#define QUADTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "boundingbox.h"
#include "Partitiontree.h"

using namespace std;

OSG_BEGIN_NAMESPACE

class QuadtreeNode : public PartitiontreeNode {
    private:
        QuadtreeWeakPtr tree;
        QuadtreeNode* parent = 0;
        QuadtreeNode* children[8] = {0,0,0,0,0,0,0,0};

    public:
        QuadtreeNode(QuadtreePtr tree, float resolution, float size = 10, int level = 0);
        ~QuadtreeNode();

        QuadtreeNode* getParent();
        QuadtreeNode* getRoot();
        vector<QuadtreeNode*> getAncestry();
        void set(QuadtreeNode* node, Vec3d p, void* data);
        float getSize();
        float getResolution();
        Vec3d getCenter();
        Vec3d getLocalCenter();

        void setResolution(float res);

        QuadtreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        QuadtreeNode* get(Vec3d p, bool checkPosition = true);

        void remData(void* data);
        //void clear();

        bool isLeaf();

        vector<QuadtreeNode*> getChildren();
        vector<QuadtreeNode*> getSubtree();
        vector<QuadtreeNode*> getLeafs();
        vector<QuadtreeNode*> getPathTo(Vec3d p);

        vector<void*> getData();
        vector<void*> getAllData();
        vector<Vec3d> getPoints();

        int dataSize();
        void* getData(int i);
        Vec3d getPoint(int i);

        template<class T>
        void delContent() {
            for (void* o : data) delete (T*)o;
            points.clear();
            data.clear();
        }

        //void destroy(QuadtreeNode* guard);
        void findInSphere(Vec3d p, float r, int d, vector<void*>& res);
        void findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll);
        void findInBox(const Boundingbox& b, int d, vector<void*>& res);
        int getOctant(Vec3d p);
        bool inBox(Vec3d p, Vec3d c, float size);

        void print(int indent = 0);
        string toString(int indent = 0);
};

class Quadtree : public Partitiontree {
    private:
        QuadtreeNode* root = 0;

        Quadtree(float resolution, float size = 10, string name = "");

    public:
        ~Quadtree();
        static QuadtreePtr create(float resolution, float size = 10, string name = "");
        QuadtreePtr ptr();

        QuadtreeNode* getRoot();
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        QuadtreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        QuadtreeNode* get(Vec3d p, bool checkPosition = true);
        vector<QuadtreeNode*> getAllLeafs();

        void setResolution(float res);
        float getSize();
        void clear();
        void updateRoot();

        template<class T>
        void delContent() {
            for (void* o : getAllData()) delete (T*)o;
        }

        vector<void*> getAllData();
        vector<void*> radiusSearch(Vec3d p, float r, int d = -1);
        vector<Vec3d> radiusPointSearch(Vec3d p, float r, int d = -1, bool getAll = true);
        vector<void*> boxSearch(const Boundingbox& b, int d = -1);

        void test();

        VRGeometryPtr getVisualization(bool onlyLeafes = false);
};

OSG_END_NAMESPACE

#endif // QUADTREE_H_INCLUDED
