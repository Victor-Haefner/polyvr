#include "VRObject.h"
#include "../VRTransform.h"
#include "../OSGObject.h"
#include "OSGCore.h"
#include "VRObjectT.h"
#include "VRAttachment.h"
#include "core/math/boundingbox.h"
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGVisitSubTree.h>
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/utils/VRStorage_template.h"
#include "addons/Semantics/Reasoning/VREntity.h"
#include <libxml++/nodes/element.h>

using namespace OSG;

template<> string typeName(const VRObjectPtr& o) { return o?o->getType():"Object(null)"; }

VRObject::VRObject(string _name) {
    static int _ID = 0;
    ID = _ID;
    _ID++;

    setName(_name);

    osg = shared_ptr<OSGObject>( new OSGObject() );

    osg->node = makeNodeFor(Group::create());
    OSG::setName(osg->node, name);
    type = "Object";

    setStorageType("Object");
    store("type", &type);
    store("pickable", &pickable);
    store("visible", &visible);
    storeObjVec("children", children);

    regStorageSetupFkt( VRUpdateCb::create("object setup", boost::bind(&VRObject::setup, this)) );
}

VRObject::~VRObject() {
    //cout << " ~VRObject " << getName() << endl;
    NodeMTRecPtr p;
    if (osg->node) p = osg->node->getParent();
    if (p) p->subChild(osg->node);
}

void VRObject::setup() {
    setVisible(visible);
    setPickable(pickable);

    auto tmp = children;
    children.clear();
    for (auto c : tmp) addChild(c);
}

void VRObject::destroy() {
    destroyed = true;
    auto p = ptr();
    if (auto pa = parent.lock())     pa->subChild( p );
}

void VRObject::detach() {
    if (getParent() == 0) return;
    getParent()->subChild(ptr(), true);
    parent.reset();
}

void allowCullingRecursive(NodeMTRecPtr n, bool b) {
    if (!n) return;

    BoxVolume &vol = n->editVolume(false);
    vol.setInfinite(!b);
    vol.setStatic(!b);
    vol.setValid(!b);

    for (uint i=0; i<n->getNChildren(); i++) {
        allowCullingRecursive(n->getChild(i), b);
    }
}

void VRObject::setVolumeCheck(bool b, bool recursive) {
    if (!getNode()) return;
    BoxVolume &vol = getNode()->node->editVolume(b);
    vol.setInfinite(!b);
    vol.setStatic(!b);
    vol.setValid(!b);

    if (recursive) allowCullingRecursive(getNode()->node, b);
}

void VRObject::setVolume(const Boundingbox& box) {
    BoxVolume &vol = getNode()->node->editVolume(false);
    vol.setBounds(Vec3f(box.min()), Vec3f(box.max()));
    vol.setStatic(true);
    vol.setValid(true);
}

VRObjectPtr VRObject::create(string name) { return VRObjectPtr(new VRObject(name) ); }
VRObjectPtr VRObject::ptr() { return static_pointer_cast<VRObject>( shared_from_this() ); }

void VRObject::printInformation() {;}

VRObjectPtr VRObject::copy(vector<VRObjectPtr> children) {
    VRObjectPtr o = VRObject::create(getBaseName());
    if (specialized) o->setCore(getCore(), getType());
    o->setVisible(visible);
    o->setPickable(pickable);
    o->setEntity(entity);
    return o;
}

int VRObject::getID() { return ID; }
string VRObject::getType() { return type; }
void VRObject::addTag(string name) { addAttachment(name, 0); }
bool VRObject::hasTag(string name) { return attachments.count(name); }
void VRObject::remTag(string name) { attachments.erase(name); }

vector<string> VRObject::getTags() {
    vector<string> res;
    for (auto a : attachments) res.push_back(a.first);
    return res;
}

vector<VRObjectPtr> VRObject::getChildrenWithTag(string name) {
    vector<VRObjectPtr> res;
    for (auto c : getChildren()) {
        if (c->hasTag(name)) res.push_back(c);
    }
    return res;
}

VRObjectPtr VRObject::hasAncestorWithTag(string name) {
    if (hasTag(name)) return ptr();
    if (getParent() == 0) return 0;
    return getParent()->hasAncestorWithTag(name);
}

void VRObject::setTravMask(int i) {
    getNode()->node->setTravMask(i);
}

vector<VRObjectPtr> VRObject::getLinks() {
    vector<VRObjectPtr> res;
    for (auto wl : links) if (auto l = wl.second.lock()) res.push_back(l);
    return res;
}

