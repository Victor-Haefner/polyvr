#ifndef PARTITIONTREE_H_INCLUDED_COLL
#define PARTITIONTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "boundingbox.h"

using namespace std;

OSG_BEGIN_NAMESPACE

class PartitiontreeNode {
    protected:
        float resolution = 0.1;
        float size = 10;
        int level = 0;

        Vec3d center;

        PartitiontreeWeakPtr tree;
        PartitiontreeNode* parent = 0;
        PartitiontreeNode* children[8] = {0,0,0,0,0,0,0,0};

        vector<void*> data;
        vector<Vec3d> points;

        Vec3d lvljumpCenter(float s2, Vec3d rp);

    public:
        PartitiontreeNode(PartitiontreePtr tree, float resolution, float size = 10, int level = 0);
        ~PartitiontreeNode();

        PartitiontreeNode* getParent();
        PartitiontreeNode* getRoot();
        vector<PartitiontreeNode*> getAncestry();
        void set(PartitiontreeNode* node, Vec3d p, void* data);
        float getSize();
        float getResolution();
        Vec3d getCenter();
        Vec3d getLocalCenter();

        void setResolution(float res);

        PartitiontreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        PartitiontreeNode* get(Vec3d p, bool checkPosition = true);

        void remData(void* data);
        //void clear();

        bool isLeaf();

        vector<PartitiontreeNode*> getChildren();
        vector<PartitiontreeNode*> getSubtree();
        vector<PartitiontreeNode*> getLeafs();
        vector<PartitiontreeNode*> getPathTo(Vec3d p);

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

        //void destroy(PartitiontreeNode* guard);
        void findInSphere(Vec3d p, float r, int d, vector<void*>& res);
        void findPointsInSphere(Vec3d p, float r, int d, vector<Vec3d>& res, bool getAll);
        void findInBox(const Boundingbox& b, int d, vector<void*>& res);
        int getOctant(Vec3d p);
        bool inBox(Vec3d p, Vec3d c, float size);

        void print(int indent = 0);
        string toString(int indent = 0);
};

class Partitiontree : public std::enable_shared_from_this<Partitiontree> {
    protected:
        float resolution = 0.1;
        float firstSize = 10;
        PartitiontreeNode* root = 0;

        Partitiontree(float resolution, float size = 10, string name = "");

    public:
        string name;

    public:
        ~Partitiontree();
        static PartitiontreePtr create(float resolution, float size = 10, string name = "");
        PartitiontreePtr ptr();

        PartitiontreeNode* getRoot();
        void addBox(const Boundingbox& b, void* data, int targetLevel = -1, bool checkPosition = true);
        PartitiontreeNode* add(Vec3d p, void* data, int targetLevel = -1, bool checkPosition = true, int partitionLimit = -1);
        PartitiontreeNode* get(Vec3d p, bool checkPosition = true);
        vector<PartitiontreeNode*> getAllLeafs();

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

#endif // PARTITIONTREE_H_INCLUDED
