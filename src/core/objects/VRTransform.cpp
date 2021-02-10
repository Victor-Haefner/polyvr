#include "VRTransform.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/boundingbox.h"
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/VRUndoInterfaceT.h"
#include "core/objects/OSGTransform.h"
#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRGeometry.h"
#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#endif
#include "core/scene/VRScene.h"
#include "core/scene/VRSpaceWarper.h"

#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGDepthChunk.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

template<> string typeName(const VRTransform& t) { return "Transform"; }


VRTransform::VRTransform(string name, bool doOpt) : VRObject(name) {
    doOptimizations = doOpt; // doOpt; // TODO: this is disabled for developing the sync
    t = OSGTransform::create( Transform::create() );
    constraint = VRConstraint::create();
    constraint->free({0,1,2,3,4,5});
    constraint->setActive(false);
    setCore(OSGCore::create(t->trans), "Transform");
    if (doOptimizations) disableCore();
    addTag("transform");

    store("from", &_from);
    //store("dir", &_dir);
    store("at", &_at);
    store("up", &_up);
    store("scale", &_scale);
    store("at_dir", &orientation_mode);
    storeObj("constraint", constraint);

    regStorageSetupFkt( VRStorageCb::create("transform_update", bind(&VRTransform::setup, this, _1)) );
}

VRTransform::~VRTransform() {
    if (physics) { delete physics; }
}

VRTransformPtr VRTransform::ptr() { return static_pointer_cast<VRTransform>( shared_from_this() ); }
VRTransformPtr VRTransform::create(string name, bool doOpt) { return VRTransformPtr(new VRTransform(name, doOpt) ); }

void VRTransform::wrapOSG(OSGObjectPtr node) {
    if (!node) return;
    VRObject::wrapOSG(node);
    if (!node->node || !node->node->getCore()) return;
    Transform* t = dynamic_cast<Transform*>(node->node->getCore());
    if (t) {
        setMatrix(toMatrix4d(t->getMatrix()));
        this->t->trans = t;
    } else {
        if (getCore()) getCore()->core = this->t->trans;
    }
}

VRObjectPtr VRTransform::copy(vector<VRObjectPtr> children) {
    VRTransformPtr t = VRTransform::create(getBaseName());
    t->setVisible(isVisible());
    t->setEntity(entity);
    t->setMatrix(getMatrix());
    t->setPickable(isPickable());
    t->old_parent = getParent();
    return t;
}

namespace OSG {
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

bool isIdentity(const Matrix4d& m) {
    static Matrix4d r;
    static bool mSet = false;
    if (!mSet) { mSet = true; r.setIdentity(); }
    return (m == r);
}
}

void VRTransform::computeMatrix4d() {
    if (orientation_mode == OM_AT) MatrixLookAt(matrix, _from, _at, _up);
    if (orientation_mode == OM_DIR) MatrixLookDir(matrix, _from, -_dir, _up);

    if (_scale != Vec3d(1,1,1)) {
        Matrix4d ms;
        ms.setScale(_scale);
        matrix.mult(ms);
    }

    auto scene = VRScene::getCurrent();
    if (scene) {
        auto sw = scene->getSpaceWarper();
        if (sw) sw->warp(matrix);
    }
}

void VRTransform::enableOptimization(bool b) {
    doOptimizations = b;
    if (b) updateTransformation();
    else enableCore();
}

void VRTransform::updateTransformation() {
    if (!t->trans) {
        cout << "Error in VRTransform::updateTransformation of " << getName() << "(" << this << "): t->trans is invalid! (" << t->trans << ")" << endl;
        return;
    }

    if (doOptimizations) {
        bool isI = isIdentity(matrix);
        if (identity && !isI) {
            identity = false;
            enableCore();
        }

        if (!identity && isI) {
            identity = true;
            disableCore();
        }
    }

    t->trans->setMatrix(toMatrix4f(matrix));
}

void VRTransform::setIdentity() {
    setMatrix(Matrix4d());
}

