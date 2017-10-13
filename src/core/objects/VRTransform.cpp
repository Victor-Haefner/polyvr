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
#include "core/scene/VRSpaceWarper.h"
#include "core/math/pose.h"
#include "geometry/VRGeometry.h"
#include "geometry/OSGGeometry.h"
#include "geometry/VRPhysics.h"
#include "core/math/path.h"
#include "core/objects/OSGObject.h"
#include "core/objects/OSGTransform.h"

#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGQuaternion.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGDepthChunk.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.

template<> string typeName(const OSG::VRTransformPtr& t) { return "Transform"; }

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
    //store("dir", &_dir);
    store("at", &_at);
    store("up", &_up);
    store("scale", &_scale);
    store("at_dir", &orientation_mode);
    storeObj("constraint", constraint);

    regStorageSetupFkt( VRUpdateCb::create("transform_update", boost::bind(&VRTransform::setup, this)) );
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
    geo->old_parent = getParent();
    return geo;
}

bool MatrixLookDir(Matrix4d &result, Pnt3d from, Vec3d dir, Vec3d up) {
    dir.normalize();
    Vec3d right = up.cross(dir);
    if (right.dot(right) < TypeTraits<Real32>::getDefaultEps()) return true;
    right.normalize();
    Vec3d newup = dir.cross(right);
    result.setIdentity();
    result.setTranslate(from);
    Matrix4d tmpm;
    tmpm.setValue(right, newup, dir);
    result.mult(tmpm);
    return false;
}

bool MatrixLookAt(Matrix4d &result, Pnt3d from, Pnt3d at, Vec3d up) {
    Vec3d dir = from - at;
    return MatrixLookDir(result, from, dir, up);
}

void VRTransform::computeMatrix4d() {
    Matrix4d mm;
    if (orientation_mode == OM_AT) MatrixLookAt(mm, _from, _at, _up);
    if (orientation_mode == OM_DIR) MatrixLookDir(mm, _from, -_dir, _up);

    if (_scale != Vec3d(1,1,1)) {
        Matrix4d ms;
        ms.setScale(_scale);
        mm.mult(ms);
    }

    dm->write(mm);
}

void VRTransform::setIdentity() {
    setMatrix(Matrix4d());
}

//read Matrix4d from doublebuffer && apply it to transformation
//should be called from the main thread only
void VRTransform::updatePhysics() {
    //update bullets transform
    if (physics == 0) return;
    if (noBlt && !held) { noBlt = false; return; }
    if (!physics->isPhysicalized()) return;

    /*Matrix4d m;
    dm->read(m);
    Matrix4d pm;
    getWorldMatrix(pm, true);
    pm.mult(m);*/
    physics->updateTransformation( ptr() );
    physics->pause();
    physics->resetForces();
}

bool isIdentity(const Matrix4d& m) {
    static Matrix4d r;
    static bool mSet = false;
    if (!mSet) { mSet = true; r.setIdentity(); }
    return (m == r);
}

void VRTransform::updateTransformation() {
    Matrix4d m;
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

    auto scene = VRScene::getCurrent();
    if (scene) {
        auto sw = scene->getSpaceWarper();
        if (sw) sw->warp(m);
    }
    t->trans->setMatrix(toMatrix4f(m));
}

void VRTransform::reg_change() {
    if (change == false) {
        if (fixed) changedObjects.push_back( ptr() );
        change = true;
        change_time_stamp = VRGlobals::CURRENT_FRAME;
    }
}

void VRTransform::printInformation() { Matrix4d m; getMatrix(m); cout << " pos " << m[3]; }

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
    "   gl_Position = gl_ModelViewProjectionMatrix4d*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "   gl_FrontColor = gl_Color;"
    "}";

    /*string shdr_fp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix4d*gl_Vertex;"
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

Vec3d VRTransform::getFrom() { return Vec3d(_from); }
Vec3d VRTransform::getDir() { return Vec3d(_dir); }
Vec3d VRTransform::getAt() { return Vec3d(_at); }
Vec3d VRTransform::getUp() { return Vec3d(_up); }

/** Returns the local Matrix4d **/
void VRTransform::getMatrix(Matrix4d& _m) {
    if(change) {
        computeMatrix4d();
        updateTransformation();
    }
    dm->read(_m);
}

Matrix4d VRTransform::getMatrix() {
    Matrix4d m;
    getMatrix(m);
    return m;
}

