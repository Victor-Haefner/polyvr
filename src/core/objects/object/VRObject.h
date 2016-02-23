#ifndef VROBJECT_H_INCLUDED
#define VROBJECT_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGVector.h>

#include "core/utils/VRName.h"
#include "core/utils/VRStorage.h"
#include "core/objects/VRObjectFwd.h"
#include <memory>

namespace xmlpp{ class Element; }
class VRAttachment;

OSG_BEGIN_NAMESPACE;
using namespace std;

class Node; OSG_GEN_CONTAINERPTR(Node);

/**

VRObjects are the base object from wich every other object type wich appears in the scenegraph inherits.
Here all the functions regarding the OSG scenegraph structure are wrapped.

*/

class VRGlobals {
    private:
        VRGlobals();

    public:
        unsigned int CURRENT_FRAME = 0;
        unsigned int FRAME_RATE = 0;
        /** TODO magic start number **/
        unsigned int PHYSICS_FRAME_RATE = 500;

        static VRGlobals* get();
};

class VRObject : public VRStorage  {
    private:
        bool specialized = false;
        VRObjectWeakPtr parent;
        NodeRecPtr node;
        int ID = 0;
        int childIndex = 0; // index of this object in its parent child vector
        int pickable = 0;
        bool visible = true;
        bool intern = false;
        int persistency = 666;
        unsigned int graphChanged = 0; //is frame number

        map<string, VRAttachment*> attachments;
        map<VRObject*, NodeRecPtr> links;

        int findChild(VRObjectPtr node);
        void updateChildrenIndices(bool recursive = false);

        static void unitTest();

    protected:
        vector<VRObjectPtr> children;
        string type;

        void setIntern(bool b);

        virtual void printInformation();
        //void printInformation();

        virtual VRObjectPtr copy(vector<VRObjectPtr> children);
        //VRObjectPtr copy();

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

    public:
        VRObject(string name = "0");
        virtual ~VRObject();

        static VRObjectPtr create(string name);
        VRObjectPtr ptr();

        /** Returns the Object ID **/
        int getID();

        /** Returns the object type **/
        string getType();

        bool getIntern();
        void setPersistency(int p);
        int getPersistency();

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

        void setCore(NodeCoreRecPtr c, string _type, bool force = false);
        NodeCoreRecPtr getCore();
        void switchCore(NodeCoreRecPtr c);

        void setVolume(bool b);

        /** Returns the object OSG node **/
        NodeRecPtr getNode();

        /** set the position in the parents child list **/
        void setSiblingPosition(int i);

        virtual void addChild(VRObjectPtr child, bool osg = true, int place = -1);
        virtual void addChild(NodeRecPtr n);
        virtual void subChild(VRObjectPtr child, bool osg = true);
        virtual void subChild(NodeRecPtr n);
        void addLink(VRObjectPtr obj);
        void remLink(VRObjectPtr obj);

        /** Switch the parent of this object **/
        void switchParent(VRObjectPtr new_p, int place = -1);

        /** Detach object from the parent**/
        void detach();

        /** Returns the child by his position **/
        int getChildIndex();
        VRObjectPtr getChild(int i);
        vector<VRObjectPtr> getChildren(bool recursive = false, string type = "");

        /** Returns the parent of this object **/
        VRObjectPtr getParent();

        /** Returns the number of children **/
        size_t getChildrenCount();

        void clearChildren();

        /** Returns all objects with a certain type wich are below this object in hirarchy **/
        vector<VRObjectPtr> getObjectListByType( string _type );
        void getObjectListByType( string _type, vector<VRObjectPtr>& list );

        /**
            To find an object in the scene graph was never easier, just pass an OSG node, object, ID || name to a VRObject.
            This Object will search all the hirachy below him (himself included).
        **/

        VRObjectPtr find(NodeRecPtr n, string indent = " ");
        VRObjectPtr find(VRObjectPtr obj);
        VRObjectPtr find(string Name);
        VRObjectPtr find(int id);
        vector<VRObjectPtr> findAll(string Name, vector<VRObjectPtr> res = vector<VRObjectPtr>() );

        vector<VRObjectPtr> filterByType(string Type, vector<VRObjectPtr> res = vector<VRObjectPtr>() );

        /** Returns the first ancestor that is pickable, || 0 if none found **/
        VRObjectPtr findPickableAncestor();

        bool hasAncestor(VRObjectPtr a);

        /** Returns the Boundingbox of the OSG Node */
        void getBoundingBox(Vec3f& v1, Vec3f& v2);
        Vec3f getBBCenter();
        Vec3f getBBExtent();
        float getBBMax();

        void flattenHiarchy();

        /** Print to console the scene subgraph starting at this object **/
        void printTree(int indent = 0);

        static void printOSGTree(NodeRecPtr o, string indent = "");

        /** duplicate this object **/
        VRObjectPtr duplicate(bool anchor = false);

        /** Hide and show this object and all his subgraph **/
        void hide();
        void show();

        /** Returns if this object is visible or not **/
        bool isVisible();

        /** Set the visibility of this object **/
        void setVisible(bool b);

        /** toggle visibility **/
        void toggleVisible();

        /** Returns if this object is pickable or not **/
        bool isPickable();

        /** Set the object pickable || not **/
        void setPickable(int b);

        void destroy();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VROBJECT_H_INCLUDED
