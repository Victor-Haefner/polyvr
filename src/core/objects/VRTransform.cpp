
#include "core/utils/isNan.h"
#include "core/utils/toString.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/VRDoublebuffer.h"
#include "core/scene/VRAnimationManagerT.h"
#include "VRTransform.h"
#include "geometry/VRPhysics.h"
#include "core/math/path.h"
#include <OpenSG/OSGTransform.h>
#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGChunkMaterial.h>
#include <OpenSG/OSGMatrixUtility.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include <libxml++/nodes/element.h>


OSG_BEGIN_NAMESPACE;
using namespace std;

VRObject* VRTransform::copy(vector<VRObject*> children) {
    VRTransform* geo = new VRTransform(getBaseName());
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
    if (noBlt && !held) { noBlt = false; return; }
    if (!physics->isPhysicalized()) return;

    /*Matrix m;
    dm->read(m);
    Matrix pm;
    getWorldMatrix(pm, true);
    pm.mult(m);*/

    physics->updateTransformation(this);
    physics->pause();
    physics->resetForces();
}

void VRTransform::updateTransformation() {
    Matrix m;
    dm->read(m);
    t->setMatrix(m);
}

void VRTransform::reg_change() {
    if (change == false) {
        if (fixed) changedObjects.push_back(this);
        change = true;
        change_time_stamp = VRGlobals::get()->CURRENT_FRAME;
    }
}

void VRTransform::printInformation() { Matrix m; getMatrix(m); cout << " pos " << m[3]; }

/** initialise a point 3D object with his name **/
VRTransform::VRTransform(string name) : VRObject(name) {
    dm = new doubleBuffer;
    t = Transform::create();
    setCore(t, "Transform");

    _from = Vec3f(0,0,0);
    _at = Vec3f(0,0,-1); //equivalent to identity matrix!
    _up = Vec3f(0,1,0);
    _scale = Vec3f(1,1,1);

    change = false;
    held = false;
    fixed = true;
    cam_invert_z = false;
    frame = 0;

    physics = new VRPhysics(this);
    noBlt = false;
    doTConstraint = false;
    doRConstraint = false;
    tConPlane = true;
    tConstraint = Vec3f(0,1,0);
    rConstraint = Vec3i(0,0,0);
    addAttachment("transform", 0);

    coords = 0;
    translator = 0;
}

VRTransform::~VRTransform() {
    dynamicObjects.remove(this);
    changedObjects.remove(this);
    delete physics;
    delete dm;
}

uint VRTransform::getLastChange() { return change_time_stamp; }
//bool VRTransform::changedNow() { return (change_time_stamp >= VRGlobals::get()->CURRENT_FRAME-1); }
bool VRTransform::changedNow() { return checkWorldChange(); }

void VRTransform::initCoords() {
    if (coords != 0) return;

    coords = makeCoordAxis(0.3, 3, false);
    coords->setTravMask(0);
    addChild(coords);
    GeometryRecPtr geo = dynamic_cast<Geometry*>(coords->getCore());

    string shdr_vp =
    "void main( void ) {"
    "   gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;"
    "   gl_Position.z = -0.1;"
    "   gl_FrontColor = gl_Color;"
    "}";

    ChunkMaterialRecPtr mat = ChunkMaterial::create();
    mat->setSortKey(100);// render last
    SimpleSHLChunkRecPtr shader_chunk = SimpleSHLChunk::create();
    shader_chunk->setVertexProgram(shdr_vp.c_str());
    mat->addChunk(shader_chunk);

    geo->setMaterial(mat);
}

