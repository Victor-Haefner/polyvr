#ifndef VRLOGISTICS_H_INCLUDED
#define VRLOGISTICS_H_INCLUDED

#include <map>
#include <vector>
#include <stack>
#include <memory>

#include "core/math/OSGMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

#include <OpenSG/OSGColor.h>

using namespace std;
OSG_BEGIN_NAMESPACE;


class FID;
class FNode;
class FNetwork;
class FProduct;
class FStack;
class FTransporter;
class FLogistics;


class FID {
    private:
        int ID;

    protected:
        FID();

    public:
        int getID();
};

class FObject : public FID {
    public:
        enum Type { NONE, PRODUCT, CONTAINER };

    private:
        Type type;
        VRTransformPtr transform = 0;
        VRSpritePtr metaData = 0;
        float t = 0;

    protected:
        FObject();

    public:
        virtual ~FObject();

        void setType(Type);
        Type getType();

        void setTransformation(VRTransformPtr t);
        VRTransformPtr getTransformation();
        bool move(PathPtr p, float dx);

        void setMetaData(string s);

        friend class FLogistics;
};

class FNode : public FID, public enable_shared_from_this<FNode> {
    public:
        enum State { FREE, CONTAINER, RESERVED, PRODUCT };

    private:
        shared_ptr<FObject> object;
        shared_ptr<FTransporter> transporter;
        State state;
        VRTransformPtr transform = 0;

        map<int, shared_ptr<FNode>> out;
        map<int, shared_ptr<FNode>> in;

    protected:
        FNode();

    public:
        ~FNode();
        void set(shared_ptr<FObject> o);
        shared_ptr<FObject> get();

        map<int, shared_ptr<FNode>>& getIncoming();
        map<int, shared_ptr<FNode>>& getOutgoing();
        shared_ptr<FNode> previous();
        shared_ptr<FNode> next();

        void setState(State);
        State getState();

        Vec3d getTangent();

        void connect(shared_ptr<FNode> n);
        void disconnect(shared_ptr<FNode> n);
        void isolate();

        void setTransform(VRTransformPtr t);
        VRTransformPtr getTransform();

        friend class FNetwork;
};

class FPath : public FID {
    private:
        vector<shared_ptr<FNode>> nodes;
        map<FNode*, PathPtr> paths;

    public:
        FPath();
        void set(shared_ptr<FNode> n1, shared_ptr<FNode> n2);
        void add(shared_ptr<FNode> n);
        vector<shared_ptr<FNode>>& get();
        shared_ptr<Path> getPath(shared_ptr<FNode>);
        void update();
};

typedef shared_ptr<FNode> FNodePtr;

class FNetwork : public FID {
    private:
        GraphPtr graph;
        map<int, FNodePtr> nodes;

    public:
        FNetwork();
        ~FNetwork();

        FNodePtr addNode(FNodePtr parent = 0);
        FNodePtr addNodeChain(int N, FNodePtr parent = 0);

        vector<FNodePtr> getNodes();

        VRStrokePtr stroke(Color3f c, float k);

        friend class FLogistics;
};

class FProduct : public FObject {
    private:
        void* data;

    public:
        FProduct();
        ~FProduct();

        friend class FLogistics;
};

class FContainer : public FObject {
    private:
        int capacity;
        vector<shared_ptr<FProduct>> products;

    public:
        FContainer();
        ~FContainer();

        void setCapacity(int i);
        int getCapacity();

        void add(shared_ptr<FProduct> p);
        shared_ptr<FProduct> pop();
        shared_ptr<FProduct> peek();

        bool isFull();
        bool isEmpty();
        int getCount();

        void clear();

        friend class FLogistics;
};

class FTransporter : public FID {
    public:
        enum FTType { PRODUCT, CONTAINER_FULL, CONTAINER_EMPTY };

    private:
        map<shared_ptr<FNode>, shared_ptr<FObject>> cargo;
        shared_ptr<FPath> fpath;
        FTType transport_type;
        float speed;

    public:
        FTransporter();
        ~FTransporter();

        void setRoute(shared_ptr<FNode> n1, shared_ptr<FNode> n2);
        void setPath(shared_ptr<FPath> p);
        void setTransportType(FTType type);
        void setSpeed(float s);
        float getSpeed();

        void load();
        void unload();

        void update(float dt);

        friend class FLogistics;
};

class FLogistics {
    private:
        map<int, shared_ptr<FNetwork>> networks;
        map<int, shared_ptr<FObject>> objects;
        map<int, shared_ptr<FTransporter>> transporter;
        map<int, shared_ptr<FPath>> paths;
        map<int, shared_ptr<FNetwork>>::iterator n_itr;
        map<int, shared_ptr<FObject>>::iterator o_itr;
        map<int, shared_ptr<FTransporter>>::reverse_iterator t_ritr;

    public:
        FLogistics();
        ~FLogistics();

        static shared_ptr<FLogistics> create();

        shared_ptr<FProduct> addProduct(VRTransformPtr t = 0);
        shared_ptr<FNetwork> addNetwork();
        shared_ptr<FTransporter> addTransporter(FTransporter::FTType type);
        shared_ptr<FPath> addPath();
        shared_ptr<FContainer> addContainer(VRTransformPtr t);
        void fillContainer(shared_ptr<FContainer> c, int N, VRTransformPtr t);
        vector<shared_ptr<FContainer>> getContainers();

        void update();
        void run();
};

OSG_END_NAMESPACE;

#endif // VRLOGISTICS_H_INCLUDED