void VRObject::addLink(VRObjectPtr obj) {
    if (osg->links.count(obj.get())) return;

    VisitSubTreeMTRecPtr visitor = VisitSubTree::create();
    visitor->setSubTreeRoot(obj->getNode()->node);
    NodeMTRecPtr visit_node = makeNodeFor(visitor);
    addChild(OSGObject::create(visit_node));

    links[obj.get()] = obj;
    osg->links[obj.get()] = visit_node;
}

void VRObject::remLink(VRObjectPtr obj) {
    if (!osg->links.count(obj.get())) return;

    NodeMTRecPtr node = osg->links[obj.get()];
    subChild(OSGObject::create(node));
    osg->links.erase(obj.get());
    links.erase(obj.get());
}

void VRObject::clearLinks() {
    vector<VRObject*> links;
    for (auto o : osg->links) links.push_back(o.first);
    for (auto o : links) {
        NodeMTRecPtr node = osg->links[o];
        subChild(OSGObject::create(node));
        osg->links.erase(o);
    }
}

void VRObject::setCore(OSGCorePtr c, string _type, bool force) {
    if (specialized && !force) {
        cout << "\nError, Object allready specialized, skip setCore()\n";
        return;
    }

    core = c;
    type = _type;
    osg->node->setCore(c->core);
    specialized = true;
}

/** Returns the object OSG core **/
OSGCorePtr VRObject::getCore() { return core; }

/** Switch the object core by another **/
void VRObject::switchCore(OSGCorePtr c) {
    if(!specialized) return;
    osg->node->setCore(c->core);
    core = c;
}

void VRObject::disableCore() { osg->node->setCore( Group::create() ); }
void VRObject::enableCore() { osg->node->setCore( core->core ); }

/** Returns the object OSG node **/
OSGObjectPtr VRObject::getNode() { return osg; }

void VRObject::setSiblingPosition(int i) {
    auto parent = getParent();
    if (parent == 0) return;
    if (i < 0 || i >= (int)parent->children.size()) return;

    NodeMTRecPtr p = parent->getNode()->node;
    p->subChild(getNode()->node);
    p->insertChild(i, getNode()->node);

    parent->children.erase(std::find(parent->children.begin(), parent->children.end(), ptr()));
    parent->children.insert(parent->children.begin() + i, ptr());
}

void VRObject::addChild(OSGObjectPtr n) {
    if (!osg || !osg->node) { cout << "Warning! VRObject::addChild: bad osg parent node!\n"; return; }
    if (!n || !n->node) { cout << "Warning! VRObject::addChild: bad osg child node!\n"; return; }
    osg->node->addChild(n->node);
}

void VRObject::addChild(VRObjectPtr child, bool osg, int place) {
    if (child == 0 || child == ptr()) return;
    if (child->getParent() != 0) { child->switchParent(ptr(), place); return; }

    if (osg) addChild(child->osg);
    child->graphChanged = VRGlobals::CURRENT_FRAME;
    child->childIndex = children.size();
    children.push_back(child);
    child->parent = ptr();
    child->setSiblingPosition(place);
    updateChildrenIndices(true);
}

int VRObject::getChildIndex() { return childIndex;}

void VRObject::subChild(OSGObjectPtr n) { osg->node->subChild(n->node); }
void VRObject::subChild(VRObjectPtr child, bool doOsg) {
    if (child == ptr()) return;
    if (doOsg) osg->node->subChild(child->osg->node);

    int target = findChild(child);

    if (target != -1) children.erase(children.begin() + target);
    if (child->getParent() == ptr()) child->parent.reset();
    child->graphChanged = VRGlobals::CURRENT_FRAME;
    updateChildrenIndices(true);
}

void VRObject::switchParent(VRObjectPtr new_p, int place) {
    if (destroyed) return;
    if (new_p == ptr()) return;
    if (new_p == 0) { cout << "\nERROR : new parent is 0!\n"; return; }

    if (getParent() == 0) { new_p->addChild(ptr(), true, place); return; }
    if (getParent() == new_p && place == childIndex) { return; }

    getParent()->subChild(ptr(), true);
    new_p->addChild(ptr(), true, place);
}

/** Returns the number of children **/
size_t VRObject::getChildrenCount() { return children.size(); }

void VRObject::clearChildren() {
    int N = getChildrenCount();
    for (int i=N-1; i>=0; i--) {
        VRObjectPtr c = getChild(i);
        subChild( c );
        c->destroy();
    }
}

VRObjectPtr VRObject::getChild(int i) {
    if (i < 0 || i >= (int)children.size()) return 0;
    return children[i];
}

bool VRObject::hasDescendant(VRObjectPtr obj) { return obj->hasAncestor(ptr()); }

bool VRObject::hasAncestor(VRObjectPtr a) {
    if (ptr() == a || getParent() == a) return true;
    if (getParent()) return getParent()->hasAncestor(a);
    return false;
}