void VRTransform::initTranslator() { // TODO
    if (translator != 0) return;

    translator = makeCoordAxis(0.3, 3, false);
    translator->setTravMask(0);
    addChild(translator);
    GeometryRecPtr geo = dynamic_cast<Geometry*>(translator->getCore());

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

bool VRTransform::checkWorldChange() {
    if (frame == 0) {
        frame = 1;
        return true;
    }

    if (hasGraphChanged()) return true;


    VRObject* obj = this;
    VRTransform* ent;
    while(obj) {
        if (obj->hasAttachment("transform")) {
            ent = (VRTransform*)obj;
            if (ent->change_time_stamp > wchange_time_stamp) {
                wchange_time_stamp = ent->change_time_stamp;
                return true;
            }
        }
        obj = obj->getParent();
    }

    return false;
}

/** Returns the world matrix **/
void VRTransform::getWorldMatrix(Matrix& M, bool parentOnly) {
    VRTransform* t = 0;
    M.setIdentity();

    Matrix m;
    VRObject* o = this;
    if (parentOnly && o->getParent() != 0) o = o->getParent();

    while(o) {
        if (o->hasAttachment("transform")) {
            t = (VRTransform*)o;
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

/** Set the object fixed || not **/
void VRTransform::setFixed(bool b) {
    if (b == fixed) return;
    fixed = b;

    if(b) { dynamicObjects.remove(this); return; }

    bool inDO = (std::find(dynamicObjects.begin(), dynamicObjects.end(),this) != dynamicObjects.end());
    if(!inDO) dynamicObjects.push_back(this);
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

/** Set the local matrix **/
void VRTransform::setMatrix(Matrix _m) {
    if (isNan(_m)) return;

    /*float s1 = Vec3f(_m[0][0], _m[1][0], _m[2][0]).length();
    float s2 = Vec3f(_m[0][1], _m[1][1], _m[2][1]).length();
    float s3 = Vec3f(_m[0][2], _m[1][2], _m[2][2]).length();*/

    float s1 = _m[0].length(); //TODO: check if this is fine
    float s2 = _m[1].length();
    float s3 = _m[2].length();

    setPose(Vec3f(_m[3]), Vec3f(-_m[2])*1.0/s3, Vec3f(_m[1])*1.0/s2);
    setScale(Vec3f(s1,s2,s3));
}
//-------------------------------------

void VRTransform::showCoordAxis(bool b) {
    initCoords();
    if (b) coords->setTravMask(0xffffffff);
    else coords->setTravMask(0);
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
    //orientation_mode = OM_EULER;

    //Vec3f d = Vec3f( -c[1]*c[2], -c[1]*s[2], s[1]);
    Vec3f d = Vec3f( -c[0]*c[2]*s[1]-s[0]*s[2], -c[0]*s[1]*s[2]+s[0]*c[2], -c[0]*c[1]);
    Vec3f u = Vec3f( s[0]*s[1]*c[2]-s[2]*c[0], s[0]*s[1]*s[2]+c[2]*c[0], c[1]*s[0]);

    setDir( d );
    setUp( u );
    reg_change();
}

Vec3f VRTransform::getScale() { return _scale; }
Vec3f VRTransform::getEuler() { return _euler; }

/** Rotate the object around its up axis **/
void VRTransform::rotate(float a) {//rotate around up axis
    if (isNan(a)) return;
    Vec3f d = _at - _from;

    Quaternion q = Quaternion(_up, a);
    q.multVec(d,d);

    _at = _from + d;

    reg_change();
    //cout << "\nRotating " << name << " " << a ;
}

void VRTransform::rotate(float a, Vec3f v) {//rotate around up axis
    if (isNan(a) || isNan(v)) return;
    Vec3f d = _at - _from;

    v.normalize();
    Quaternion q = Quaternion(v, a);
    q.multVec(d,d);
    q.multVec(_up,_up);

    _at = _from + d;

    reg_change();
}

        /** Rotate the object around its dir axis **/
void VRTransform::rotateUp(float a) {//rotate around _at axis
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
void VRTransform::drag(VRTransform* new_parent) {
    if (held) return;
    held = true;
    old_parent = getParent();
    old_child_id = getChildIndex();
    setFixed(false);

    //showTranslator(true); //TODO

    Matrix m;
    getWorldMatrix(m);
    switchParent(new_parent);
    setWorldMatrix(m);

    physics->updateTransformation(this);
    physics->resetForces();
    physics->pause(true);
    reg_change();
    update();
}

/** Drop the object, this returns the object at its old place in hirarchy **/
void VRTransform::drop() {
    if (!held) return;
    held = false;
    setFixed(true);

    Matrix m;
    getWorldMatrix(m);
    switchParent(old_parent, old_child_id);
    setWorldMatrix(m);

    physics->updateTransformation(this);
    physics->resetForces();
    physics->pause(false);
    reg_change();
    update();
}

void VRTransform::rebaseDrag(VRObject* new_parent) {
    if (!held) { switchParent(new_parent); return; }
    old_parent = new_parent;
}

VRObject* VRTransform::getDragParent() { return old_parent; }

/** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
Line VRTransform::castRay(VRObject* obj, Vec3f dir) {
    Matrix m = getWorldMatrix();
    if (obj) obj = obj->getParent();

    if (obj != 0) {
        while (!obj->hasAttachment("transform")) { obj = obj->getParent(); if(obj == 0) break; }
        if (obj != 0) {
            VRTransform* tr = (VRTransform*)obj;
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
    wm_osg = getNode()->getToWorld();
    getMatrix(lm);
    cout << "Position of " << getName() << ", local: " << Vec3f(lm[3]) << ", world: " << Vec3f(wm[3]) << "  " << Vec3f(wm_osg[3]);
}

/** Print the positions of all the subtree **/
void VRTransform::printTransformationTree(int indent) {
    if(indent == 0) cout << "\nPrint Transformation Tree : ";

    cout << "\n";
    for (int i=0;i<indent;i++) cout << "  ";
    if (getType() == "Transform" || getType() == "Geometry") {
        VRTransform* _this = (VRTransform*) this;
        _this->printPos();
    }

    for (uint i=0;i<getChildrenCount();i++) {
        if (getChild(i)->getType() == "Transform" || getChild(i)->getType() == "Geometry") {
            VRTransform* tmp = (VRTransform*) getChild(i);
            tmp->printTransformationTree(indent+1);
        }
    }

    if(indent == 0) cout << "\n";
}

/** enable constraints on the object, 0 leaves the DOF free, 1 restricts it **/
void VRTransform::apply_constraints() {
    if (!doTConstraint && !doRConstraint) return;
    auto now = VRGlobals::get()->CURRENT_FRAME;
    if (apply_time_stamp == now) return;
    apply_time_stamp = now;

    Matrix t = getWorldMatrix();

    //Matrix pt = getWorldMatrix(false); // parent world matrix
    //Matrix t; dm->read(t); t.multLeft(pt); // own world matrix

    //rotation
    if (doRConstraint) {
        int qs = rConstraint[0]+rConstraint[1]+rConstraint[2];
        Matrix t0 = constraints_reference;

        if (qs == 3) for (int i=0;i<3;i++) t[i] = t0[i];

        if (qs == 2) {
            int u,v,w;
            if (rConstraint[0] == 0) { u = 0; v = 1; w = 2; }
            if (rConstraint[1] == 0) { u = 1; v = 0; w = 2; }
            if (rConstraint[2] == 0) { u = 2; v = 0; w = 1; }

            for (int i=0;i<3;i++) t[i][u] = t0[i][u]; //copy old transformation

            //normiere so das die b komponennte konstant bleibt
            for (int i=0;i<3;i++) {
                float a = 1-t[i][u]*t[i][u];
                if (a < 1e-6) {
                    t[i][v] = t0[i][v];
                    t[i][w] = t0[i][w];
                } else {
                    a /= (t0[i][v]*t0[i][v] + t0[i][w]*t0[i][w]);
                    a = sqrt(a);
                    t[i][v] *= a;
                    t[i][w] *= a;
                }
            }
        }

        if (qs == 1) {
            /*int u,v,w;
            if (rConstraint[0] == 1) { u = 0; v = 1; w = 2; }
            if (rConstraint[1] == 1) { u = 1; v = 0; w = 2; }
            if (rConstraint[2] == 1) { u = 2; v = 0; w = 1; }*/

            // TODO
        }
    }

    //translation
    if (doTConstraint) {
        if (tConPlane) {
            float d = Vec3f(t[3] - constraints_reference[3]).dot(tConstraint);
            for (int i=0; i<3; i++) t[3][i] -= d*tConstraint[i];
        } else {
            Vec3f d = Vec3f(t[3] - constraints_reference[3]);
            d = d.dot(tConstraint)*tConstraint;
            for (int i=0; i<3; i++) t[3][i] = constraints_reference[3][i] + d[i];
        }
    }

    //pt.invert();
    //t.multLeft(pt);
    //setMatrix(t);

    setWorldMatrix(t);
}

//void VRTransform::setBulletObject(btRigidBody* b) { physics->setObj(b); }
//btRigidBody* VRTransform::getBulletObject() { return physics->obj(); }

void VRTransform::updateFromBullet() {
    if (held) return;
    Matrix m = physics->getTransformation();
    setWorldMatrix(m);
    setNoBltFlag();
}

void VRTransform::setNoBltFlag() { noBlt = true; }

void VRTransform::setRestrictionReference(Matrix m) { constraints_reference = m; }
void VRTransform::toggleTConstraint(bool b) { doTConstraint = b; if (b) getWorldMatrix(constraints_reference); if(!doRConstraint) setFixed(!b); }
void VRTransform::toggleRConstraint(bool b) { doRConstraint = b; if (b) getWorldMatrix(constraints_reference); if(!doTConstraint) setFixed(!b); }
void VRTransform::setTConstraint(Vec3f trans) { tConstraint = trans; if (tConstraint.length() > 1e-4) tConstraint.normalize(); }
void VRTransform::setTConstraintMode(bool plane) { tConPlane = plane; }
void VRTransform::setRConstraint(Vec3i rot) { rConstraint = rot; }

bool VRTransform::getTConstraintMode() { return tConPlane; }
Vec3f VRTransform::getTConstraint() { return tConstraint; }
Vec3i VRTransform::getRConstraint() { return rConstraint; }

bool VRTransform::hasTConstraint() { return doTConstraint; }
bool VRTransform::hasRConstraint() { return doRConstraint; }

VRPhysics* VRTransform::getPhysics() { return physics; }

/** Update the object OSG transformation **/
void VRTransform::update() {
    apply_constraints();

    if (held) updatePhysics();
    //if (checkWorldChange()) updatePhysics();

    if (!change) return;
    computeMatrix();
    updateTransformation();
    updatePhysics();
    change = false;
}

void VRTransform::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);

    e->set_attribute("from", toString(_from).c_str());
    e->set_attribute("at", toString(_at).c_str());
    e->set_attribute("up", toString(_up).c_str());
    e->set_attribute("scale", toString(_scale).c_str());

    e->set_attribute("cT", toString(tConstraint).c_str());
    e->set_attribute("cR", toString(rConstraint).c_str());
    e->set_attribute("do_cT", toString(doTConstraint).c_str());
    e->set_attribute("do_cR", toString(doRConstraint).c_str());
    e->set_attribute("cT_mode", toString(int(tConPlane)).c_str());

    e->set_attribute("at_dir", toString(orientation_mode).c_str());
}

void VRTransform::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);

    Vec3f f, a, u;
    f = toVec3f(e->get_attribute("from")->get_value());
    a = toVec3f(e->get_attribute("at")->get_value());
    u = toVec3f(e->get_attribute("up")->get_value());
    if (e->get_attribute("scale")) _scale = toVec3f(e->get_attribute("scale")->get_value());

    setPose(f, a, u);
    setAt(a);

    if (e->get_attribute("cT_mode")) tConPlane = toBool(e->get_attribute("cT_mode")->get_value());
    if (e->get_attribute("do_cT")) doTConstraint = toBool(e->get_attribute("do_cT")->get_value());
    if (e->get_attribute("do_cR")) doRConstraint = toBool(e->get_attribute("do_cR")->get_value());
    if (e->get_attribute("cT")) tConstraint = toVec3f(e->get_attribute("cT")->get_value());
    if (e->get_attribute("cR")) rConstraint = toVec3i(e->get_attribute("cR")->get_value());

    if (e->get_attribute("at_dir")) orientation_mode = toInt(e->get_attribute("at_dir")->get_value());

    if(doTConstraint || doRConstraint) setFixed(false);
}

void setFromPath(VRTransform* tr, path* p, bool redirect, float t) {
    tr->setFrom( p->getPosition(t) );
    if (redirect) {
        Vec3f d,u;
        p->getOrientation(t, d, u);
        tr->setDir( d );
        tr->setUp( u );
    }
}

void VRTransform::addAnimation(VRAnimation* anim) { animations[anim->getName()] = anim; }
vector<VRAnimation*> VRTransform::getAnimations() {
    vector<VRAnimation*> res;
    for (auto a : animations) res.push_back(a.second);
    return res;
}

void VRTransform::startPathAnimation(path* p, float time, float offset, bool redirect, bool loop) {
    VRFunction<float>* fkt = new VRFunction<float>("TransAnim", boost::bind(setFromPath, this, p, redirect, _1));
    VRScene* scene = VRSceneManager::getCurrent();
    VRAnimation* a = scene->addAnimation(time, offset, fkt, 0.f, 1.f, loop);
    addAnimation(a);
}

void VRTransform::stopAnimation() {
    for (auto a : animations) a.second->stop();
}

list<VRTransform* > VRTransform::dynamicObjects = list<VRTransform* >();
list<VRTransform* > VRTransform::changedObjects = list<VRTransform* >();

OSG_END_NAMESPACE;
