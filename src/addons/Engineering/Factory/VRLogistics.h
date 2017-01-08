#ifndef VRLOGISTICS_H_INCLUDED
#define VRLOGISTICS_H_INCLUDED

#include <map>
#include <vector>
#include <stack>
#include <memory>

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

class FID;
class FNode;
class FNetwork;
class FProduct;
class FStack;
class FTransporter;
class FLogistics;

using namespace std;
namespace OSG{ class VRTransform; class VRStroke; class VRSprite; class path; }

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
        OSG::VRTransformPtr transform = 0;
        OSG::VRSpritePtr metaData = 0;
        float t = 0;

    protected:
        FObject();

    public:
        virtual ~FObject();

        void setType(Type);
        Type getType();

        void setTransformation(OSG::VRTransformPtr t);
        OSG::VRTransformPtr getTransformation();
        bool move(OSG::pathPtr p, float dx);

        void setMetaData(std::string s);

        friend class FLogistics;
};

class FNode : public FID, public std::enable_shared_from_this<FNode> {
    public:
        enum State { FREE, CONTAINER, RESERVED, PRODUCT };

    private:
        shared_ptr<FObject> object;
        shared_ptr<FTransporter> transporter;
        State state;
        OSG::VRTransformPtr transform = 0;

        std::map<int, shared_ptr<FNode>> out;
        std::map<int, shared_ptr<FNode>> in;
        std::map<int, shared_ptr<FNode>>::iterator itr;

    protected:
        FNode();

    public:
        ~FNode();
        void set(shared_ptr<FObject> o);
        shared_ptr<FObject> get();

        std::map<int, shared_ptr<FNode>>& getIncoming();
        std::map<int, shared_ptr<FNode>>& getOutgoing();
        shared_ptr<FNode> previous();
        shared_ptr<FNode> next();

        void setState(State);
        State getState();

        OSG::Vec3f getTangent();

        void connect(shared_ptr<FNode> n);
        void disconnect(shared_ptr<FNode> n);
        void isolate();

        void setTransform(OSG::VRTransformPtr t);
        OSG::VRTransformPtr getTransform();

        friend class FNetwork;
};

class FPath : public FID {
    private:
        std::vector<shared_ptr<FNode>> nodes;
        std::map<FNode*, OSG::pathPtr> paths;

    public:
        FPath();
        void set(shared_ptr<FNode> n1, shared_ptr<FNode> n2);
        void add(shared_ptr<FNode> n);
        std::vector<shared_ptr<FNode>>& get();
        shared_ptr<OSG::path> getPath(shared_ptr<FNode>);
        void update();
};


class FNetwork : public FID {
    private:
        std::map<int, shared_ptr<FNode>> nodes;
        std::map<int, shared_ptr<FNode>>::iterator itr;

    public:
        FNetwork();
        ~FNetwork();

        shared_ptr<FNode> addNode(shared_ptr<FNode> parent = 0);
        shared_ptr<FNode> addNodeChain(int N, shared_ptr<FNode> parent = 0);

        std::vector<shared_ptr<FNode>> getNodes();

        OSG::VRStrokePtr stroke(OSG::Vec3f c, float k);

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
        std::vector<shared_ptr<FProduct>> products;

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
        std::map<shared_ptr<FNode>, shared_ptr<FObject>> cargo;
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
        std::map<int, shared_ptr<FNetwork>> networks;
        std::map<int, shared_ptr<FObject>> objects;
        std::map<int, shared_ptr<FTransporter>> transporter;
        std::map<int, shared_ptr<FPath>> paths;
        std::map<int, shared_ptr<FNetwork>>::iterator n_itr;
        std::map<int, shared_ptr<FObject>>::iterator o_itr;
        std::map<int, shared_ptr<FTransporter>>::reverse_iterator t_ritr;

    public:
        FLogistics();
        ~FLogistics();

        static shared_ptr<FLogistics> create();

        shared_ptr<FProduct> addProduct(OSG::VRTransformPtr t = 0);
        shared_ptr<FNetwork> addNetwork();
        shared_ptr<FTransporter> addTransporter(FTransporter::FTType type);
        shared_ptr<FPath> addPath();
        shared_ptr<FContainer> addContainer(OSG::VRTransformPtr t);
        void fillContainer(shared_ptr<FContainer> c, int N, OSG::VRTransformPtr t);
        std::vector<shared_ptr<FContainer>> getContainers();

        void update();
        void run();
};

#endif // VRLOGISTICS_H_INCLUDED