bool VRObject::shareAncestry(VRObjectPtr obj) {
    return getRoot() == obj->getRoot();
}

VRObjectPtr VRObject::getParent(bool checkForDrag) {
    if (checkForDrag && held) return old_parent.lock();
    return parent.lock();
}

vector<VRObjectPtr> VRObject::getAncestry(VRObjectPtr ancestor) {
    VRObjectPtr first = ptr();
    vector<VRObjectPtr> res;
    while (first->getParent() && first != ancestor) {
        first = first->getParent();
        res.push_back(first);
    }
    reverse(res.begin(), res.end());
    return res;
}

VRObjectPtr VRObject::getAtPath(string path) {
    vector<int> pvec;
    stringstream ss(path);
    string item;
    while (getline(ss, item, ':')) pvec.push_back(toInt(item));

    pvec.erase( pvec.begin() ); // ditch first element (should be the local node)

    VRObjectPtr res = ptr();
    for (int c : pvec) {
        if (res == 0) break;
        res = res->getChild(c);
    }

    return res;
}

vector<VRObjectPtr> VRObject::getChildren(bool recursive, string type, bool includeSelf) {
    if (!recursive) {
        if (type == "") return children;
        vector<VRObjectPtr> res;
        for (auto c : children) if (c->getType() == type) res.push_back(c);
        return res;
    }

    vector<VRObjectPtr> res = getChildren(false, type);
    if (includeSelf && (getType() == type || type == "")) res.push_back( ptr() );

    for (auto c : children) {
        vector<VRObjectPtr> tmp = c->getChildren(true, type);
        res.insert( res.end(), tmp.begin(), tmp.end() );
    }
    return res;
}

vector<VRObjectPtr> VRObject::getObjectListByType(string _type) {
    vector<VRObjectPtr> v;
    getObjectListByType(_type, v);
    return v;
}

void VRObject::getObjectListByType(string _type, vector<VRObjectPtr>& list) {
    if (type == "Camera") return;
    if (type == _type) list.push_back(ptr());
    for (auto c : children) c->getObjectListByType(_type, list);
}

