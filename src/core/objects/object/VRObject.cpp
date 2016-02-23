#include "VRObject.h"
#include "../VRTransform.h"
#include "VRObjectT.h"
#include "VRAttachment.h"
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGVisitSubTree.h>
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRGlobals::VRGlobals() {}
VRGlobals* VRGlobals::get() {
    static VRGlobals* s = new VRGlobals();
    return s;
}

VRObject::VRObject(string _name) {
    static int _ID = 0;
    ID = _ID;
    _ID++;

    setName(_name);

    node = makeNodeFor(Group::create());
    OSG::setName(node, name);
    type = "Object";
}

VRObject::~VRObject() {
    NodeRecPtr p;
    if (node) p = node->getParent();
    if (p) p->subChild(node);
}

void VRObject::destroy() {
    auto p = ptr();
    if (getParent()) getParent()->subChild( p );
}

void VRObject::detach() {
    if (getParent() == 0) return;
    getParent()->subChild(ptr(), true);
    parent.reset();
}

void VRObject::setVolume(bool b) {
    float inf = std::numeric_limits<float>::max();
    BoxVolume &vol = getNode()->editVolume(false);
    vol.setEmpty();
    vol.extendBy(Pnt3f(-inf,-inf,-inf));
    vol.extendBy(Pnt3f(inf,inf,inf));
    vol.setValid(!b);
    vol.setStatic(!b);
}

VRObjectPtr VRObject::create(string name) { return VRObjectPtr(new VRObject(name) ); }
VRObjectPtr VRObject::ptr() { return static_pointer_cast<VRObject>( shared_from_this() ); }

void VRObject::printInformation() {;}

VRObjectPtr VRObject::copy(vector<VRObjectPtr> children) {
    VRObjectPtr o = VRObject::create(getBaseName());
    if (specialized) o->setCore(getCore(), getType());
    o->setVisible(visible);
    o->setPickable(pickable);
    return o;
}

int VRObject::getID() { return ID; }
string VRObject::getType() { return type; }
bool VRObject::hasAttachment(string name) { return attachments.count(name); }
void VRObject::remAttachment(string name) { attachments.erase(name); }

vector<string> VRObject::getAttachmentNames() {
    vector<string> res;
    for (auto a : attachments) res.push_back(a.first);
    return res;
}

vector<VRObjectPtr> VRObject::getChildrenWithAttachment(string name) {
    vector<VRObjectPtr> res;
    for (auto c : getChildren()) {
        if (c->hasAttachment(name)) res.push_back(c);
    }
    return res;
}

VRObjectPtr VRObject::hasAncestorWithAttachment(string name) {
    if (hasAttachment(name)) return ptr();
    if (getParent() == 0) return 0;
    return getParent()->hasAncestorWithAttachment(name);
}

void VRObject::addLink(VRObjectPtr obj) {
    if (links.count(obj.get())) return;

    VisitSubTreeRecPtr visitor = VisitSubTree::create();
    visitor->setSubTreeRoot(obj->getNode());
    NodeRecPtr visit_node = makeNodeFor(visitor);
    addChild(visit_node);

    links[obj.get()] = visit_node;
}

void VRObject::remLink(VRObjectPtr obj) {
    if (!links.count(obj.get())) return;

    NodeRecPtr node = links[obj.get()];
    subChild(node);
    links.erase(obj.get());
}

void VRObject::setCore(NodeCoreRecPtr c, string _type, bool force) {
    if (specialized && !force) {
        cout << "\nError, Object allready specialized, skip setCore()\n";
        return;
    }

    type = _type;
    node->setCore(c);
    specialized = true;
}

void VRObject::setPersistency(int p) { persistency = p; }
int VRObject::getPersistency() { return persistency; }

/** Returns the object OSG core **/
NodeCoreRecPtr VRObject::getCore() { return node->getCore(); }

/** Switch the object core by another **/
void VRObject::switchCore(NodeCoreRecPtr c) {
    if(!specialized) return;
    node->setCore(c);
}

/** Returns the object OSG node **/
NodeRecPtr VRObject::getNode() { return node; }

void VRObject::setSiblingPosition(int i) {
    auto parent = getParent();
    if (parent == 0) return;
    if (i < 0 || i >= (int)parent->children.size()) return;

    NodeRecPtr p = parent->getNode();
    p->subChild(getNode());
    p->insertChild(i, getNode());

    parent->children.erase(std::find(parent->children.begin(), parent->children.end(), ptr()));
    parent->children.insert(parent->children.begin() + i, ptr());
}

void VRObject::addChild(NodeRecPtr n) { node->addChild(n); }

void VRObject::addChild(VRObjectPtr child, bool osg, int place) {
    if (child == 0) return;
    if (child->getParent() != 0) { child->switchParent(ptr(), place); return; }

    if (osg) node->addChild(child->node);
    child->graphChanged = VRGlobals::get()->CURRENT_FRAME;
    child->childIndex = children.size();
    children.push_back(child);
    child->parent = ptr();
    child->setSiblingPosition(place);
    updateChildrenIndices(true);
}

int VRObject::getChildIndex() { return childIndex;}

