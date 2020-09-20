#ifndef VROBJECT_H_INCLUDED
#define VROBJECT_H_INCLUDED

#include "core/utils/VRName.h"
#include "core/utils/VRUndoInterface.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include <OpenSG/OSGMatrixFwd.h>

class VRAttachment;

OSG_BEGIN_NAMESPACE;
using namespace std;

/**

VRObjects are the base object from wich every other object type wich appears in the scenegraph inherits.
Here all the functions regarding the OSG scenegraph structure are wrapped.

*/

class VRObject : public std::enable_shared_from_this<VRObject>, public VRName, public VRUndoInterface {
    private:
        OSGObjectPtr osg;
        OSGCorePtr core;
        bool destroyed = false;
        bool specialized = false;
        VRObjectWeakPtr parent;
        int ID = 0;
        int childIndex = 0; // index of this object in its parent child vector
        int pickable = 0;
        unsigned int visibleMask = -1;
        unsigned int graphChanged = 0; //is frame number
        map<string, VRAttachment*> attachments;

        int findChild(VRObjectPtr node);
        void updateChildrenIndices(bool recursive = false);

        static void unitTest();

    protected:
        vector<VRObjectPtr> children;
        vector<pair<VRObject*, VRObjectWeakPtr>> links;
        string type;
        VREntityPtr entity;

        bool held = false;//drag n drop
        VRObjectWeakPtr old_parent;
        int old_child_id = 0;

        void setIntern(bool b);
        virtual void printInformation();
        virtual VRObjectPtr copy(vector<VRObjectPtr> children);

    public:
        VRObject(string name = "0");
        virtual ~VRObject();

        static VRObjectPtr create(string name = "None");
        VRObjectPtr ptr();

        virtual void wrapOSG(OSGObjectPtr node);

        int getID();
        string getType();

        VRObjectPtr getRoot();
        string getPath();
        VRObjectPtr getAtPath(string path);

        bool hasGraphChanged();

        template<typename T> void addAttachment(string name, T t);
        template<typename T> T getAttachment(string name);
        string getAttachmentAsString(string name);
        void setAttachmentFromString(string name, string value);
        void remAttachment(string name);
        void addTag(string name);
        bool hasTag(string name);
        void remTag(string name);
        vector<string> getTags();
        VRObjectPtr hasAncestorWithTag(string name);
        vector<VRObjectPtr> getChildrenWithTag(string name, bool recursive = false, bool includeSelf = false);

        void setCore(OSGCorePtr c, string _type, bool force = false);
        OSGCorePtr getCore();
        void switchCore(OSGCorePtr c);
        void disableCore();
        void enableCore();

        OSGObjectPtr getNode();
        vector<OSGObjectPtr> getNodes();
        virtual void addChild(OSGObjectPtr n);
        virtual void subChild(OSGObjectPtr n);
        VRObjectPtr find(OSGObjectPtr n, string indent = " ");

        string getOSGTreeString();
        static string printOSGTreeString(OSGObjectPtr o, string indent = "");
        static void printOSGTree(OSGObjectPtr o, string indent = "");

        void setTravMask(int i);
        int getTravMask();
        void setVolume(Boundingbox box);
        virtual void setVolumeCheck(bool b, bool recursive = false);
        void setSiblingPosition(int i);

        virtual void addChild(VRObjectPtr child, bool osg = true, int place = -1);
        virtual void subChild(VRObjectPtr child, bool osg = true);
        VRObjectPtr getLink(int i);
        vector<VRObjectPtr> getLinks();
        void addLink(VRObjectPtr obj);
        void remLink(VRObjectPtr obj);
        void clearLinks();

        void switchParent(VRObjectPtr new_p, int place = -1);
        void detach();

        int getChildIndex();
        VRObjectPtr getChild(int i);
        vector<VRObjectPtr> getChildren(bool recursive = false, string type = "", bool includeSelf = false);
        VRObjectPtr getParent(bool checkForDrag = false);
        vector<VRObjectPtr> getAncestry(VRObjectPtr ancestor = 0);
        size_t getChildrenCount();
        void clearChildren(bool destroy = true);

        bool hasDescendant(VRObjectPtr obj);
        bool hasAncestor(VRObjectPtr obj);
        bool shareAncestry(VRObjectPtr obj);
        VRObjectPtr findPickableAncestor();

        vector<VRObjectPtr> getObjectListByType( string _type );
        void getObjectListByType( string _type, vector<VRObjectPtr>& list );

        VRObjectPtr find(VRObjectPtr obj);
        VRObjectPtr find(string Name);
        VRObjectPtr findFirst(string Name);
        VRObjectPtr find(int id);
        vector<VRObjectPtr> findAll(string Name, vector<VRObjectPtr> res = vector<VRObjectPtr>() );
        vector<VRObjectPtr> filterByType(string Type, vector<VRObjectPtr> res = vector<VRObjectPtr>() );

        void setEntity(VREntityPtr e);
        VREntityPtr getEntity();

        BoundingboxPtr getBoundingbox();
        BoundingboxPtr getWorldBoundingbox();

        void flattenHiarchy();

        void printTree(int indent = 0);

        VRObjectPtr duplicate(bool anchor = false, bool subgraph = true);

        void setVisibleUndo(unsigned int b);
        void hide(string mode = "");
        void show(string mode = "");
        bool isVisible(string mode = "", bool recursive = false);
        void setVisibleMask(unsigned int mask);
        void setVisible(bool b, string mode = "");
        void toggleVisible(string mode = "");

        bool isPickable();
        void setPickable(int b);

        void setupBefore(VRStorageContextPtr context);
        void setupAfter(VRStorageContextPtr context);
        void destroy();

        PosePtr getPoseTo(VRObjectPtr o);
        Matrix4d getMatrixTo(VRObjectPtr o, bool parentOnly = false);

        void exportToFile(string path);

        void reduceModel(string strategy);
};

OSG_END_NAMESPACE;

#endif // VROBJECT_H_INCLUDED