void VRTransform::updateChange() {
    apply_constraints();
#ifndef WITHOUT_BULLET
    if (held) updatePhysics();
#endif
    computeMatrix4d();
    updateTransformation();
#ifndef WITHOUT_BULLET
    updatePhysics();
#endif
}

void VRTransform::reg_change() {
    change_time_stamp = VRGlobals::CURRENT_FRAME;
    noBlt = true;
    updateChange();
}

void VRTransform::printInformation() { Matrix4d m; getMatrix(m); cout << " pos " << m[3]; }

unsigned int VRTransform::getLastChange() { return change_time_stamp; }
//bool VRTransform::changedNow() { return (change_time_stamp >= VRGlobals::get()->CURRENT_FRAME-1); }
bool VRTransform::changedNow() { return checkWorldChange(); }

bool VRTransform::changedSince(unsigned int& frame, bool includingFrame) {
    unsigned int f = frame;
    frame = VRGlobals::CURRENT_FRAME;
    int offset = includingFrame?1:0;
    if (change_time_stamp + offset > f) return true;
    if (wchange_time_stamp + offset > f) return true;
    for (auto a : getAncestry()) {
        auto t = dynamic_pointer_cast<VRTransform>(a);
        if (t && t->change_time_stamp + offset > f) return true;
    }
    return false;
}

bool VRTransform::changedSince2(unsigned int frame, bool includingFrame) { // for py binding
    return changedSince(frame, includingFrame);
}

bool VRTransform::checkWorldChange() {
    if (frame == 0) { frame = 1; return true; }
    if (VRGlobals::CURRENT_FRAME == change_time_stamp) return true;
    if (VRGlobals::CURRENT_FRAME == wchange_time_stamp) return true;
    if (hasGraphChanged()) return true;

    for (auto a : getAncestry()) {
        auto t = dynamic_pointer_cast<VRTransform>(a);
        if (t && t->change_time_stamp > wchange_time_stamp) { wchange_time_stamp = VRGlobals::CURRENT_FRAME; return true; }
    }

    return false;
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

    //string shdr_fp =
    //"void main( void ) {"
    //"   gl_Position = gl_ModelViewProjectionMatrix4d*gl_Vertex;"
    //"   gl_Position.z = -0.1;"
    //"}";

    ChunkMaterialMTRecPtr mat = ChunkMaterial::create();
    mat->setSortKey(100);// render last
    SimpleSHLChunkMTRecPtr shader_chunk = SimpleSHLChunk::create();
    shader_chunk->setVertexProgram(shdr_vp.c_str());
    //shader_chunk->setVertexProgram(shdr_fp.c_str());
    mat->addChunk(shader_chunk);

    geo->setMaterial(mat);
}

Vec3d VRTransform::getFrom() { return Vec3d(_from); }
Vec3d VRTransform::getDir() { return Vec3d(_dir); }
Vec3d VRTransform::getAt() { return Vec3d(_at); }
Vec3d VRTransform::getUp() { return Vec3d(_up); }
void VRTransform::getMatrix(Matrix4d& _m) { _m = matrix; }
Matrix4d VRTransform::getMatrix() { return matrix; }

Matrix4d VRTransform::getRotationMatrix() {
    Matrix4d m;
    getMatrix(m);
    m[3][0] = m[3][1] = m[3][2] = 0;
    return m;
}

void VRTransform::setMatrixTo(Matrix4d m, VRObjectPtr obj) {
    VRTransformPtr ent = getParentTransform(obj);
    if (!ent) { setWorldMatrix(m); return; }

    Matrix4d m1 = ent->getWorldMatrix();
    m1.mult(m);
    setWorldMatrix(m1);
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

Vec3d VRTransform::getWorldPosition(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return Vec3d(m[3]);
}

Vec3d VRTransform::getWorldDirection(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return -Vec3d(m[2]);
}

Vec3d VRTransform::getWorldUp(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    return Vec3d(m[1]);
}

Vec3d VRTransform::getWorldAt(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, true);
    Pnt3d a = Pnt3d(getAt());
    m.mult(a,a);
    return Vec3d(a);
}

