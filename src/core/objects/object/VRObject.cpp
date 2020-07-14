#include "VRObject.h"
#include "VRAttachment.h"
#include "OSGCore.h"
#include "../OSGObject.h"
#include "../VRTransform.h"

#include "core/math/pose.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/utils/VRStorage_template.h"
#include "core/scene/import/VRExport.h"

#include <OpenSG/OSGGroup.h>
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGVisitSubTree.h>
#include <OpenSG/OSGSceneFileHandler.h>

using namespace OSG;

template<> string typeName(const VRObject& o) {
    VRObject* O = (VRObject*)&o;
    return O->getType();
}

VRObject::VRObject(string _name) {
    static int _ID = 0;
    ID = _ID;
    _ID++;

    setName(_name);

    osg = OSGObject::create( makeNodeFor( Group::create() ) );
    OSG::setName(osg->node, name);
    type = "Object";

    setStorageType("Object");
    store("type", &type);
    store("pickable", &pickable);
    store("visible", &visibleMask);
    storeObjVec("children", children);
    storeMap("attachments", &attachments, true);

    regStorageSetupBeforeFkt( VRStorageCb::create("object setup", bind(&VRObject::setupBefore, this, placeholders::_1)) );
    regStorageSetupFkt( VRStorageCb::create("object setup", bind(&VRObject::setupAfter, this, placeholders::_1)) );
}

vector<VRObjectPtr> tmpChildren;

void VRObject::setupBefore(VRStorageContextPtr context) {
    bool onlyReload = false;
    if (context) onlyReload = context->onlyReload;
    if (!onlyReload) clearChildren();
    else tmpChildren = children;
}

void VRObject::setupAfter(VRStorageContextPtr context) {
    setVisibleMask(visibleMask);
    setPickable(pickable);

    bool onlyReload = false;
    if (context) onlyReload = context->onlyReload;
    if (!onlyReload) {
        auto tmp = children;
        children.clear();
        for (auto c : tmp) addChild(c);
    } else children = tmpChildren;
}

VRObject::~VRObject() {
    //cout << " ~VRObject " << getName() << endl;
    NodeMTRecPtr p;
    if (osg->node) p = osg->node->getParent();
    if (p) p->subChild(osg->node);
    for (auto a : attachments) delete a.second;
    attachments.clear();
}

Matrix4d VRObject::getMatrixTo(VRObjectPtr obj, bool parentOnly) {
    VRTransformPtr ent1 = VRTransform::getParentTransform(ptr());
    VRTransformPtr ent2 = VRTransform::getParentTransform(obj);

    Matrix4d m1, m2;
    if (ent1) m1 = ent1->getWorldMatrix(parentOnly);
    if (ent2) m2 = ent2->getWorldMatrix();
    if (!ent1) return m2;

    m1.invert();
    if (!ent2) return m1;

    m1.mult(m2);
    return m1;
}

