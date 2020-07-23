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

ptrFwd(FID);
ptrFwd(FPath);
ptrFwd(FObject);
ptrFwd(FNode);
ptrFwd(FNetwork);
ptrFwd(FProduct);
ptrFwd(FStack);
ptrFwd(FTransporter);
ptrFwd(FContainer);
ptrFwd(FLogistics);

class FID {
    private:
        int ID;

    protected:
        FID();

    public:
        int getID();
        void setID(int ID);
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
        FObjectPtr object;
        FTransporterPtr transporter;
        State state;
        VRTransformPtr transform = 0;

    protected:
        FNode();

    public:
        ~FNode();
        void set(FObjectPtr o);
        FObjectPtr get();

        void setState(State);
        State getState();

        void setTransform(VRTransformPtr t);
        VRTransformPtr getTransform();

        friend class FNetwork;
};

class FPath {
    public:
        vector<int> nodes;
        PathPtr path;

        FPath();
        void updatePath(GraphPtr graph);

        int first();
        int last();
};

class FNetwork : public FID {
    private:
        GraphPtr graph;
        map<int, FNodePtr> nodes;

    public:
        FNetwork();
        ~FNetwork();

        int addNode(PosePtr p);
        void connect(int n1, int n2);

        GraphPtr getGraph();
        FNodePtr getNode(int ID);
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
        vector<FProductPtr> products;

    public:
        FContainer();
        ~FContainer();

        void setCapacity(int i);
        int getCapacity();

        void add(FProductPtr p);
        FProductPtr pop();
        FProductPtr peek();

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
        map<shared_ptr<FNode>, FObjectPtr> cargo;
        FPathPtr fpath;
        FTType transport_type;
        float speed;
        weak_ptr<FNetwork> network;

    public:
        FTransporter();
        ~FTransporter();

        void setRoute(shared_ptr<FNode> n1, shared_ptr<FNode> n2);
        void setPath(FPathPtr p);
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
        FNetworkPtr network;
        map<int, FObjectPtr> objects;
        map<int, FTransporterPtr> transporter;

    public:
        FLogistics();
        ~FLogistics();

        static shared_ptr<FLogistics> create();

        FProductPtr addProduct(VRTransformPtr t = 0);
        FNetworkPtr addNetwork();
        FTransporterPtr addTransporter(string type);
        FContainerPtr addContainer(VRTransformPtr t);
        void fillContainer(FContainerPtr c, int N, VRTransformPtr t);
        vector<FContainerPtr> getContainers();

        FPathPtr computeRoute(int n1, int n2);

        void update();
        void run();
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRLOGISTICS_H_INCLUDED
