#ifndef VROBJECT_H_INCLUDED
#define VROBJECT_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGVector.h>

#include "core/utils/VRName.h"

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

class VRObject : public VRName {
    private:
        bool specialized = false;
        VRObject* parent = 0;
        NodeRecPtr node;
        int ID = 0;
        int childIndex = 0; // index of this object in its parent child vector
        bool pickable = false;
        bool visible = true;
        bool intern = false;
        unsigned int graphChanged = 0; //is frame number

        map<string, VRAttachment*> attachments;

        int findChild(VRObject* node);
        void updateChildrenIndices(bool recursive = false);

        static void unitTest();

    protected:
        vector<VRObject*> children;
        string type;

        void setIntern(bool b);

        virtual void printInformation();
        //void printInformation();

        virtual VRObject* copy(vector<VRObject*> children);
        //VRObject* copy();

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

    public:

        /** initialise an object with his name **/
        VRObject(string _name = "0");
        virtual ~VRObject();

        /** Returns the Object ID **/
        int getID();

        /** Returns the object type **/
        string getType();

        bool getIntern();

        VRObject* getRoot();
        string getPath();
        VRObject* getAtPath(string path);

        bool hasGraphChanged();

        template<typename T> void addAttachment(string name, T t);
        template<typename T> T getAttachment(string name);
        bool hasAttachment(string name);
        void remAttachment(string name);
        VRObject* hasAncestorWithAttachment(string name);
        vector<VRObject*> getChildrenWithAttachment(string name);

        /** Set the object OSG core && specify the type**/
        void setCore(NodeCoreRecPtr c, string _type);

        /** Returns the object OSG core **/
        NodeCoreRecPtr getCore();

        /** Switch the object core by another **/
        void switchCore(NodeCoreRecPtr c);

        /** Returns the object OSG node **/
        NodeRecPtr getNode();

        /** set the position in the parents child list **/
        void setSiblingPosition(int i);

        /** Add a child to this object **/
        void addChild(VRObject* child, bool osg = true, int place = -1);

        /** Add a OSG node as child to this object **/
        void addChild(NodeRecPtr n);

        /** Remove a child **/
        void subChild(VRObject* child, bool osg = true);

        /** Remove a OSG node child **/
        void subChild(NodeRecPtr n);

        /** Switch the parent of this object **/
        void switchParent(VRObject* new_p, int place = -1);

        /** Detach object from the parent**/
        void detach();

        /** Returns the child by his position **/
        int getChildIndex();
        VRObject* getChild(int i);
        vector<VRObject*> getChildren(bool recursive = false, string type = "");

        /** Returns the parent of this object **/
        VRObject* getParent();

        /** Returns the number of children **/
        size_t getChildrenCount();

        void clearChildren();

        /** Returns all objects with a certain type wich are below this object in hirarchy **/
        vector<VRObject*> getObjectListByType( string _type );
        void getObjectListByType( string _type, vector<VRObject*>& list );

        /**
            To find an object in the scene graph was never easier, just pass an OSG node, object, ID || name to a VRObject.
            This Object will search all the hirachy below him (himself included).
        **/

        VRObject* find(NodeRecPtr n, string indent = " ");
        VRObject* find(VRObject* obj);
        VRObject* find(string Name);
        VRObject* find(int id);

        vector<VRObject*> filterByType(string Type, vector<VRObject*> res = vector<VRObject*>() );

        /** Returns the first ancestor that is pickable, || 0 if none found **/
        VRObject* findPickableAncestor();

        bool hasAncestor(VRObject* a);

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
        VRObject* duplicate(bool anchor = false);

        /** Hide this object && all his subgraph **/
        void hide();

        /** Show this object && all his subgraph **/
        void show();

        /** Returns if this object is visible || not **/
        bool isVisible();

        /** Set the visibility of this object **/
        void setVisible(bool b);

        /** toggle visibility **/
        void toggleVisible();

        /** Returns if this object is pickable || not **/
        bool isPickable();

        /** Set the object pickable || not **/
        void setPickable(bool b);

        void destroy();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VROBJECT_H_INCLUDED