VRObjectPtr VRObject::find(OSGObjectPtr n, string indent) {
    //cout << endl << indent << getName() << " " << node << " " << ptr() << flush;
    if (osg->node == n->node) return ptr();
    for (auto c : children) {
        VRObjectPtr tmp = c->find(n, indent+" ");
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::find(VRObjectPtr obj) {
    if (obj == ptr()) return ptr();
    for (auto c : children) {
        VRObjectPtr tmp = c->find(obj);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::find(string Name) {
    if (name == Name) return ptr();
    for (auto c : children) {
        if (c == ptr()) continue; // workaround! TODO: find why ptr() can happn
        VRObjectPtr tmp = c->find(Name);
        if (tmp != 0) return tmp;
    }
    return 0;
}

vector<VRObjectPtr> VRObject::findAll(string Name, vector<VRObjectPtr> res ) {
    if (base_name == Name) res.push_back(ptr());
    for (auto c : children) {
        if (c == ptr()) continue; // workaround! TODO: find why ptr() can happn
        res = c->findAll(Name, res);
    }
    return res;
}

VRObjectPtr VRObject::find(int id) {
    if (ID == -1) return 0;
    if (ID == id) return ptr();
    for (auto c : children) {
        VRObjectPtr tmp = c->find(id);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::getRoot() {
    VRObjectPtr o = ptr();
    while (o->getParent()) o = o->getParent();
    return o;
}

vector<VRObjectPtr> VRObject::filterByType(string Type, vector<VRObjectPtr> res) {
    if (type == Type) res.push_back(ptr());
    for (uint i=0;i<children.size();i++)
        res = children[i]->filterByType(Type, res);
    return res;
}

/** Returns the first ancestor that is pickable, || 0 if none found **/
VRObjectPtr VRObject::findPickableAncestor() {
    if (pickable == -1) return 0;
    if (isPickable()) return ptr();
    else if (getParent() == 0) return 0;
    else return getParent()->findPickableAncestor();
}

bool VRObject::hasGraphChanged() {
    if (graphChanged == VRGlobals::CURRENT_FRAME) return true;
    if (getParent() == 0) return false;
    return getParent()->hasGraphChanged();
}

/** Returns the Boundingbox of the OSG Node */
BoundingboxPtr VRObject::getBoundingbox() {
    Pnt3f p1, p2;
    commitChanges();
    osg->node->updateVolume();
    osg->node->getVolume().getBounds(p1, p2);
    BoundingboxPtr b = shared_ptr<Boundingbox>( new Boundingbox );
    b->update(Vec3d(p1));
    b->update(Vec3d(p2));
    return b;
}

void VRObject::flattenHiarchy() {
    map<VRTransformPtr, Matrix4d> geos;
    for(auto g : getChildren(true, "Geometry") ) {
        shared_ptr<VRTransform> t = static_pointer_cast<VRTransform>(g);
        geos[t] = t->getWorldMatrix();
        t->detach();
    }

    clearChildren();

    for (auto g : geos) {
        addChild(g.first);
        g.first->setWorldMatrix(g.second);
    }
}

/** Print to console the scene subgraph starting at ptr() object **/
void VRObject::printTree(int indent) {
    if(indent == 0) cout << "\nPrint Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    cout << "name: " << name << " ID: " << ID << " type: " << type << " pnt: " << ptr();
    printInformation();
    for (uint i=0;i<children.size();i++) children[i]->printTree(indent+1);

    if(indent == 0) cout << "\n";
}

void VRObject::printOSGTree(OSGObjectPtr o, string indent) {
    if (o == 0) return;

    string type = o->node->getCore()->getTypeName();
    string name = "Unnamed";
    if (OSG::getName(o->node)) name = OSG::getName(o->node);

    // get attachments
    // print them

    cout << "\n" << indent << name << " " << type << "  ";
    if (type == "Transform") {
        Transform* t = dynamic_cast<Transform*>(o->node->getCore());
        cout << t->getMatrix()[0] << "  " << t->getMatrix()[1] << "  " << t->getMatrix()[2];
    }
    cout << flush;

    for (uint i=0; i<o->node->getNChildren(); i++) {
        printOSGTree(OSGObject::create(o->node->getChild(i)), indent + " ");
    }
}

/** duplicate ptr() object **/
VRObjectPtr VRObject::duplicate(bool anchor) {
    vector<VRObjectPtr> children;
    int N = getChildrenCount();
    for (int i=0;i<N;i++) {// first duplicate all children
        VRObjectPtr d = getChild(i)->duplicate();
        children.push_back(d);// ptr() is not the objects children vector! (its local)
    }

    VRObjectPtr o = copy(children); // copy himself
    for (uint i=0; i<children.size();i++)
        o->addChild(children[i]); // append children

    if (anchor && getParent()) getParent()->addChild(o);
    return o;
}

void VRObject::updateChildrenIndices(bool recursive) {
    for (uint i=0; i<children.size(); i++) {
        children[i]->childIndex = i;
        if (recursive) children[i]->updateChildrenIndices();
    }
}

int VRObject::findChild(VRObjectPtr node) {
    for (uint i=0;i<children.size();i++)
        if (children[i] == node) return i;
    return -1;
}

/** Hide/show the object and all his subgraph **/
void VRObject::hide() { setVisible(false); }
void VRObject::show() { setVisible(true); }

/** Returns if object is visible or not **/
bool VRObject::isVisible() { return visible; }

/** Set the visibility **/
void VRObject::setVisible(bool b) {
    if (b == visible) return;
    recUndo(&VRObject::setVisible, ptr(), visible, b);
    visible = b;
    if (b) osg->node->setTravMask(0xffffffff);
    else osg->node->setTravMask(0);
}

/** toggle visibility **/
void VRObject::toggleVisible() { setVisible(!visible); }

/** Returns if ptr() object is pickable || not **/
bool VRObject::isPickable() { return pickable == 1; }

/** Set the object pickable || not **/
void VRObject::setPickable(int b) { if (hasTag("transform")) pickable = b; } //TODO: check if the if is necessary!

string VRObject::getPath() {
    VRObjectPtr o = ptr();
    string path = toString(o->childIndex);

    while (o->getParent()) {
        o = o->getParent();
        path = toString(o->childIndex) + ":" + path;
    }

    return path;
}

void VRObject::unitTest() {
    //only test once
    static bool done = false;
    if (done == true) return;
    done = true;

    cout << "\nUnit Test: VRObject.h";
    cout << "\n Init 3 test objects" << flush;
    VRObjectPtr o1, o2, o3;
    o1 = VRObject::create("test1");
    o2 = VRObject::create("test2");
    o3 = VRObject::create("test3");
    cout << "  Ok" << flush;
    cout << "\n addChild" << flush;
    o1->addChild(o2);
    o2->addChild(o3);
    cout << "  Ok" << flush;
    cout << "\n switchParent" << flush;
    o3->switchParent(o1);
    cout << "  Ok" << flush;
    cout << "\nEnd Unit Test\n";
}


void VRObject::setEntity(VREntityPtr e) { entity = e; }
VREntityPtr VRObject::getEntity() { return entity; }