PosePtr VRObject::getPoseTo(VRObjectPtr o) {
    auto m = getMatrixTo(o);
    return Pose::create(m);
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

void applyVolumeCheck(NodeMTRecPtr n, bool b) {
    if (!n) return;
    BoxVolume &vol = n->editVolume(false);
    vol.setInfinite(!b);
    vol.setStatic(!b);
    vol.setValid(!b);
}

void applyVolumeCheckRecursive(NodeMTRecPtr n, bool b) {
    if (!n) return;
    applyVolumeCheck(n, b);

    for (unsigned int i=0; i<n->getNChildren(); i++) {
        applyVolumeCheckRecursive(n->getChild(i), b);
    }
}

void VRObject::setVolumeCheck(bool b, bool recursive) {
    if (!getNode()) return;
    applyVolumeCheck(getNode()->node, b);
    if (recursive) applyVolumeCheckRecursive(getNode()->node, b);
}

void VRObject::setVolume(Boundingbox box) {
    BoxVolume &vol = getNode()->node->editVolume(false);
    vol.setBounds(Vec3f(box.min()), Vec3f(box.max()));
    vol.setStatic(true);
    vol.setValid(true);
}

VRObjectPtr VRObject::create(string name) { return VRObjectPtr(new VRObject(name) ); }
VRObjectPtr VRObject::ptr() { return static_pointer_cast<VRObject>( shared_from_this() ); }

void VRObject::printInformation() {;}

VRObjectPtr VRObject::copy(vector<VRObjectPtr> children) {
    if (getBaseName() == "DeviceBeacons") cout << "-------------------- VRObject::copy " << this << endl;
    VRObjectPtr o = VRObject::create(getBaseName());
    if (specialized) o->setCore(getCore(), getType());
    o->setPersistency(getPersistency());
    o->setVisibleMask(visibleMask);
    o->setPickable(pickable);
    o->setEntity(entity);
    return o;
}

int VRObject::getID() { return ID; }
string VRObject::getType() { return type; }
bool VRObject::hasTag(string name) { return attachments.count(name); }
void VRObject::remTag(string name) { remAttachment(name); }

void VRObject::addTag(string name) {
    if (!attachments.count(name)) attachments[name] = new VRAttachment(name);
}

void VRObject::remAttachment(string name) {
    if (attachments.count(name)) {
        delete attachments[name];
        attachments.erase(name);
    }
}

string VRObject::getAttachmentAsString(string name) {
    cout << "VRObject::getAttachmentAsString " << name << " " << getName() << " " << attachments.size() << endl;
    if (attachments.count(name)) {
        return attachments[name]->asString();
    }
    return "";
}

void VRObject::setAttachmentFromString(string name, string value) {
    if (!hasTag(name)) addAttachment(name, value);
    else attachments[name]->fromString(value);
}

vector<string> VRObject::getTags() {
    vector<string> res;
    for (auto a : attachments) res.push_back(a.first);
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

int VRObject::getTravMask() {
    return getNode()->node->getTravMask();
}

VRObjectPtr VRObject::getLink(int i) {
    if (i < 0 || i >= (int)links.size()) return 0;
    return links[i].second.lock();
}

vector<VRObjectPtr> VRObject::getLinks() {
    vector<VRObjectPtr> res;
    for (auto wl : links) if (auto l = wl.second.lock()) res.push_back(l);
    return res;
}

void VRObject::addLink(VRObjectPtr obj) {
    if (osg->links.count(obj.get())) return;

    NodeMTRecPtr node = obj->getNode()->node;
    VisitSubTreeMTRecPtr visitor = VisitSubTree::create();
    visitor->setSubTreeRoot(node);
    NodeMTRecPtr visit_node = makeNodeFor(visitor);
    OSG::setName(visit_node, getName()+"_link");
    addChild(OSGObject::create(visit_node));
    osg->links[obj.get()] = visit_node;

    for (auto l : links) if (l.first == obj.get()) return; // dont add links twice!
    links.push_back( make_pair(obj.get(), VRObjectWeakPtr(obj)) );
}

void VRObject::remLink(VRObjectPtr obj) {
    if (!osg->links.count(obj.get())) return;

    NodeMTRecPtr node = osg->links[obj.get()];
    subChild(OSGObject::create(node));
    osg->links.erase(obj.get());

    for (unsigned int i=0; i<links.size(); i++) {
        if (links[i].first == obj.get()) {
            links.erase(links.begin() + i);
            return;
        }
    }
}

void VRObject::clearLinks() {
    auto tmp = links;
    for (auto o : tmp) remLink(o.second.lock());
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

OSGCorePtr VRObject::getCore() { return core; }

void VRObject::switchCore(OSGCorePtr c) {
    if(!specialized) return;
    osg->node->setCore(c->core);
    core = c;
}

void VRObject::disableCore() { osg->node->setCore( Group::create() ); }
void VRObject::enableCore() { osg->node->setCore( core->core ); }

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
    if (!osg || !osg->node) { cout << "Warning! VRObject::addChild to " << getName() << ": bad osg parent node!\n"; return; }
    if (!n || !n->node) { cout << "Warning! VRObject::addChild to " << getName() << ": bad osg child node!\n"; return; }
    osg->node->addChild(n->node);
}

void VRObject::addChild(VRObjectPtr child, bool osg, int place) {
    if (child == 0 || child == ptr()) return;
    //cout << "VRObject::addChild " << child->getName() << "  to: " << getName() << endl;
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
    //cout << "VRObject::switchParent of: " << getName() << "  new parent: " << new_p->getName() << " destroyed? " << destroyed << endl;
    if (destroyed) { cout << "VRObject::switchParent ERROR: object is marked as destroyed!" << endl; return; }
    if (new_p == ptr()) return;
    if (new_p == 0) { cout << "VRObject::switchParent ERROR: new parent is 0!" << endl; return; }

    if (getParent() == 0) { new_p->addChild(ptr(), true, place); return; }
    if (getParent() == new_p && place == childIndex) { return; }

    getParent()->subChild(ptr(), true);
    new_p->addChild(ptr(), true, place);
}

size_t VRObject::getChildrenCount() { return children.size(); }

void VRObject::clearChildren(bool destroy) {
    int N = getChildrenCount();
    for (int i=N-1; i>=0; i--) {
        VRObjectPtr c = getChild(i);
        subChild( c );
        if (destroy) c->destroy();
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

vector<VRObjectPtr> VRObject::getChildrenWithTag(string tag, bool recursive, bool includeSelf) {
    if (!recursive) {
        vector<VRObjectPtr> res;
        for (auto c : children) if (c->hasTag(tag)) res.push_back(c);
        return res;
    }

    vector<VRObjectPtr> res = getChildrenWithTag(tag);
    if (includeSelf && (hasTag(tag))) res.push_back( ptr() );

    for (auto c : children) {
        vector<VRObjectPtr> tmp = c->getChildrenWithTag(tag, true);
        res.insert( res.end(), tmp.begin(), tmp.end() );
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
    for (auto& c : children) {
        VRObjectPtr tmp = c->find(n, indent+" ");
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::find(VRObjectPtr obj) {
    if (obj == ptr()) return ptr();
    for (auto& c : children) {
        VRObjectPtr tmp = c->find(obj);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::find(string Name) {
    if (name == Name) return ptr();
    for (auto& c : children) {
        if (c.get() == this) {
            cout << "ERROR!! 'this' is also its child!" << endl;
            continue; // workaround! TODO: find why ptr() can happn
        }
        VRObjectPtr tmp = c->find(Name);
        if (tmp != 0) return tmp;
    }
    return 0;
}

VRObjectPtr VRObject::findFirst(string Name) {
    if (base_name == Name) return ptr();
    for (auto& c : children) {
        if (c == ptr()) continue; // workaround! TODO: find why ptr() can happn
        VRObjectPtr tmp = c->findFirst(Name);
        if (tmp != 0) return tmp;
    }
    return 0;
}

vector<VRObjectPtr> VRObject::findAll(string Name, vector<VRObjectPtr> res ) {
    if (base_name == Name) res.push_back(ptr());
    for (auto& c : children) {
        if (c == ptr()) continue; // workaround! TODO: find why ptr() can happn
        res = c->findAll(Name, res);
    }
    return res;
}

VRObjectPtr VRObject::find(int id) {
    if (ID == -1) return 0;
    if (ID == id) return ptr();
    for (auto& c : children) {
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
    for (unsigned int i=0;i<children.size();i++)
        res = children[i]->filterByType(Type, res);
    return res;
}

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

BoundingboxPtr VRObject::getBoundingbox() {
    Pnt3f p1, p2;
    commitChanges();
    osg->node->updateVolume();
    osg->node->getVolume().getBounds(p1, p2);
    auto b = Boundingbox::create();
    b->update(Vec3d(p1));
    b->update(Vec3d(p2));
    return b;
}

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"

BoundingboxPtr VRObject::getWorldBoundingbox() {
    auto b = Boundingbox::create();
    for (auto obj : getChildren(true, "", true)) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) continue;
        Matrix4d M = geo->getWorldMatrix();
        auto pos = geo->getMesh()->geo->getPositions();
        for (unsigned int i=0; i<pos->size(); i++) {
            Pnt3d p = Pnt3d( pos->getValue<Pnt3f>(i) );
            M.mult(p,p);
            b->update(Vec3d(p));
        }
    }
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

void VRObject::printTree(int indent) {
    if(indent == 0) cout << "\nPrint Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    cout << "name: " << name << " ID: " << ID << " type: " << type << " pnt: " << ptr();
    printInformation();
    for (unsigned int i=0;i<children.size();i++) children[i]->printTree(indent+1);

    if(indent == 0) cout << "\n";
}

vector<OSGObjectPtr> VRObject::getNodes() {
    vector<OSGObjectPtr> nodes;

    function< void (NodeMTRecPtr)> aggregate = [&](NodeMTRecPtr node) {
        nodes.push_back( OSGObject::create(node) );
        for (unsigned int i=0; i<node->getNChildren(); i++) aggregate( node->getChild(i) );
    };

    aggregate(getNode()->node);

    return nodes;
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

    for (unsigned int i=0; i<o->node->getNChildren(); i++) {
        printOSGTree(OSGObject::create(o->node->getChild(i)), indent + " ");
    }
}

VRObjectPtr VRObject::duplicate(bool anchor, bool subgraph) {
    vector<VRObjectPtr> children;

    if (subgraph) {
        int N = getChildrenCount();
        for (int i=0;i<N;i++) {// first duplicate all children
            VRObjectPtr d = getChild(i)->duplicate();
            children.push_back(d);// ptr() is not the objects children vector! (its local)
        }
    }

    VRObjectPtr o = copy(children); // copy himself
    for (unsigned int i=0; i<children.size();i++) o->addChild(children[i]); // append children
    if (anchor && getParent()) getParent()->addChild(o);
    return o;
}

void VRObject::updateChildrenIndices(bool recursive) {
    for (unsigned int i=0; i<children.size(); i++) {
        children[i]->childIndex = i;
        if (recursive) children[i]->updateChildrenIndices();
    }
}

int VRObject::findChild(VRObjectPtr node) {
    for (unsigned int i=0;i<children.size();i++)
        if (children[i] == node) return i;
    return -1;
}

void VRObject::hide(string mode) { setVisible(false, mode); }
void VRObject::show(string mode) { setVisible(true, mode); }

void VRObject::setVisibleUndo(unsigned int b) {
    setVisibleMask(b);
}

bool getBit(const unsigned int& mask, int bit) {
    return (mask & 1UL << bit);
}

void setBit(unsigned int& mask, int bit, bool value) {
    if (value) mask |= 1UL << bit;
    else mask &= ~(1UL << bit);
}

int getVisibleMaskBit(const string& mode) {
    if (mode == "SHADOW") return 1;
    return 0;
}

bool VRObject::isVisible(string mode, bool recursive) {
    int b = getVisibleMaskBit(mode);
    if (!recursive) return getBit(visibleMask, b);
    if (!isVisible(mode)) return false;
    if (auto p = parent.lock()) return p->isVisible(mode, true);
    return getBit(visibleMask, b);
}

void VRObject::exportToFile(string path) { // may crash due to strange charachters in object names
    if (!getNode()) return;
    VRExport::get()->write( ptr(), path );
}

void VRObject::setVisible(bool b, string mode) {
    int bit = getVisibleMaskBit(mode);
    if (getBit(visibleMask, bit) == b) return;
    auto oldMask = visibleMask;
    setBit(visibleMask, bit, b);
    if (undoInitiated()) recUndo(&VRObject::setVisibleUndo, ptr(), oldMask, visibleMask);
    setVisibleMask(visibleMask);
}

void VRObject::setVisibleMask(unsigned int mask) {
    visibleMask = mask;
    bool showObj = getBit(visibleMask, 0);
    bool showShadow = getBit(visibleMask, 1);

    unsigned int osgMask = 0;
    if (showObj) osgMask = 0xffffffff;
    setBit(osgMask, 4, showShadow);
    if (!showObj) osgMask = 0;
    setTravMask(osgMask);
}

void VRObject::toggleVisible(string mode) { setVisible(!isVisible(mode), mode); }

bool VRObject::isPickable() { return pickable == 1; }

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


void VRObject::reduceModel(string strategy) {
    map<string, string> strategies;
    for (auto strat : splitString(strategy, ';')) {
        string key = splitString(strat, ':')[0];
        string val = splitString(strat, ':')[1];
        strategies[key] = val;
    }

    vector<VRGeometryPtr> geos;
    for (auto obj : getChildren(true, "Geometry", true)) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
        geos.push_back( geo );
    }

    auto byBoundingRadius = [&](float f) {
        auto BB = getBoundingbox();
        float R = BB->radius();
        int N = 0;
        for (auto geo : geos) {
            auto bb = geo->getBoundingbox();
            float r = bb->radius();

            if (r < R*f) {
                N++;
                geo->hide();
            }
        }
        return N;
    };

    if (strategies.count("byBoundingRadius")) {
        int N = byBoundingRadius( toFloat(strategies["byBoundingRadius"]) );
        cout << "reduceModel " << getName() << ", N geos: " << geos.size() << ", N reduced: " << N << endl;
    }
}





