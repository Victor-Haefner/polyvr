#include "VRTransform.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"
#include "core/scene/VRScene.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/geometry/VRConstraint.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/utils/VRDoublebuffer.h"
#include "core/utils/VRGlobals.h"
#include "core/scene/VRAnimationManagerT.h"
#include "core/math/pose.h"
#include "geometry/VRPhysics.h"
#include "core/math/path.h"
#include "core/objects/OSGObject.h"
#include "core/objects/OSGTransform.h"

#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGDepthChunk.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.


OSG_BEGIN_NAMESPACE;
using namespace std;

VRTransform::VRTransform(string name) : VRObject(name) {
    dm = new doubleBuffer;
    t = OSGTransform::create( Transform::create() );
    constraint = VRConstraint::create();
    setCore(OSGCore::create(t->trans), "Transform");
    disableCore();
    addAttachment("transform", 0);

    store("from", &_from);
    store("at", &_at);
    store("up", &_up);
    store("scale", &_scale);
    store("at_dir", &orientation_mode);
    storeObj("constraint", constraint);

    regStorageSetupFkt( VRFunction<int>::create("transform_update", boost::bind(&VRTransform::setup, this)) );
}

VRTransform::~VRTransform() {
    if (physics) { delete physics; }
    delete dm;
}

VRTransformPtr VRTransform::ptr() { return static_pointer_cast<VRTransform>( shared_from_this() ); }
VRTransformPtr VRTransform::create(string name) {
    auto ptr = shared_ptr<VRTransform>(new VRTransform(name) );
    //ptr->physics = new VRPhysics( ptr );
    return ptr;
}

VRObjectPtr VRTransform::copy(vector<VRObjectPtr> children) {
    VRTransformPtr geo = VRTransform::create(getBaseName());
    geo->setPickable(isPickable());
    geo->setMatrix(getMatrix());
    geo->setVisible(isVisible());
    geo->setPickable(isPickable());
    return geo;
}

void VRTransform::computeMatrix() {
    Matrix mm;
    MatrixLookAt(mm, _from, _at, _up);

    if (_scale != Vec3f(1,1,1)) {
        Matrix ms;
        ms.setScale(_scale);
        mm.mult(ms);
    }

    dm->write(mm);
}

//read matrix from doublebuffer && apply it to transformation
//should be called from the main thread only
void VRTransform::updatePhysics() {
    //update bullets transform
    if (physics == 0) return;
    if (noBlt && !held) { noBlt = false; return; }
    if (!physics->isPhysicalized()) return;

    /*Matrix m;
    dm->read(m);
    Matrix pm;
    getWorldMatrix(pm, true);
    pm.mult(m);*/
    physics->updateTransformation( ptr() );
    physics->pause();
    physics->resetForces();
}

bool isIdentity(const Matrix& m) {
    static Matrix r;
    static bool mSet = false;
    if (!mSet) { mSet = true; r.setIdentity(); }
    return (m == r);
}

void VRTransform::updateTransformation() {
    Matrix m;
    dm->read(m);

    if (!t->trans) return;

    bool isI = isIdentity(m);
    if (identity && !isI) {
        identity = false;
        enableCore();
    }

    if (!identity && isI) {
        identity = true;
        disableCore();
    }

    t->trans->setMatrix(m);
}

void VRTransform::reg_change() {
    if (change == false) {
        if (fixed) changedObjects.push_back( ptr() );
        change = true;
        change_time_stamp = VRGlobals::CURRENT_FRAME;
    }
}

void VRTransform::printInformation() { Matrix m; getMatrix(m); cout << " pos " << m[3]; }

uint VRTransform::getLastChange() { return change_time_stamp; }
//bool VRTransform::changedNow() { return (change_time_stamp >= VRGlobals::get()->CURRENT_FRAME-1); }
bool VRTransform::changedNow() { return checkWorldChange(); }

void VRTransform::initCoords() {
    if (coords != 0) return;
    coords = OSGObject::create( makeCoordAxis(0.3, 3, false) );
    coords->node->setTravMask(0);
    addChild(coords);
    GeometryMTRecPtr geo = dynamic_cast<Geometry*>(coords->node->getCore());
    ChunkMaterialRecPtr mat = ChunkMaterial::create();
    DepthChunkRecPtr depthChunk = DepthChunk::create();
    depthChunk->setFunc( GL_ALWAYS );
    mat->addChunk(depthChunk);
    mat->setSortKey(100);// render last
    geo->setMaterial(mat);
}