Vec3d VRTransform::getWorldScale(bool parentOnly) {
    Matrix4d m;
    getWorldMatrix(m, parentOnly);
    Vec3d s = Vec3d(1,1,1), x = Vec3d(1,0,0), y = Vec3d(0,1,0), z = Vec3d(0,0,1);
    m.mult(x,x); x.normalize();
    m.mult(y,y); y.normalize();
    m.mult(z,z); z.normalize();
    m.mult(s,s);
    return Vec3d(s.dot(x), s.dot(y), s.dot(z));
}


void VRTransform::updateTransform(VRTransformPtr t) {
    if (!t) return;
    if (t->getLastChange() < getLastChange()) return;
    setMatrix(t->getMatrix()); // TODO: may need world matrix here
}

void VRTransform::setWorldMatrix(Matrix4d m) {
    if (isNan(m)) return;
    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    setMatrix(wm);
}

VRTransformPtr VRTransform::getParentTransform(VRObjectPtr o) {
    if (!o) return 0;
    o = o->hasAncestorWithTag("transform");
    return static_pointer_cast<VRTransform>(o);
}

OSGTransformPtr VRTransform::getOSGTransformPtr(){
    return t;
}

void VRTransform::setRelativePose(PosePtr p, VRObjectPtr o) {
    Matrix4d m = p->asMatrix();
    Matrix4d wm = getMatrixTo(o);
    //wm.invert();
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

void VRTransform::setWorldPosition(Vec3d pos) {
    if (isNan(pos)) return;

    Matrix4d m;
    m.setTranslate(pos);

    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    wm.mult(m);
    setFrom( Vec3d(wm[3]) );
}

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

void VRTransform::setWorldAt(Vec3d at) {
    Matrix4d wm = getWorldMatrix(true);
    wm.invert();
    Pnt3d a = Pnt3d(at);
    wm.mult(a,a);
    setAt(Vec3d(a));
}

void VRTransform::setWorldScale(Vec3d s) {
    Vec3d sP = getWorldScale(true);
    for (int i=0; i<3; i++) s[i] = s[i]/sP[i];
    setScale(s);
}

//local pose setter--------------------
void VRTransform::setFrom(Vec3d pos) {
    if (isNan(pos)) return;
    _from = pos;
    if (orientation_mode == OM_DIR) _at = _from + _dir;
    if (orientation_mode == OM_AT) _dir = _at - _from;
    reg_change();
}

void VRTransform::setAt(Vec3d at) {
    if (isNan(at)) return;
    _at = at;
    _dir = _at - _from;
    orientation_mode = OM_AT;
    reg_change();
}

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

void VRTransform::setOrientation(Vec3d dir, Vec3d up) {
    if (isNan(dir) || isNan(up)) return;
    _up = up;
    setDir(dir);
}

void VRTransform::setOrientationQuat(Quaterniond q) {
    //if (isNan(q)) return;
    Matrix4d m;
    m.setRotate(q);
    m.setTranslate(getFrom());
    setMatrix(m);
}

void VRTransform::setOrientationQuat(Vec4d vecq) {
    setOrientationQuat(Quaterniond(vecq[0],vecq[1],vecq[2],vecq[3]));
}

void VRTransform::setTransform(Vec3d from, Vec3d dir, Vec3d up) {
    if (isNan(from) || isNan(dir) || isNan(up)) return;
    if (from == _from && dir == _dir && up == _up) return;
    _from = from;
    _up = up;
    setDir(dir);
}

void VRTransform::setPose(PosePtr p) { if (p) setPose2(*p); }
void VRTransform::setPose2(Pose& p) { setTransform(p.pos(), p.dir(), p.up()); setScale(p.scale()); }
PosePtr VRTransform::getPose() { return Pose::create(_from, _dir, _up, _scale); }
PosePtr VRTransform::getWorldPose() { return Pose::create( getWorldMatrix() ); }
void VRTransform::setWorldPose(PosePtr p) { setWorldMatrix(p->asMatrix()); }

void VRTransform::setMatrix(Matrix4d m) {
    if (isNan(m)) return;

    //float s1 = Vec3d(_m[0][0], _m[1][0], _m[2][0]).length();
    //float s2 = Vec3d(_m[0][1], _m[1][1], _m[2][1]).length();
    //float s3 = Vec3d(_m[0][2], _m[1][2], _m[2][2]).length();

    float s1 = m[0].length(); //TODO: check if this is fine
    float s2 = m[1].length();
    float s3 = m[2].length();

    setTransform(Vec3d(m[3]), Vec3d(-m[2])*1.0/s3, Vec3d(m[1])*1.0/s2);
    setScale(Vec3d(s1,s2,s3));
}

//-------------------------------------

void VRTransform::initCoords() {
    if (coords != 0) return;
    coords = OSGObject::create( makeCoordAxis(0.3, 3, false) );
    coords->node->setTravMask(0);
    addChild(coords);
    GeometryMTRecPtr geo = dynamic_cast<Geometry*>(coords->node->getCore());
    ChunkMaterialMTRecPtr mat = ChunkMaterial::create();
    DepthChunkMTRecPtr depthChunk = DepthChunk::create();
    depthChunk->setFunc( GL_ALWAYS );
    mat->addChunk(depthChunk);
    mat->setSortKey(100);// render last
    geo->setMaterial(mat);
}

void VRTransform::showCoordAxis(bool b) {
    initCoords();
    if (b) {
        coords->node->setTravMask(0xffffffff);
        Vec3d scale = getWorldScale();
        for (int j=0; j<3; j++) scale[j] = 1.0/scale[j];
        GeometryMTRecPtr geo = dynamic_cast<Geometry*>(coords->node->getCore());
        GeoPnt3fPropertyMTRecPtr pos = (GeoPnt3fProperty*)geo->getPositions();
        for (int i : {0,1,2}) {
            Pnt3f p;
            p[i] = 0.3*scale[i];
            pos->setValue(p,i*2+1);
        }
    }
    else coords->node->setTravMask(0);
}

void VRTransform::setScale(float s) { setScale(Vec3d(s,s,s)); }

void VRTransform::setScale(Vec3d s) {
    if (isNan(s)) return;
    _scale = s;
    reg_change();
}

void VRTransform::setEuler(Vec3d e) {
    if (isNan(e)) return;
    _euler = e;
    auto m = getMatrix();
    applyEulerAngles(m, e);
    setMatrix(m);
}

void VRTransform::setEulerDegree(Vec3d e) {
    e *= Pi/180.0;
    setEuler(e);
}

Vec3d VRTransform::getScale() { return _scale; }
Vec3d VRTransform::getEuler() {
    //return _euler;
    return computeEulerAngles( getMatrix() );
}

Vec3d VRTransform::computeEulerAngles(const Matrix4d& m) {
    Vec3d a;
    a[0] = atan2( m[1][2], m[2][2]);
    a[1] = atan2(-m[0][2], sqrt(m[1][2]*m[1][2] + m[2][2]*m[2][2]));
    a[2] = atan2( m[0][1], m[0][0]);
    return a;
}

void VRTransform::applyEulerAngles(Matrix4d& t, Vec3d e) {
    Pnt3d p = Pnt3d(t[3]); // copy position
    Vec3d s = Vec3d(sin(e[0]), sin(e[1]), sin(e[2]));
    Vec3d c = Vec3d(cos(e[0]), cos(e[1]), cos(e[2]));
    Vec3d d = Vec3d( c[0]*c[2]*s[1]+s[0]*s[2], c[0]*s[1]*s[2]-s[0]*c[2], c[0]*c[1]);
    Vec3d u = Vec3d( s[0]*s[1]*c[2]-s[2]*c[0], s[0]*s[1]*s[2]+c[2]*c[0], c[1]*s[0]);
    MatrixLookDir(t, p, d, u);
}

void VRTransform::rotateWorld(float a, Vec3d v, Vec3d o) {
    auto pW = getWorldMatrix();
    auto posW = Vec3d(pW[3]);

    Matrix4d R;
    R.setRotate( Quaterniond(Vec3d(v), a) );
    R.setTranslate(o + posW);

    pW.setTranslate(-o);
    pW.multLeft(R);
    setWorldMatrix(pW);
}

void VRTransform::rotate(float a, Vec3d v, Vec3d o) {//rotate around axis
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

void VRTransform::rotateAround(float a, Vec3d v) {//rotate around focus using up axis
    if (isNan(a)) return;
    orientation_mode = OM_AT;
    Quaterniond q(v, -a);
    q.multVec(_dir,_dir);
    _from = _at - _dir;
    reg_change();
}

void VRTransform::rotateYonZ() {
    rotate(Pi*0.5, Vec3d(1,0,0));
}

void VRTransform::translate(Vec3d v) {
    if (isNan(v)) return;
    _at += Vec3d(v);
    _from += Vec3d(v);
    reg_change();
}

void VRTransform::zoom(float d) {
    if (isNan(d)) return;
    setFrom(_from + _dir*d);
    reg_change();
}

void VRTransform::move(float d) {
    if (isNan(d)) return;
    Vec3d dv = _dir;
    dv.normalize();
    translate(Vec3d(dv*d));
}

void VRTransform::drag(VRTransformPtr new_parent, VRIntersection i) {
    if (held) return;
    held = true;
    if (auto p = getParent()) old_parent = p;
    old_child_id = getChildIndex();

    //showTranslator(true); //TODO

    Matrix4d m;
    old_transformation = getMatrix();
    getWorldMatrix(m);
    switchParent(new_parent);
    setWorldMatrix(m);

#ifndef WITHOUT_BULLET
    if (physics && physics->isDynamic()) {
        physics->updateTransformation( ptr() );
        physics->resetForces();
        //physics->pause(true);
        //physics->setGravity(Vec3d(0,0,0));
        if (!new_parent->physics) new_parent->physicalize(1,0,"Box",0.01);
        auto c = VRConstraint::create();
        c->free({0,1,2,3,4,5});
        auto cs = VRConstraint::create();
        for (int i=0; i<3; i++) {
            cs->setMinMax(i,1000,0.01); // stiffness, dampness
            cs->setMinMax(i+3,-1,0);
        }

        Pnt3d P = i.point; // intersection point in world coords
        m.invert();
        m.mult(P, P);

        c->setReferenceA(Pose::create(Vec3d(P)));
        c->setReferenceB(Pose::create(getFrom()));
        physics->setConstraint(new_parent->physics, c, cs);
    }
#endif

    reg_change();
    updateChange();
}

void VRTransform::drop() {
    if (!held) return;
    held = false;

    Matrix4d wm, m1, m2;
    getWorldMatrix(wm);
    m1 = getMatrix();
    auto dragParent = dynamic_pointer_cast<VRTransform>( getParent() );
    if (auto p = old_parent.lock()) switchParent(p, old_child_id);
    setWorldMatrix(wm);
    recUndo(&VRTransform::setMatrix, ptr(), old_transformation, getMatrix());

#ifndef WITHOUT_BULLET
    if (physics) {
        physics->updateTransformation( ptr() );
        physics->resetForces();
        //physics->pause(false);
        //physics->setGravity(Vec3d(0,-10,0));
        if (dragParent && dragParent->physics) physics->deleteConstraints(dragParent->physics);
    }
#endif

    reg_change();
    updateChange();
}

void VRTransform::rebaseDrag(VRObjectPtr new_parent) {
    if (new_parent == 0 || new_parent == ptr()) return;
    //cout << "VRTransform::rebaseDrag " << new_parent->getName() << " " << getName() << endl;
    if (!held) { switchParent(new_parent); return; }
    if (new_parent->hasAncestor(ptr())) return;
    old_parent = new_parent;
    old_child_id = 0;
}

bool VRTransform::isDragged() { return held; }
VRObjectPtr VRTransform::getDragParent() { return old_parent.lock(); }

// Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults
Line VRTransform::castRay(VRObjectPtr obj, Vec3d dir) { // TODO: check what this is doing exactly and simplify!
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

VRIntersection VRTransform::intersect(VRObjectPtr obj, Vec3d dir) {
    VRIntersect in;
    return in.intersect(obj, false, ptr(), dir);
}

void VRTransform::printPos() {
    Matrix4d wm, wm_osg, lm;
    getWorldMatrix(wm);
    wm_osg = toMatrix4d(getNode()->node->getToWorld());
    getMatrix(lm);
    cout << "Position of " << getName() << ", local: " << Vec3d(lm[3]) << ", world: " << Vec3d(wm[3]) << "  " << Vec3d(wm_osg[3]);
}

void VRTransform::printTransformationTree(int indent) {
    if (indent == 0) cout << "\nPrint Transformation Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    if (getType() == "Transform" || getType() == "Geometry") {
        printPos();
    }

    for (unsigned int i=0;i<getChildrenCount();i++) {
        if (getChild(i)->getType() == "Transform" || getChild(i)->getType() == "Geometry") {
            VRTransformPtr tmp = static_pointer_cast<VRTransform>( getChild(i) );
            tmp->printTransformationTree(indent+1);
        }
    }

    if (indent == 0) cout << "\n";
}

map<VRTransform*, VRTransformWeakPtr> constrainedObjects;

void VRTransform::updateConstraints() { // global updater
    for (auto wc : constrainedObjects) {
        if (VRTransformPtr obj = wc.second.lock()) obj->updateChange();
    }
}

void VRTransform::setConstraint(VRConstraintPtr c) {
    constraint = c;
    if (c) constrainedObjects[this] = ptr();
    else constrainedObjects.erase(this);
}

VRConstraintPtr VRTransform::getConstraint() { setConstraint(constraint); return constraint; }

void VRTransform::apply_constraints(bool force) { // TODO: check efficiency
    if (!constraint && aJoints.size() == 0) return;
    if (!checkWorldChange() && !force) return;
    computeMatrix4d(); // update matrix!

    if (constraint) constraint->apply(ptr(), 0, force);
    for (auto joint : aJoints) {
        VRTransformPtr parent = joint.second.second.lock();
        if (parent) joint.second.first->apply(ptr(), parent, force);
        //if (parent) cout << "VRTransform::apply_constraints to " << getName() << " with parent: " << parent->getName() << endl;
    }

    for (auto joint : bJoints) {
        VRTransformPtr child = joint.second.second.lock();
        //if (child) child->apply_constraints(true); // TODO: may introduce loops?
    }
}

void VRTransform::setup(VRStorageContextPtr context) {
    setAt(_at);
}

void setFromPath(VRTransformWeakPtr trp, PathPtr p, PathPtr po, bool redirect, float t) {
    auto tr = trp.lock();
    if (!tr) return;
    if (p) tr->setFrom( p->getPosition(t) );
    if (!redirect && !po) return;

    if (po) p = po;
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

VRAnimationPtr VRTransform::animate(PathPtr p, float time, float offset, bool redirect, bool loop, PathPtr po) {
    pathAnimPtr = VRAnimCb::create("TransAnim", bind(setFromPath, VRTransformWeakPtr(ptr()), p, po, redirect, _1));
    animCBs.push_back(pathAnimPtr);
    auto a = VRScene::getCurrent()->addAnimation<float>(time, offset, pathAnimPtr, 0.f, 1.f, loop);addAnimation(a);
    return a;
}

void VRTransform::stopAnimation() {
    for (auto a : animations) a.second->stop();
}

list<VRTransformWeakPtr > VRTransform::dynamicObjects = list<VRTransformWeakPtr >();
list<VRTransformWeakPtr > VRTransform::changedObjects = list<VRTransformWeakPtr >();

namespace OSG {
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
}

void VRTransform::applyTransformation(PosePtr po) {
    if (!po) po = getPose();
    Matrix4d m0 = po->asMatrix();
    map<GeoVectorPropertyMTRecPtr, bool> applied;
    map<VRTransform*, Matrix4d> matrices;

    auto applyMatrix = [&](OSGGeometryPtr mesh, Matrix4d& m) {
        auto pos = mesh->geo->getPositions();
        auto norms = mesh->geo->getNormals();
        Vec3d n; Pnt3d p;
        for (unsigned int i=0; i<pos->size(); i++) {
            p = Pnt3d(pos->getValue<Pnt3f>(i));
            m.mult(p,p);
            pos->setValue(p,i);
        }

        for (unsigned int i=0; i<norms->size(); i++) {
            n = Vec3d(norms->getValue<Vec3f>(i));
            m.mult(n,n);
            norms->setValue(n,i);
        }
    };

    auto computeNewMatrix = [&](VRGeometryPtr geo) {
        auto m = getMatrixTo(geo);
        auto mI = m; mI.invert();
        m.multLeft(m0);
        m.multLeft(mI);
        return m;
    };

    auto objects = getChildren(true, "", true);
    //for (auto obj : objects)
    //    if (auto trans = dynamic_pointer_cast<VRTransform>(obj)) matrices[trans.get()] = trans->getWorldMatrix();

    for (auto obj : objects) {
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

    setIdentity();
    //for (auto obj : objects)
    //    if (auto trans = dynamic_pointer_cast<VRTransform>(obj)) trans->setWorldMatrix(matrices[trans.get()]);
}

void VRTransform::attach(VRTransformPtr b, VRConstraintPtr c, VRConstraintPtr cs) {
    VRTransformPtr a = ptr();
    if (!c) { constrainedObjects.erase(b.get()); return; }
    constrainedObjects[b.get()] = b;
    a->bJoints[b.get()] = make_pair(c, VRTransformWeakPtr(b)); // children
    b->aJoints[a.get()] = make_pair(c, VRTransformWeakPtr(a)); // parents
    c->setActive(true);
#ifndef WITHOUT_BULLET
    if (auto p = getPhysics()) p->setConstraint( b->getPhysics(), c, cs );
    //cout << "VRTransform::attach " << b->getName() << " to " << a->getName() << endl;
#endif
}

void VRTransform::detachJoint(VRTransformPtr b) { // TODO, remove joints
#ifndef WITHOUT_BULLET
    if (auto p = getPhysics()) p->deleteConstraints(b->getPhysics());
#endif
}

void VRTransform::setSpringParameters(VRTransformPtr b, int dof, float stiffnes, float damping) {
#ifndef WITHOUT_BULLET
    if (auto p = getPhysics()) p->setSpringParameters(b->getPhysics(), dof, stiffnes, damping);
#endif
}

Vec3d VRTransform::getConstraintAngleWith(VRTransformPtr t, bool rotationOrPosition) {
    Vec3d a;
#ifndef WITHOUT_BULLET
    if (auto p = getPhysics()) {
        auto p2 = t->getPhysics();
        if (!rotationOrPosition) {
            a[0] = p->getConstraintAngle(p2,0);
            a[1] = p->getConstraintAngle(p2,1);
            a[2] = p->getConstraintAngle(p2,2);
        } else {
            a[0] = p->getConstraintAngle(p2,3);
            a[1] = p->getConstraintAngle(p2,4);
            a[2] = p->getConstraintAngle(p2,5);
        }
    }
#endif
    return a;
}

#ifndef WITHOUT_BULLET
VRPhysics* VRTransform::getPhysics() {
    if (physics == 0) physics = new VRPhysics( ptr() );
    return physics;
}

vector<VRCollision> VRTransform::getCollisions() {
    if (physics == 0) return vector<VRCollision>();
    return physics->getCollisions();
}

void VRTransform::updatePhysics() { //should be called from the main thread only
    if (physics == 0) return;
    //if (physics->isPhysicalized()) cout << getName() << "  VRTransform::updatePhysics from SG " << bltOverride << endl;
    if (noBlt && !held && !bltOverride) { noBlt = false; return; }
    if (!physics->isPhysicalized()) return;

    physics->updateTransformation( ptr() );
    physics->resetForces();
    bltOverride = false;
}


void VRTransform::updateFromBullet() {
    //cout << getName() << "  VRTransform::updateFromBullet!" << endl;
    Matrix4d m = physics->getTransformation();
    setWorldMatrix(m);
    auto vs = physics->getVisualShape();
    if (vs && vs->isVisible()) vs->setWorldMatrix(m);
    setNoBltFlag();
}

void VRTransform::resolvePhysics() {
    if (!physics) return;
    if (physics->isGhost()) { updatePhysics(); return; }
    if (physics->isDynamic() && !bltOverride) { updateFromBullet(); return; }
    physics->updateTransformation( ptr() );
}

void VRTransform::physicalize(bool b, bool dynamic, string shape, float param) {
    if (auto p = getPhysics()) {
        p->setDynamic(dynamic);
        p->setShape(shape, param);
        p->setPhysicalized(b);
    }
}

void VRTransform::setCollisionGroup(vector<int> gv) {
    int g = 0;
    for (auto gi : gv) g = g | int( pow(2,gi) );
    if (auto p = getPhysics()) p->setCollisionGroup(g);
}

void VRTransform::setCollisionMask(vector<int> gv) {
    int g = 0;
    for (auto gi : gv) g = g | int( pow(2,gi) );
    if (auto p = getPhysics()) p->setCollisionMask(g);
}

void VRTransform::setConvexDecompositionParameters(float cw, float vw, float nc, float nv, float c, bool aedp, bool andp, bool afp) {
    getPhysics()->setConvexDecompositionParameters(cw, vw, nc, nv, c, aedp, andp, afp);
}

bool VRTransform::getPhysicsDynamic() { return getPhysics()->isDynamic(); }
void VRTransform::setPhysicsDynamic(bool b) { getPhysics()->setDynamic(b,true); }
void VRTransform::setNoBltFlag() { noBlt = true; }
void VRTransform::setBltOverrideFlag() { bltOverride = true; }
void VRTransform::setPhysicalizeTree(bool b) { if (auto p = getPhysics()) p->physicalizeTree(b); }
void VRTransform::setMass(float m) { if (auto p = getPhysics()) p->setMass(m); }
void VRTransform::setCollisionMargin(float m) { if (auto p = getPhysics()) p->setCollisionMargin(m); }
void VRTransform::setCollisionShape(string s, float f) { if (auto p = getPhysics()) p->setShape(s, f); }
void VRTransform::setPhysicsActivationMode(int m) { if (auto p = getPhysics()) p->setActivationMode(m); }
void VRTransform::applyImpulse(Vec3d i) { if (auto p = getPhysics()) p->applyImpulse(i); }
void VRTransform::applyTorqueImpulse(Vec3d i) { if (auto p = getPhysics()) p->applyTorqueImpulse(i); }
void VRTransform::applyForce(Vec3d f) { if (auto p = getPhysics()) p->addForce(f); }
void VRTransform::applyConstantForce(Vec3d f) { if (auto p = getPhysics()) p->addConstantForce(f); }
void VRTransform::applyTorque(Vec3d f) { if (auto p = getPhysics()) p->addTorque(f); }
void VRTransform::applyConstantTorque(Vec3d f) { if (auto p = getPhysics()) p->addConstantTorque(f); }
void VRTransform::setGravity(Vec3d g) { if (auto p = getPhysics()) p->setGravity(g); }
void VRTransform::setCenterOfMass(Vec3d g) { if (auto p = getPhysics()) p->setCenterOfMass(g); }
void VRTransform::setGhost(bool g) { if (auto p = getPhysics()) p->setGhost(g); }
void VRTransform::setDamping(float ld, float ad) { if (auto p = getPhysics()) p->setDamping(ld, ad); }

Vec3d VRTransform::getForce() { if (auto p = getPhysics()) return p->getForce(); else return Vec3d(); }
Vec3d VRTransform::getTorque() { if (auto p = getPhysics()) return p->getTorque(); else return Vec3d(); }
Vec3d VRTransform::getCenterOfMass() { if (auto p = getPhysics()) return p->getCenterOfMass(); else return Vec3d(); }
#endif

