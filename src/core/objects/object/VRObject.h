#ifndef VROBJECT_H_INCLUDED
#define VROBJECT_H_INCLUDED

#include "core/utils/VRName.h"
#include "core/utils/VRUndoInterface.h"
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

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
        bool visible = true;
        unsigned int graphChanged = 0; //is frame number
        map<string, VRAttachment*> attachments;
        VREntityPtr entity;

        int findChild(VRObjectPtr node);
        void updateChildrenIndices(bool recursive = false);

        static void unitTest();

    protected:
        vector<VRObjectPtr> children;
        map<VRObject*, VRObjectWeakPtr> links;
        string type;

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

        int getID();
        string getType();

        VRObjectPtr getRoot();
        string getPath();
        VRObjectPtr getAtPath(string path);

        bool hasGraphChanged();

        template<typename T> void addAttachment(string name, T t);
        template<typename T> T getAttachment(string name);
        bool hasAttachment(string name);
        void remAttachment(string name);
        vector<string> getAttachmentNames();
        VRObjectPtr hasAncestorWithAttachment(string name);
        vector<VRObjectPtr> getChildrenWithAttachment(string name);

        void setCore(OSGCorePtr c, string _type, bool force = false);
        OSGCorePtr getCore();
        void switchCore(OSGCorePtr c);
        void disableCore();
        void enableCore();

        OSGObjectPtr getNode();
        virtual void addChild(OSGObjectPtr n);
        virtual void subChild(OSGObjectPtr n);
        VRObjectPtr find(OSGObjectPtr n, string indent = " ");
        static void printOSGTree(OSGObjectPtr o, string indent = "");

        void allowCulling(bool b, bool recursive = false);
        void setSiblingPosition(int i);

        virtual void addChild(VRObjectPtr child, bool osg = true, int place = -1);
        virtual void subChild(VRObjectPtr child, bool osg = true);
        vector<VRObjectPtr> getLinks();
        void addLink(VRObjectPtr obj);
        void remLink(VRObjectPtr obj);
        void clearLinks();

        void switchParent(VRObjectPtr new_p, int place = -1);
        void detach();

        int getChildIndex();
        VRObjectPtr getChild(int i);
        vector<VRObjectPtr> getChildren(bool recursive = false, string type = "", bool includeSelf = false);
        VRObjectPtr getParent();
        size_t getChildrenCount();
        void clearChildren();

        vector<VRObjectPtr> getObjectListByType( string _type );
        void getObjectListByType( string _type, vector<VRObjectPtr>& list );

        VRObjectPtr find(VRObjectPtr obj);
        VRObjectPtr find(string Name);
        VRObjectPtr find(int id);
        vector<VRObjectPtr> findAll(string Name, vector<VRObjectPtr> res = vector<VRObjectPtr>() );
        vector<VRObjectPtr> filterByType(string Type, vector<VRObjectPtr> res = vector<VRObjectPtr>() );

        VRObjectPtr findPickableAncestor();
        bool hasAncestor(VRObjectPtr a);

        void setEntity(VREntityPtr e);
        VREntityPtr getEntity();

        boundingboxPtr getBoundingBox();

        void flattenHiarchy();

        void printTree(int indent = 0);

        VRObjectPtr duplicate(bool anchor = false);

        void hide();
        void show();
        bool isVisible();
        void setVisible(bool b);
        void toggleVisible();

        bool isPickable();
        void setPickable(int b);

        void setup();
        void destroy();
};

OSG_END_NAMESPACE;

#endif // VROBJECT_H_INCLUDED