void VRTransform::initTranslator() { // TODO
    if (translator != 0) return;

    translator = OSGObject::create( makeCoordAxis(0.3, 3, false) );
    translator->node->setTravMask(0);
    addChild(translator);
    GeometryMTRecPtr geo = dynamic_cast<Geometry*>(translator->node->getCore());

    string shdr_vp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "   gl_FrontColor = gl_Color;"
    "}";

    /*string shdr_fp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "}";*/

    ChunkMaterialRecPtr mat = ChunkMaterial::create();
    mat->setSortKey(100);// render last
    SimpleSHLChunkRecPtr shader_chunk = SimpleSHLChunk::create();
    shader_chunk->setVertexProgram(shdr_vp.c_str());
    //shader_chunk->setVertexProgram(shdr_fp.c_str());
    mat->addChunk(shader_chunk);

    geo->setMaterial(mat);
}

Vec3f VRTransform::getFrom() { return _from; }
Vec3f VRTransform::getAt() { return _at; }
Vec3f VRTransform::getUp() { return _up; }

/** Returns the local matrix **/
//void VRTransform::getMatrix(Matrix& _m) { if(change) update(); dm.read(_m); }
void VRTransform::getMatrix(Matrix& _m) {
    if(change) {
        computeMatrix();
        updateTransformation();
    }
    dm->read(_m);
}

Matrix VRTransform::getMatrix() {
    Matrix m;
    getMatrix(m);
    return m;
}

Matrix VRTransform::getMatrixTo(VRObjectPtr obj) {
    VRTransformPtr ent; // get first transform object
    while(obj) {
        if (obj->hasAttachment("transform")) {
            ent = static_pointer_cast<VRTransform>(obj);
            break;
        }
        obj = obj->getParent();
    }

    Matrix m1, m2;
    if (ent) m1 = ent->getWorldMatrix();
    m2 = getWorldMatrix();
    if (!ent) return m2;

    m1.invert();
    m1.mult(m2);
    return m1;
}

bool VRTransform::checkWorldChange() {
    if (frame == 0) { frame = 1; return true; }
    if (change) return true;
    if (VRGlobals::CURRENT_FRAME == wchange_time_stamp) return true;
    if (hasGraphChanged()) return true;

    VRObjectPtr obj = ptr();
    VRTransformPtr ent;
    while(obj) {
        if (obj->hasAttachment("transform")) {
            ent = static_pointer_cast<VRTransform>(obj);
            if (ent->change_time_stamp > wchange_time_stamp) {
                wchange_time_stamp = VRGlobals::CURRENT_FRAME;
                return true;
            }
        }
        obj = obj->getParent();
    }

    return false;
}

/** Returns the world matrix **/
void VRTransform::getWorldMatrix(Matrix& M, bool parentOnly) {
    VRTransformPtr t = 0;
    M.setIdentity();

    Matrix m;
    VRObjectPtr o = ptr();
    if (parentOnly && o->getParent() != 0) o = o->getParent();

    while(o) {
        if (o->hasAttachment("transform")) {
            t = static_pointer_cast<VRTransform>(o);
            t->getMatrix(m);
            M.multLeft(m);
        }
        o = o->getParent();
    }
}

Matrix VRTransform::getWorldMatrix(bool parentOnly) {
    Matrix m;
    getWorldMatrix(m, parentOnly);
    return m;
}

/** Returns the world Position **/
Vec3f VRTransform::getWorldPosition(bool parentOnly) {
    Matrix m;
    getWorldMatrix(m, parentOnly);
    return Vec3f(m[3]);
}

/** Returns the direction vector (not normalized) **/
Vec3f VRTransform::getDir() { return _at-_from; }

/** Returns the world direction vector (not normalized) **/
Vec3f VRTransform::getWorldDirection(bool parentOnly) {
    Matrix m;
    getWorldMatrix(m, parentOnly);
    return Vec3f(m[2]);
}

/** Returns the world direction vector (not normalized) **/
Vec3f VRTransform::getWorldUp(bool parentOnly) {
    Matrix m;
    getWorldMatrix(m, parentOnly);
    return Vec3f(m[1]);
}

/** Set the object fixed or not **/
void VRTransform::setFixed(bool b) {
    if (b == fixed) return;
    fixed = b;
    VRTransformPtr This = ptr();

    dynamicObjects.remove_if([This](VRTransformWeakPtr p2){
        auto sp = p2.lock();
        return (This && sp) ? This == sp : false;
    });

    if (!b) dynamicObjects.push_back(This);
}

/** Set the world matrix of the object **/
void VRTransform::setWorldMatrix(Matrix m) {
    if (isNan(m)) return;

    Matrix wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    setMatrix(wm);
}