void VRObject::subChild(NodeRecPtr n) { node->subChild(n); }
void VRObject::subChild(VRObjectPtr child, bool osg) {
    if (osg) node->subChild(child->node);

    int target = findChild(child);

    if (target != -1) children.erase(children.begin() + target);
    if (child->getParent() == ptr()) child->parent.reset();
    child->graphChanged = VRGlobals::get()->CURRENT_FRAME;
    updateChildrenIndices(true);
}

void VRObject::switchParent(VRObjectPtr new_p, int place) {
    if (new_p == 0) { cout << "\nERROR : new parent is 0!\n"; return; }

    if (getParent() == 0) { new_p->addChild(ptr(), true, place); return; }
    if (getParent() == new_p && place == childIndex) { return; }

    getParent()->subChild(ptr(), true);
    new_p->addChild(ptr(), true, place);
}

void VRObject::setIntern(bool b) { intern = b; }
bool VRObject::getIntern() { return intern; }

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

/** Returns the parent of ptr() object **/
VRObjectPtr VRObject::getParent() { return parent.lock(); }

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

vector<VRObjectPtr> VRObject::getChildren(bool recursive, string type) {
    if (!recursive) {
        if (type == "") return children;
        vector<VRObjectPtr> res;
        for (auto c : children) if (c->getType() == type) res.push_back(c);
        return res;
    }

    vector<VRObjectPtr> res = getChildren(false, type);
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

VRObjectPtr VRObject::find(NodeRecPtr n, string indent) {
    //cout << endl << indent << getName() << " " << node << " " << ptr() << flush;
    if (node == n) return ptr();
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
    if (graphChanged == VRGlobals::get()->CURRENT_FRAME) return true;
    if (getParent() == 0) return false;
    return getParent()->hasGraphChanged();
}

bool VRObject::hasAncestor(VRObjectPtr a) {
    if (ptr() == a) return true;
    if (getParent() == a) return true;
    if (getParent() != 0) return getParent()->hasAncestor(a);
    else return false;
}

/** Returns the Boundingbox of the OSG Node */
void VRObject::getBoundingBox(Vec3f& v1, Vec3f& v2) {
    Pnt3f p1, p2;
    commitChanges();
    node->updateVolume();
    node->getVolume().getBounds(p1, p2);
    v1 = p1.subZero();
    v2 = p2.subZero();
}

Vec3f VRObject::getBBCenter() {
    Vec3f v1, v2;
    getBoundingBox(v1, v2);

    return (v1+v2)*0.5;
}

Vec3f VRObject::getBBExtent() {
    Vec3f v1, v2;
    getBoundingBox(v1, v2);

    return v2-v1;
}

float VRObject::getBBMax() {
    Vec3f v1, v2;
    getBoundingBox(v1, v2);
    v1 = v2-v1;
    float r = 0;
    for (int i=0; i<3; i++) r = max(r,v1[i]);

    return r;
}

void VRObject::flattenHiarchy() {
    map<VRTransformPtr, Matrix> geos;
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

void VRObject::printOSGTree(NodeRecPtr o, string indent) {
    string type = o->getCore()->getTypeName();
    string name = "Unnamed";
    if (OSG::getName(o)) name = OSG::getName(o);

    // get attachments
    // print them

    cout << "\n" << indent << name << " " << type << "  ";
    if (type == "Transform") {
        Transform* t = dynamic_cast<Transform*>(o->getCore());
        cout << t->getMatrix()[0] << "  " << t->getMatrix()[1] << "  " << t->getMatrix()[2];
    }
    cout << flush;

    for (uint i=0; i<o->getNChildren(); i++) {
        printOSGTree(o->getChild(i), indent + " ");
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

/** Hide ptr() object && all his subgraph **/
void VRObject::hide() { node->setTravMask(0); visible = false; }

/** Show ptr() object && all his subgraph **/
void VRObject::show() { node->setTravMask(0xffffffff); visible = true; }

/** Returns if ptr() object is visible || not **/
bool VRObject::isVisible() { return visible; }


/** Set the visibility of ptr() object **/
void VRObject::setVisible(bool b) {
    if (b) show();
    else hide();
}

/** toggle visibility **/
void VRObject::toggleVisible() { setVisible(!visible); }

/** Returns if ptr() object is pickable || not **/
bool VRObject::isPickable() { return pickable == 1; }

/** Set the object pickable || not **/
void VRObject::setPickable(int b) { if (hasAttachment("transform")) pickable = b; } //TODO: check if the if is necessary!

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
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "" << flush;
    cout << "\nEnd Unit Test\n";
}

void VRObject::saveContent(xmlpp::Element* e) {
    VRName::saveName(e);
    e->set_attribute("type", type);
    e->set_attribute("pickable", toString(pickable));
    e->set_attribute("visible", toString(visible));
}

void VRObject::loadContent(xmlpp::Element* e) {
    VRName::loadName(e);
    type = e->get_attribute("type")->get_value();

    if (e->get_attribute("pickable")) toValue(e->get_attribute("pickable")->get_value(), pickable);
    if (e->get_attribute("visible")) toValue(e->get_attribute("visible")->get_value(), visible);
    setVisible(visible);
    setPickable(pickable);
}

void VRObject::save(xmlpp::Element* e) {
    saveContent(e);
}

void VRObject::load(xmlpp::Element* e) {
    loadContent(e);
}

OSG_END_NAMESPACE