Matrix4d VRTransform::getRotationMatrix() {
    Matrix4d m;
    getMatrix(m);
    m[3][0] = m[3][1] = m[3][2] = 0;
    return m;
}

Matrix4d VRTransform::getMatrixTo(VRObjectPtr obj, bool parentOnly) {
    VRTransformPtr ent = getParentTransform(obj);

    Matrix4d m1, m2;
    if (ent) m1 = ent->getWorldMatrix();
    m2 = getWorldMatrix(parentOnly);
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
        if (obj->hasTag("transform")) {
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

Vec3d VRTransform::getRelativePosition(VRObjectPtr o, bool parentOnly) {
    return getRelativePose(o, parentOnly)->pos();
}

Vec3d VRTransform::getRelativeDirection(VRObjectPtr o, bool parentOnly) {
    return getRelativePose(o, parentOnly)->dir();
}

Vec3d VRTransform::getRelativeUp(VRObjectPtr o, bool parentOnly) {
    return getRelativePose(o, parentOnly)->up();
}

void VRTransform::getRelativeMatrix(Matrix4d& m, VRObjectPtr o, bool parentOnly) {
    m = getWorldMatrix(parentOnly);
    VRTransformPtr ent = VRTransform::getParentTransform(o);
    if (ent) {
        Matrix4d sm = ent->getWorldMatrix();
        sm.invert();
        m.multLeft(sm);
    }
}

Matrix4d VRTransform::getRelativeMatrix(VRObjectPtr o, bool parentOnly) {
    Matrix4d m;
    getRelativeMatrix(m,o,parentOnly);
    return m;
}

PosePtr VRTransform::getRelativePose(VRObjectPtr o, bool parentOnly) { return Pose::create( getRelativeMatrix(o,parentOnly) ); }

/** Returns the world Matrix4d **/
void VRTransform::getWorldMatrix(Matrix4d& M, bool parentOnly) {
    VRTransformPtr t = 0;
    M.setIdentity();

    Matrix4d m;
    VRObjectPtr o = ptr();
    if (parentOnly && o->getParent() != 0) o = o->getParent();

    while(o) {
        if (o->hasTag("transform")) {
            t = static_pointer_cast<VRTransform>(o);
            t->getMatrix(m);
            M.multLeft(m);
        }
        o = o->getParent();
    }
}

Matrix4d VRTransform::getWorldMatrix(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return m;
}

/** Returns the world Position **/
Vec3d VRTransform::getWorldPosition(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return Vec3d(m[3]);
}

/** Returns the world direction vector (not normalized) **/
Vec3d VRTransform::getWorldDirection(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return -Vec3d(m[2]);
}

/** Returns the world direction vector (not normalized) **/
Vec3d VRTransform::getWorldUp(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return Vec3d(m[1]);
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

/** Set the world Matrix4d of the object **/
void VRTransform::setWorldMatrix(Matrix4d m) {
    if (isNan(m)) return;
    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    setMatrix(wm);
}

VRTransformPtr VRTransform::getParentTransform(VRObjectPtr o) {
    o = o->hasAncestorWithTag("transform");
    return static_pointer_cast<VRTransform>(o);
}

void VRTransform::setRelativePose(PosePtr p, VRObjectPtr o) {
    Matrix4d m = p->asMatrix();
    Matrix4d wm = getMatrixTo(o);
    wm.invert();
    wm.mult(m);

    Matrix4d lm = getMatrix();
    lm.mult(wm);
    setMatrix( lm );
}

void VRTransform::setRelativePosition(Vec3d pos, VRObjectPtr o) {
    if (isNan(pos)) return;
    auto p = getRelativePose(o);
    p->setPos(pos);
    setRelativePose(p,o);
}

void VRTransform::setRelativeDir(Vec3d dir, VRObjectPtr o) {
    if (isNan(dir)) return;
    auto p = getRelativePose(o);
    p->setDir(dir);
    setRelativePose(p,o);
}

void VRTransform::setRelativeUp(Vec3d up, VRObjectPtr o) {
    if (isNan(up)) return;
    auto p = getRelativePose(o);
    p->setUp(up);
    setRelativePose(p,o);
}

/** Set the world position of the object **/
void VRTransform::setWorldPosition(Vec3d pos) {
    if (isNan(pos)) return;

    Matrix4d m;
    m.setTranslate(pos);

    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    setFrom( Vec3d(wm[3]) );
}

/** Set the world position of the object **/
void VRTransform::setWorldOrientation(Vec3d dir, Vec3d up) {
    if (isNan(dir)) return;

    Matrix4d m;
    MatrixLookAt(m, Vec3d(), Vec3d(dir), Vec3d(up));

    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    wm.setTranslate(Pnt3d(_from));
    setMatrix(wm);

    reg_change();
}

void VRTransform::setWorldDir(Vec3d dir) {
    setWorldOrientation(dir, getUp());
}

void VRTransform::setWorldUp(Vec3d up) {
    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(up,up);
    setUp(up);
}

doubleBuffer* VRTransform::getBuffer() { return dm; }

//local pose setter--------------------
/** Set the from vector **/
void VRTransform::setFrom(Vec3d pos) {
    if (isNan(pos)) return;
    _from = pos;
    if (orientation_mode == OM_DIR) _at = _from + _dir;
    if (orientation_mode == OM_AT) _dir = _at - _from;
    reg_change();
}

/** Set the at vector **/
void VRTransform::setAt(Vec3d at) {
    if (isNan(at)) return;
    _at = at;
    _dir = _at - _from;
    orientation_mode = OM_AT;
    reg_change();
}

/** Set the up vector **/
void VRTransform::setUp(Vec3d up) {
    if (isNan(up)) return;
    _up = up;
    reg_change();
}

void VRTransform::setDir(Vec3d dir) {
    if (isNan(dir)) return;
    _dir = dir;
    _at = _from + _dir;
    orientation_mode = OM_DIR;
    reg_change();
}

int VRTransform::get_orientation_mode() { return orientation_mode; }
void VRTransform::set_orientation_mode(int b) { orientation_mode = b; }

/** Set the orientation of the object with the at and up vectors **/
void VRTransform::setOrientation(Vec3d at, Vec3d up) {
    if (isNan(at) || isNan(up)) return;
    _at = at;
    _up = up;
    reg_change();
}

/** Set the pose of the object with the from, at and up vectors **/
void VRTransform::setPose(Vec3d from, Vec3d dir, Vec3d up) {
    if (isNan(from) || isNan(dir) || isNan(up)) return;
    _from = from;
    _up = up;
    setDir(dir);
}

void VRTransform::setPose(const Pose& p) { setPose(p.pos(), p.dir(), p.up()); }
void VRTransform::setPose(PosePtr p) { setPose(p->pos(), p->dir(), p->up()); }
PosePtr VRTransform::getPose() { return Pose::create(Vec3d(_from), Vec3d(_dir), Vec3d(_up)); }
PosePtr VRTransform::getWorldPose() { return Pose::create( getWorldMatrix() ); }
void VRTransform::setWorldPose(PosePtr p) { setWorldMatrix(p->asMatrix()); }

PosePtr VRTransform::getPoseTo(VRObjectPtr o) {
    auto m = getMatrixTo(o);
    return Pose::create(m);
}

/** Set the local Matrix4d **/
void VRTransform::setMatrix(Matrix4d m) {
    if (isNan(m)) return;

    /*float s1 = Vec3d(_m[0][0], _m[1][0], _m[2][0]).length();
    float s2 = Vec3d(_m[0][1], _m[1][1], _m[2][1]).length();
    float s3 = Vec3d(_m[0][2], _m[1][2], _m[2][2]).length();*/

    float s1 = m[0].length(); //TODO: check if this is fine
    float s2 = m[1].length();
    float s3 = m[2].length();

    setPose(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2);
    setScale(Vec3d(s1,s2,s3));
}
//-------------------------------------

void VRTransform::showCoordAxis(bool b) {
    initCoords();
    if (b) coords->node->setTravMask(0xffffffff);
    else coords->node->setTravMask(0);
}

/** Set the scale of the object, not implemented **/
void VRTransform::setScale(float s) { setScale(Vec3d(s,s,s)); }

void VRTransform::setScale(Vec3d s) {
    if (isNan(s)) return;
    //cout << "setScale " << s << endl;
    _scale = s;
    reg_change();
}

void VRTransform::setEuler(Vec3d e) {
    if (isNan(e)) return;
    _euler = e;
    Vec3d s = Vec3d(sin(e[0]), sin(e[1]), sin(e[2]));
    Vec3d c = Vec3d(cos(e[0]), cos(e[1]), cos(e[2]));

    Vec3d d = Vec3d( -c[0]*c[2]*s[1]-s[0]*s[2], -c[0]*s[1]*s[2]+s[0]*c[2], -c[0]*c[1]);
    Vec3d u = Vec3d( s[0]*s[1]*c[2]-s[2]*c[0], s[0]*s[1]*s[2]+c[2]*c[0], c[1]*s[0]);

    setDir( d );
    setUp( u );
    reg_change();
}

Vec3d VRTransform::getScale() { return _scale; }
Vec3d VRTransform::getEuler() {
    //return _euler;
    auto m = getMatrix();
    Vec3d a;
    a[0] = atan2( m[1][2], m[2][2]);
    a[1] = atan2(-m[0][2], sqrt(m[1][2]*m[1][2] + m[2][2]*m[2][2]));
    a[2] = atan2( m[0][1], m[0][0]);
    return a;
}

void VRTransform::rotate(float a, Vec3d v) {//rotate around axis
    if (isNan(a) || isNan(v)) return;
    v.normalize();
    Quaterniond q(Vec3d(v), a);
    q.multVec(_dir,_dir);
    q.multVec(_up,_up);
    _at = _from + _dir;
    reg_change();
}

void VRTransform::rotateUp(float a) {//rotate the _up axis
    if (isNan(a)) return;
    Vec3d d = _dir;
    d.normalize();
    Quaterniond q(d, a);
    q.multVec(_up,_up);
    reg_change();
}

        /** Rotate the object around its x axis **/
void VRTransform::rotateX(float a) {//rotate around x axis
    if (isNan(a)) return;
    Vec3d d = _dir.cross(_up);
    d.normalize();
    Quaterniond q(d, a);
    q.multVec(_up,_up);
    q.multVec(_dir,_dir);
    _at = _from + _dir;
    reg_change();
}

/** Rotate the object around the point where at indicates && the up axis **/
void VRTransform::rotateAround(float a) {//rotate around focus using up axis
    if (isNan(a)) return;
    orientation_mode = OM_AT;
    Quaterniond q(_up, -a);
    q.multVec(_dir,_dir);
    _from = _at - _dir;
    reg_change();
}

void VRTransform::rotateYonZ() {
    rotate(Pi*0.5, Vec3d(1,0,0));
}

/** translate the object with a vector v, this changes the from && at vector **/
void VRTransform::translate(Vec3d v) {
    if (isNan(v)) return;
    _at += Vec3d(v);
    _from += Vec3d(v);
    reg_change();
}

/** translate the object by changing the from in direction of the at vector **/
void VRTransform::zoom(float d) {
    if (isNan(d)) return;
    _from += _dir*d;
    _dir = _at - _from;
    reg_change();
}

/** Translate the object towards at **/
void VRTransform::move(float d) {
    if (isNan(d)) return;
    Vec3d dv = _dir;
    dv.normalize();
    translate(Vec3d(dv*d));
}

/** Drag the object with another **/
void VRTransform::drag(VRTransformPtr new_parent) {
    if (held) return;
    held = true;
    if (auto p = getParent()) old_parent = p;
    old_child_id = getChildIndex();
    setFixed(false);

    //showTranslator(true); //TODO

    Matrix4d m;
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
    updateChange();
}

/** Drop the object, this returns the object at its old place in hirarchy **/
void VRTransform::drop() {
    if (!held) return;
    held = false;

    bool dyn = constraint ? constraint->hasConstraint() : false;
    setFixed(!dyn);

    Matrix4d wm, m1, m2;
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
    updateChange();
}

void VRTransform::rebaseDrag(VRObjectPtr new_parent) {
    if (!held) { switchParent(new_parent); return; }
    old_parent = new_parent;
}

bool VRTransform::isDragged() { return held; }
VRObjectPtr VRTransform::getDragParent() { return old_parent.lock(); }

/** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
Line VRTransform::castRay(VRObjectPtr obj, Vec3d dir) {
    Matrix4d m = getWorldMatrix();
    if (obj) obj = obj->getParent();

    if (obj != 0) {
        while (!obj->hasTag("transform")) { obj = obj->getParent(); if(obj == 0) break; }
        if (obj != 0) {
            VRTransformPtr tr = static_pointer_cast<VRTransform>(obj);
            Matrix4d om = tr->getWorldMatrix();
            om.invert();
            om.mult(m);
            m = om;
        }
    }

    m.mult(dir,dir); dir.normalize();
    Pnt3d p0 = Vec3d(m[3]);

    Line ray;
    ray.setValue(Pnt3f(p0), Vec3f(dir));
    return ray;
}

/** Print the position of the object in local && world coords **/
void VRTransform::printPos() {
    Matrix4d wm, wm_osg, lm;
    getWorldMatrix(wm);
    wm_osg = toMatrix4d(getNode()->node->getToWorld());
    getMatrix(lm);
    cout << "Position of " << getName() << ", local: " << Vec3d(lm[3]) << ", world: " << Vec3d(wm[3]) << "  " << Vec3d(wm_osg[3]);
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
    Matrix4d m = physics->getTransformation();
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
void VRTransform::updateChange() {
    if (checkWorldChange()) apply_constraints();
    if (held) updatePhysics();
    //if (checkWorldChange()) updatePhysics();

    if (!change) return;
    computeMatrix4d();
    updateTransformation();
    updatePhysics();
    change = false;
}

void VRTransform::setup() {
    change = true;
    setAt(Vec3d(_at));
    updateChange();
}

void setFromPath(VRTransformWeakPtr trp, pathPtr p, bool redirect, float t) {
    auto tr = trp.lock();
    if (!tr) return;
    tr->setFrom( p->getPosition(t) );
    if (!redirect) return;

    Vec3d d,u;
    p->getOrientation(t, d, u);
    tr->setDir( d );
    tr->setUp( u );
}

vector<VRAnimCbPtr> animCBs;

void VRTransform::addAnimation(VRAnimationPtr anim) { animations[anim->getName()] = anim; }
vector<VRAnimationPtr> VRTransform::getAnimations() {
    vector<VRAnimationPtr> res;
    for (auto a : animations) res.push_back(a.second);
    return res;
}

VRAnimationPtr VRTransform::startPathAnimation(pathPtr p, float time, float offset, bool redirect, bool loop) {
    pathAnimPtr = VRAnimCb::create("TransAnim", boost::bind(setFromPath, VRTransformWeakPtr(ptr()), p, redirect, _1));
    animCBs.push_back(pathAnimPtr);
    auto a = VRScene::getCurrent()->addAnimation<float>(time, offset, pathAnimPtr, 0.f, 1.f, loop);addAnimation(a);
    return a;
}

void VRTransform::stopAnimation() {
    for (auto a : animations) a.second->stop();
}

list<VRTransformWeakPtr > VRTransform::dynamicObjects = list<VRTransformWeakPtr >();
list<VRTransformWeakPtr > VRTransform::changedObjects = list<VRTransformWeakPtr >();

Matrix4f toMatrix4f(Matrix4d md) {
    Matrix4f mf;
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) mf[i][j] = md[i][j];
    return mf;
}

Matrix4d toMatrix4d(Matrix4f mf) {
    Matrix4d md;
    for (int i=0; i<4; i++) for (int j=0; j<4; j++) md[i][j] = mf[i][j];
    return md;
}

void VRTransform::applyTransformation(PosePtr po) {
    Matrix4d m0 = po->asMatrix();

    map<GeoVectorPropertyRecPtr, bool> applied;

    auto applyMatrix = [&](OSGGeometryPtr mesh, Matrix4d& m) {
        auto pos = mesh->geo->getPositions();
        auto norms = mesh->geo->getNormals();
        Vec3d n; Pnt3d p;
        for (uint i=0; i<pos->size(); i++) {
            p = Pnt3d(pos->getValue<Pnt3f>(i));
            m.mult(p,p);
            pos->setValue(p,i);
        }

        for (uint i=0; i<norms->size(); i++) {
            n = Vec3d(norms->getValue<Vec3f>(i));
            m.mult(n,n);
            norms->setValue(n,i);
        }
    };

    auto computeNewMatrix = [&](VRGeometryPtr geo) {
        auto m = geo->getMatrixTo(ptr());
        auto mI = m; mI.invert();
        m.multLeft(m0);
        m.multLeft(mI);
        return m;
    };

    for (auto obj : getChildren(true, "", true)) {
        auto geo = dynamic_pointer_cast<VRGeometry>(obj);
        if (!geo) continue;
        auto mesh = geo->getMesh();
        if (!mesh) continue;
        if (!mesh->geo) continue;
        auto pos = mesh->geo->getPositions();
        if (!pos) continue;
        if (applied.count(pos)) continue;
        applied[pos] = true;
        auto m = computeNewMatrix(geo);
        applyMatrix(mesh, m);
    }
}

void VRTransform::applyTransformation() {
    applyTransformation(getPose());
    setMatrix(Matrix4d());
}




OSG_END_NAMESPACE;