/** Set the world position of the object **/
void VRTransform::setWorldPosition(Vec3f pos) {
    if (isNan(pos)) return;

    Matrix m;
    m.setTranslate(pos);

    Matrix wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    _from = Vec3f(wm[3]);

    reg_change();
}

/** Set the world position of the object **/
void VRTransform::setWorldOrientation(Vec3f dir, Vec3f up) {
    if (isNan(dir)) return;

    Matrix m;
    MatrixLookAt(m, Vec3f(), dir, up);

    Matrix wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    wm.setTranslate(_from);
    setMatrix(wm);

    reg_change();
}

void VRTransform::setWorldDir(Vec3f dir) {
    setWorldOrientation(dir, getUp());
}

void VRTransform::setWorldUp(Vec3f up) {
    Matrix wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(up,up);
    setUp(up);
}

doubleBuffer* VRTransform::getBuffer() { return dm; }

//local pose setter--------------------
/** Set the from vector **/
void VRTransform::setFrom(Vec3f pos) {
    if (isNan(pos)) return;
    //cout << "\nSet From : " << name << getID() << " : " << pos;
    Vec3f dir;
    if (orientation_mode == OM_DIR) dir = _at - _from; // TODO: there may a better way
    _from = pos;
    if (orientation_mode == OM_DIR) _at = _from + dir;
    reg_change();
}

/** Set the at vector **/
void VRTransform::setAt(Vec3f at) {
    if (isNan(at)) return;
    _at = at;
    orientation_mode = OM_AT;
    reg_change();
}

/** Set the up vector **/
void VRTransform::setUp(Vec3f up) {
    if (isNan(up)) return;
    _up = up;
    reg_change();
}

void VRTransform::setDir(Vec3f dir) {
    if (isNan(dir)) return;
    _at = _from + dir;
    orientation_mode = OM_DIR;
    reg_change();
}

int VRTransform::get_orientation_mode() { return orientation_mode; }
void VRTransform::set_orientation_mode(int b) { orientation_mode = b; }

/** Set the orientation of the object with the at && up vectors **/
void VRTransform::setOrientation(Vec3f at, Vec3f up) {
    if (isNan(at) || isNan(up)) return;
    _at = at;
    _up = up;
    reg_change();
}

/** Set the pose of the object with the from, at && up vectors **/
void VRTransform::setPose(Vec3f from, Vec3f dir, Vec3f up) {
    if (isNan(from) || isNan(dir) || isNan(up)) return;
    _from = from;
    _up = up;
    setDir(dir);
    reg_change();
}

void VRTransform::setPose(posePtr p) { setPose(p->pos(), p->dir(), p->up()); }
posePtr VRTransform::getPose() { return pose::create(_from, getDir(), _up); }
posePtr VRTransform::getWorldPose() { return pose::create(getWorldPosition(), getWorldDirection(), getWorldUp()); }
void VRTransform::setWorldPose(posePtr p) { setWorldMatrix(p->asMatrix()); }

/** Set the local matrix **/
void VRTransform::setMatrix(Matrix m) {
    if (isNan(m)) return;

    /*float s1 = Vec3f(_m[0][0], _m[1][0], _m[2][0]).length();
    float s2 = Vec3f(_m[0][1], _m[1][1], _m[2][1]).length();
    float s3 = Vec3f(_m[0][2], _m[1][2], _m[2][2]).length();*/

    float s1 = m[0].length(); //TODO: check if this is fine
    float s2 = m[1].length();
    float s3 = m[2].length();

    setPose(Vec3f(m[3]), Vec3f(-m[2])*1.0/s3, Vec3f(m[1])*1.0/s2);
    setScale(Vec3f(s1,s2,s3));
}
//-------------------------------------

void VRTransform::showCoordAxis(bool b) {
    initCoords();
    if (b) coords->node->setTravMask(0xffffffff);
    else coords->node->setTravMask(0);
}

/** Set the scale of the object, not implemented **/
void VRTransform::setScale(float s) { setScale(Vec3f(s,s,s)); }

void VRTransform::setScale(Vec3f s) {
    if (isNan(s)) return;
    //cout << "setScale " << s << endl;
    _scale = s;
    reg_change();
}

