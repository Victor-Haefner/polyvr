#include "VRObject.h"
#include "../VRTransform.h"
#include "VRObjectT.h"
#include "VRAttachment.h"
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGTransform.h>
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


VRGlobals::VRGlobals() {}
VRGlobals* VRGlobals::get() {
    static VRGlobals* s = new VRGlobals();
    return s;
}


void VRObject::printInformation() {;}

VRObject* VRObject::copy(vector<VRObject*> children) {
    VRObject* o = new VRObject(getBaseName());
    if (specialized) o->setCore(getCore(), getType());
    o->setVisible(visible);
    o->setPickable(pickable);
    return o;
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
    int N = children.size();
    vector<VRObject*> chv;
    for(int i = 0;i<N; i++) chv.push_back(children[i]); // store children in tmp vector to avoid bad stuff

    for(int i = 0;i<N; i++) {
        chv[i]->detach();
        if ( ! chv[i]->hasAttachment("global") )
            delete chv[i];
    }

    if (parent) parent->subChild(this);

    NodeRecPtr p;
    if (node !=  0) p = node->getParent();
    if (p !=  0) p->subChild(node);
}

int VRObject::getID() { return ID; }
string VRObject::getType() { return type; }
bool VRObject::hasAttachment(string name) { return attachments.count(name); }
void VRObject::remAttachment(string name) { attachments.erase(name); }

vector<VRObject*> VRObject::getChildrenWithAttachment(string name) {
    vector<VRObject*> res;
    for (auto c : getChildren()) {
        if (c->hasAttachment(name)) res.push_back(c);
    }
    return res;
}

VRObject* VRObject::hasAncestorWithAttachment(string name) {
    if (hasAttachment(name)) return this;
    if (parent == 0) return 0;
    return parent->hasAncestorWithAttachment(name);
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
    if (parent == 0) return;
    if (i < 0 || i >= (int)parent->children.size()) return;

    NodeRecPtr p = parent->getNode();

    p->subChild(getNode());
    p->insertChild(i, getNode());

    parent->children.erase(std::find(parent->children.begin(), parent->children.end(), this));
    parent->children.insert(parent->children.begin() + i, this);
}

void VRObject::addChild(NodeRecPtr n) { node->addChild(n); }

void VRObject::addChild(VRObject* child, bool osg, int place) {
    if (child == 0) return;
    if (child->getParent() != 0) { child->switchParent(this, place); return; }

    if (osg) node->addChild(child->node);
    child->graphChanged = VRGlobals::get()->CURRENT_FRAME;
    child->childIndex = children.size();
    children.push_back(child);
    child->parent=this;
    child->setSiblingPosition(place);
    updateChildrenIndices(true);
}

int VRObject::getChildIndex() { return childIndex;}

void VRObject::subChild(NodeRecPtr n) { node->subChild(n); }
void VRObject::subChild(VRObject* child, bool osg) {
    if (osg) node->subChild(child->node);

    int target = findChild(child);

    if (target != -1) children.erase(children.begin() + target);
    if (child->parent == this) child->parent = 0;
    child->graphChanged = VRGlobals::get()->CURRENT_FRAME;
    updateChildrenIndices(true);
}

void VRObject::switchParent(VRObject* new_p, int place) {
    if (new_p == 0) { cout << "\nERROR : new parent is 0!\n"; return; }

    if (parent == 0) { new_p->addChild(this, true, place); return; }
    if (parent == new_p && place == childIndex) { return; }

    parent->subChild(this, true);
    new_p->addChild(this, true, place);
}

void VRObject::setIntern(bool b) { intern = b; }
bool VRObject::getIntern() { return intern; }

/** Returns the number of children **/
size_t VRObject::getChildrenCount() { return children.size(); }

void VRObject::clearChildren() {
    int N = getChildrenCount();
    for (int i=N-1; i>=0; i--) {
        VRObject* c = getChild(i);
        subChild( c );
        c->destroy();
    }
}

void VRObject::detach() {
    if (parent == 0) return;
    parent->subChild(this, true);
    parent = 0;
}

VRObject* VRObject::getChild(int i) {
    if (i < 0 || i >= (int)children.size()) return 0;
    return children[i];
}

/** Returns the parent of this object **/
VRObject* VRObject::getParent() { return parent; }

VRObject* VRObject::getAtPath(string path) {
    vector<int> pvec;
    stringstream ss(path);
    string item;
    while (getline(ss, item, ':')) pvec.push_back(toInt(item));

    pvec.erase( pvec.begin() ); // ditch first element (should be the local node)

    VRObject* res = this;
    for (int c : pvec) {
        if (res == 0) break;
        res = res->getChild(c);
    }

    return res;
}

vector<VRObject*> VRObject::getChildren(bool recursive, string type) {
    if (!recursive) {
        if (type == "") return children;
        vector<VRObject*> res;
        for (auto c : children) if (c->getType() == type) res.push_back(c);
        return res;
    }

    vector<VRObject*> res = getChildren(false, type);
    for (auto c : children) {
        vector<VRObject*> tmp = c->getChildren(true, type);
        res.insert( res.end(), tmp.begin(), tmp.end() );
    }
    return res;
}