void VRTransform::setEuler(Vec3f e) {
    if (isNan(e)) return;
    _euler = e;
    Vec3f s = Vec3f(sin(e[0]), sin(e[1]), sin(e[2]));
    Vec3f c = Vec3f(cos(e[0]), cos(e[1]), cos(e[2]));

    Vec3f d = Vec3f( -c[0]*c[2]*s[1]-s[0]*s[2], -c[0]*s[1]*s[2]+s[0]*c[2], -c[0]*c[1]);
    Vec3f u = Vec3f( s[0]*s[1]*c[2]-s[2]*c[0], s[0]*s[1]*s[2]+c[2]*c[0], c[1]*s[0]);

    setDir( d );
    setUp( u );
    reg_change();
}

Vec3f VRTransform::getScale() { return _scale; }
Vec3f VRTransform::getEuler() {
    //return _euler;
    auto m = getMatrix();
    Vec3f a;
    a[0] = atan2( m[1][2], m[2][2]);
    a[1] = atan2(-m[0][2], sqrt(m[1][2]*m[1][2] + m[2][2]*m[2][2]));
    a[2] = atan2( m[0][1], m[0][0]);
    return a;
}

void VRTransform::rotate(float a, Vec3f v) {//rotate around axis
    if (isNan(a) || isNan(v)) return;
    Vec3f d = _at - _from;

    v.normalize();
    Quaternion q = Quaternion(v, a);
    q.multVec(d,d);
    q.multVec(_up,_up);

    _at = _from + d;

    reg_change();
}