vector<VRObject*> VRObject::getObjectListByType(string _type) {
    vector<VRObject*> v;
    getObjectListByType(_type, v);
    return v;
}

void VRObject::getObjectListByType(string _type, vector<VRObject*>& list) {
    if (type == "Camera") return;
    if (type == _type) list.push_back(this);
    for (auto c : children) c->getObjectListByType(_type, list);
}

VRObject* VRObject::find(NodeRecPtr n, string indent) {
    //cout << endl << indent << getName() << " " << node << " " << this << flush;
    if (node == n) return this;
    for (auto c : children) {
        VRObject* tmp = c->find(n, indent+" ");
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObject* VRObject::find(VRObject* obj) {
    if (obj == this) return this;
    for (auto c : children) {
        VRObject* tmp = c->find(obj);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObject* VRObject::find(string Name) {
    if (name == Name) return this;
    for (auto c : children) {
        if (c == this) continue; // workaround! TODO: find why this can happn
        VRObject* tmp = c->find(Name);
        if (tmp != 0) return tmp;
    }
    return 0;
}

vector<VRObject*> VRObject::findAll(string Name, vector<VRObject*> res ) {
    if (base_name == Name) res.push_back(this);
    for (auto c : children) {
        if (c == this) continue; // workaround! TODO: find why this can happn
        res = c->findAll(Name, res);
    }
    return res;
}

VRObject* VRObject::find(int id) {
    if (ID == -1) return 0;
    if (ID == id) return this;
    for (auto c : children) {
        VRObject* tmp = c->find(id);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObject* VRObject::getRoot() {
    VRObject* o = this;
    while (o->parent) o = o->parent;
    return o;
}

vector<VRObject*> VRObject::filterByType(string Type, vector<VRObject*> res) {
    if (type == Type) res.push_back(this);
    for (uint i=0;i<children.size();i++)
        res = children[i]->filterByType(Type, res);
    return res;
}

/** Returns the first ancestor that is pickable, || 0 if none found **/
VRObject* VRObject::findPickableAncestor() {
    if (isPickable()) return this;
    else if (parent == 0) return 0;
    else return parent->findPickableAncestor();
}

bool VRObject::hasGraphChanged() {
    if (graphChanged == VRGlobals::get()->CURRENT_FRAME) return true;
    if (parent == 0) return false;
    return parent->hasGraphChanged();
}

bool VRObject::hasAncestor(VRObject* a) {
    if (this == a) return true;
    if (parent == a) return true;
    if (parent != 0) return parent->hasAncestor(a);
    else return false;
}

/** Returns the Boundingbox of the OSG Node */
void VRObject::getBoundingBox(Vec3f& v1, Vec3f& v2) {
    Pnt3f p1, p2;
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
    map<VRTransform*, Matrix> geos;
    for(auto g : getChildren(true, "Geometry") ) {
        VRTransform* t = (VRTransform*)g;
        geos[t] = t->getWorldMatrix();
        t->detach();
    }

    clearChildren();

    for (auto g : geos) {
        addChild(g.first);
        g.first->setWorldMatrix(g.second);
    }
}

/** Print to console the scene subgraph starting at this object **/
void VRObject::printTree(int indent) {
    if(indent == 0) cout << "\nPrint Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    cout << "name: " << name << " ID: " << ID << " type: " << type << " pnt: " << this;
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

/** duplicate this object **/
VRObject* VRObject::duplicate(bool anchor) {
    vector<VRObject*> children;
    int N = getChildrenCount();
    for (int i=0;i<N;i++) {// first duplicate all children
        VRObject* d = getChild(i)->duplicate();
        children.push_back(d);// this is not the objects children vector! (its local)
    }

    VRObject* o = copy(children); // copy himself
    for (uint i=0; i<children.size();i++)
        o->addChild(children[i]); // append children

    if (anchor && parent) parent->addChild(o);
    return o;
}

void VRObject::destroy() { delete this; }

void VRObject::updateChildrenIndices(bool recursive) {
    for (uint i=0; i<children.size(); i++) {
        children[i]->childIndex = i;
        if (recursive) children[i]->updateChildrenIndices();
    }
}

int VRObject::findChild(VRObject* node) {
    for (uint i=0;i<children.size();i++)
        if (children[i] == node) return i;
    return -1;
}

/** Hide this object && all his subgraph **/
void VRObject::hide() { node->setTravMask(0); visible = false; }

/** Show this object && all his subgraph **/
void VRObject::show() { node->setTravMask(0xffffffff); visible = true; }

/** Returns if this object is visible || not **/
bool VRObject::isVisible() { return visible; }


/** Set the visibility of this object **/
void VRObject::setVisible(bool b) {
    if (b) show();
    else hide();
}

/** toggle visibility **/
void VRObject::toggleVisible() { setVisible(!visible); }

/** Returns if this object is pickable || not **/
bool VRObject::isPickable() {return pickable;}

/** Set the object pickable || not **/
void VRObject::setPickable(bool b) { if (hasAttachment("transform")) pickable = b; } //TODO: check if the if is necessary!

string VRObject::getPath() {
    VRObject* o = this;
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
    VRObject *o1, *o2, *o3;
    o1 = new VRObject("test1");
    o2 = new VRObject("test2");
    o3 = new VRObject("test3");
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