void VRTransform::rotateUp(float a) {//rotate around _up axis
    if (isNan(a)) return;
    Vec3f d = _at - _from;
    d.normalize();

    Quaternion q = Quaternion(d, a);
    q.multVec(_up,_up);

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

        /** Rotate the object around its x axis **/
void VRTransform::rotateX(float a) {//rotate around x axis
    if (isNan(a)) return;
    Vec3f dir = _at - _from;
    Vec3f d = dir.cross(_up);
    d.normalize();

    Quaternion q = Quaternion(d, a);
    q.multVec(_up,_up);
    q.multVec(dir,dir);
    _at = _from + dir;

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

/** Rotate the object around the point where at indicates && the up axis **/
void VRTransform::rotateAround(float a) {//rotate around focus using up axis
    if (isNan(a)) return;
    orientation_mode = OM_AT;
    Vec3f d = _at - _from;

    Quaternion q = Quaternion(_up, -a);
    q.multVec(d,d);

    _from = _at - d;

    reg_change();
}

/** translate the object with a vector v, this changes the from && at vector **/
void VRTransform::translate(Vec3f v) {
    if (isNan(v)) return;
    _at += v;
    _from += v;
    reg_change();
}

/** translate the object by changing the from in direction of the at vector **/
void VRTransform::zoom(float d) {
    if (isNan(d)) return;
    Vec3f dv = _at-_from;
    /*float norm = dv.length();
    dv /= norm;
    _from += dv*d*norm;*/
    _from += dv*d;
    reg_change();
}

/** Translate the object towards at **/
void VRTransform::move(float d) {
    if (isNan(d)) return;
    Vec3f dv = _at-_from;
    dv.normalize();
    _at += dv*d;
    _from += dv*d;
    reg_change();
}

/** Drag the object with another **/
void VRTransform::drag(VRTransformPtr new_parent) {
    if (held) return;
    held = true;
    old_parent = getParent();
    old_child_id = getChildIndex();
    setFixed(false);

    //showTranslator(true); //TODO

    Matrix m;
    old_transformation = getMatrix();
    getWorldMatrix(m);
    switchParent(new_parent);
    setWorldMatrix(m);

    if (physics) {
        physics->updateTransformation( ptr() );
        physics->resetForces();
        physics->pause(true);
    }
    reg_change();
    update();
}

/** Drop the object, this returns the object at its old place in hirarchy **/
void VRTransform::drop() {
    if (!held) return;
    held = false;

    bool dyn = constraint ? constraint->hasConstraint() : false;
    setFixed(!dyn);

    Matrix wm, m1, m2;
    getWorldMatrix(wm);
    m1 = getMatrix();
    if (auto p = old_parent.lock()) switchParent(p, old_child_id);
    setWorldMatrix(wm);
    recUndo(&VRTransform::setMatrix, ptr(), old_transformation, getMatrix());

    if (physics) {
        physics->updateTransformation( ptr() );
        physics->resetForces();
        physics->pause(false);
    }
    reg_change();
    update();
}

void VRTransform::rebaseDrag(VRObjectPtr new_parent) {
    if (!held) { switchParent(new_parent); return; }
    old_parent = new_parent;
}

bool VRTransform::isDragged() { return held; }
VRObjectPtr VRTransform::getDragParent() { return old_parent.lock(); }

/** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
Line VRTransform::castRay(VRObjectPtr obj, Vec3f dir) {
    Matrix m = getWorldMatrix();
    if (obj) obj = obj->getParent();

    if (obj != 0) {
        while (!obj->hasAttachment("transform")) { obj = obj->getParent(); if(obj == 0) break; }
        if (obj != 0) {
            VRTransformPtr tr = static_pointer_cast<VRTransform>(obj);
            Matrix om = tr->getWorldMatrix();
            om.invert();
            om.mult(m);
            m = om;
        }
    }

    m.mult(dir,dir); dir.normalize();
    Pnt3f p0 = Vec3f(m[3]);

    Line ray;
    ray.setValue(p0, dir);
    return ray;
}

/** Print the position of the object in local && world coords **/
void VRTransform::printPos() {
    Matrix wm, wm_osg, lm;
    getWorldMatrix(wm);
    wm_osg = getNode()->node->getToWorld();
    getMatrix(lm);
    cout << "Position of " << getName() << ", local: " << Vec3f(lm[3]) << ", world: " << Vec3f(wm[3]) << "  " << Vec3f(wm_osg[3]);
}

/** Print the positions of all the subtree **/
void VRTransform::printTransformationTree(int indent) {
    if(indent == 0) cout << "\nPrint Transformation Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    if (getType() == "Transform" || getType() == "Geometry") {
        printPos();
    }

    for (uint i=0;i<getChildrenCount();i++) {
        if (getChild(i)->getType() == "Transform" || getChild(i)->getType() == "Geometry") {
            VRTransformPtr tmp = static_pointer_cast<VRTransform>( getChild(i) );
            tmp->printTransformationTree(indent+1);
        }
    }

    if(indent == 0) cout << "\n";
}

void VRTransform::setConstraint(VRConstraintPtr c) { constraint = c; }
VRConstraintPtr VRTransform::getConstraint() { return constraint; }

/** enable constraints on the object, 0 leaves the DOF free, 1 restricts it **/
void VRTransform::apply_constraints() {
    if (!constraint) return;
    constraint->apply(ptr());
}

void VRTransform::updateFromBullet() {
    if (held) return;
    Matrix m = physics->getTransformation();
    setWorldMatrix(m);
    auto vs = physics->getVisualShape();
    if (vs && vs->isVisible()) vs->setWorldMatrix(m);
    setNoBltFlag();
}

void VRTransform::setNoBltFlag() { noBlt = true; }

VRPhysics* VRTransform::getPhysics() {
    if (physics == 0) physics = new VRPhysics( ptr() );
    return physics;
}

/** Update the object OSG transformation **/
void VRTransform::update() {
    if (checkWorldChange()) apply_constraints();
    if (held) updatePhysics();
    //if (checkWorldChange()) updatePhysics();

    if (!change) return;
    computeMatrix();
    updateTransformation();
    updatePhysics();
    change = false;
}

void VRTransform::setup() {
    change = true;
    update();
}

void setFromPath(VRTransformWeakPtr trp, pathPtr p, bool redirect, float t) {
    auto tr = trp.lock();
    if (!tr) return;
    tr->setFrom( p->getPosition(t) );
    if (!redirect) return;

    Vec3f d,u;
    p->getOrientation(t, d, u);
    tr->setUp( u );
    if (tr->get_orientation_mode() == VRTransform::OM_DIR) tr->setDir( d );
    if (tr->get_orientation_mode() == VRTransform::OM_AT) tr->setAt( p->getColor(t) );
}

void VRTransform::addAnimation(VRAnimationPtr anim) { animations[anim->getName()] = anim; }
vector<VRAnimationPtr> VRTransform::getAnimations() {
    vector<VRAnimationPtr> res;
    for (auto a : animations) res.push_back(a.second);
    return res;
}

VRAnimationPtr VRTransform::startPathAnimation(pathPtr p, float time, float offset, bool redirect, bool loop) {
    pathAnimPtr = VRFunction<float>::create("TransAnim", boost::bind(setFromPath, VRTransformWeakPtr(ptr()), p, redirect, _1));
    auto scene = VRScene::getCurrent();
    auto a = scene->addAnimation<float>(time, offset, pathAnimPtr, 0.f, 1.f, loop);
    addAnimation(a);
    return a;
}

void VRTransform::stopAnimation() {
    for (auto a : animations) a.second->stop();
}

list<VRTransformWeakPtr > VRTransform::dynamicObjects = list<VRTransformWeakPtr >();
list<VRTransformWeakPtr > VRTransform::changedObjects = list<VRTransformWeakPtr >();

OSG_END_NAMESPACE;
