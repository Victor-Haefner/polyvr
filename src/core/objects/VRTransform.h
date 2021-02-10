#ifndef VR3DENTITY_H_INCLUDED
#define VR3DENTITY_H_INCLUDED

#include <vector>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGQuaternion.h>

#include "object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/setup/devices/VRIntersect.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRCollision;
class VRPhysics;
class path;
class VRAnimation;

class VRTransform : public VRObject {
    public:
        enum ORIENTATION_MODE {
            OM_AT = 0,
            OM_DIR = 1,
            OM_EULER = 2
        };

    protected:
        Matrix4d matrix;
        OSGTransformPtr t;
        bool noBlt = false;
        bool bltOverride = false;
        bool doOptimizations = true;
        VRPhysics* physics = 0;
        VRAnimCbPtr pathAnimPtr;

        unsigned int change_time_stamp = 0;
        unsigned int wchange_time_stamp = 0;
        bool identity = true;
        int orientation_mode = OM_DIR;

        Vec3d _at = Vec3d(0,0,-1);
        Vec3d _dir = Vec3d(0,0,-1);
        Vec3d _from;
        Vec3d _up = Vec3d(0,1,0);
        Vec3d _scale = Vec3d(1,1,1);
        Vec3d _euler;
        OSGObjectPtr coords;
        OSGObjectPtr translator;

        int frame = 0;
        Matrix4d WorldTransformation;
        VRConstraintPtr constraint;
        map<VRTransform*, pair<VRConstraintPtr, VRTransformWeakPtr> > aJoints;
        map<VRTransform*, pair<VRConstraintPtr, VRTransformWeakPtr> > bJoints;

        Matrix4d old_transformation; //drag n drop

        virtual VRObjectPtr copy(vector<VRObjectPtr> children);

        void computeMatrix4d();

        //read Matrix4d from doublebuffer && apply it to transformation
        //should be called from the main thread only
        void updateTransformation();
        void reg_change();
        bool checkWorldChange();

        void printInformation();
        void initCoords();
        void initTranslator();

    public:
        VRTransform(string name = "", bool doOptimizations = true);
        virtual ~VRTransform();

        static VRTransformPtr create(string name = "None", bool doOptimizations = true);
        VRTransformPtr ptr();
        OSGTransformPtr getOSGTransformPtr();

        virtual void wrapOSG(OSGObjectPtr node);

        static VRTransformPtr getParentTransform(VRObjectPtr o);
        static Vec3d computeEulerAngles(const Matrix4d& t);
        static void applyEulerAngles(Matrix4d& t, Vec3d angles);

        static list< VRTransformWeakPtr > changedObjects;
        static list< VRTransformWeakPtr > dynamicObjects;

        unsigned int getLastChange();
        bool changedNow();
        bool changedSince(unsigned int& frame, bool includingFrame = true);
        bool changedSince2(unsigned int frame, bool includingFrame = true);

        Vec3d getFrom();
        Vec3d getDir();
        Vec3d getAt();
        Vec3d getUp();
        Vec3d getScale();
        PosePtr getPose();
        PosePtr getWorldPose();
        Vec3d getEuler();
        void getMatrix(Matrix4d& _m);
        Matrix4d getMatrix();
        Matrix4d getRotationMatrix();

        void setIdentity();
        void updateTransform(VRTransformPtr t);
        virtual void setFrom(Vec3d pos);
        virtual void setAt(Vec3d at);
        virtual void setUp(Vec3d up);
        virtual void setDir(Vec3d dir);
        void setScale(float s);
        void setScale(Vec3d s);
        void setOrientation(Vec3d dir, Vec3d up);
        void setOrientationQuat(Quaterniond q);
        void setOrientationQuat(Vec4d vecq);
        void setEuler(Vec3d euler);
        void setEulerDegree(Vec3d euler);
        void setTransform(Vec3d p, Vec3d d = Vec3d(0,0,-1), Vec3d u = Vec3d(0,1,0));
        void setPose2(Pose& p);
        void setPose(PosePtr p);
        virtual void setMatrix(Matrix4d m);
        void setMatrixTo(Matrix4d m, VRObjectPtr o);

        void getWorldMatrix(Matrix4d& _m, bool parentOnly = false);
        Matrix4d getWorldMatrix(bool parentOnly = false);
        Vec3d getWorldPosition(bool parentOnly = false);
        Vec3d getWorldDirection(bool parentOnly = false);
        Vec3d getWorldUp(bool parentOnly = false);
        Vec3d getWorldAt(bool parentOnly = false);
        Vec3d getWorldScale(bool parentOnly = false);

        void setWorldPose(PosePtr p);
        void setWorldMatrix(Matrix4d _m);
        void setWorldPosition(Vec3d pos);
        void setWorldOrientation(Vec3d dir, Vec3d up);
        void setWorldDir(Vec3d dir);
        void setWorldUp(Vec3d up);
        void setWorldAt(Vec3d at);
        void setWorldScale(Vec3d s);

        void getRelativeMatrix(Matrix4d& m, VRObjectPtr o, bool parentOnly = false);
        Matrix4d getRelativeMatrix(VRObjectPtr o, bool parentOnly = false);
        PosePtr getRelativePose(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativePosition(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativeDirection(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativeUp(VRObjectPtr o, bool parentOnly = false);

        void setRelativePosition(Vec3d pos, VRObjectPtr o);
        void setRelativeDir(Vec3d pos, VRObjectPtr o);
        void setRelativeUp(Vec3d pos, VRObjectPtr o);
        void setRelativePose(PosePtr p, VRObjectPtr o);

        int get_orientation_mode();
        void set_orientation_mode(int b);
        void enableOptimization(bool b);

        void applyTransformation(PosePtr p = 0);

        //-------------------------------------

        void showCoordAxis(bool b);

        void rotate(float a, Vec3d v = Vec3d(0,1,0), Vec3d o = Vec3d(0,0,0));
        void rotateWorld(float a, Vec3d v = Vec3d(0,1,0), Vec3d o = Vec3d(0,0,0));
        void rotateUp(float a);
        void rotateX(float a);
        void rotateAround(float a, Vec3d v = Vec3d(0,1,0));
        void translate(Vec3d v);
        void zoom(float d);
        void move(float d);
        void rotateYonZ();

        virtual void drag(VRTransformPtr new_parent, VRIntersection i = VRIntersection());
        virtual void drop();
        void rebaseDrag(VRObjectPtr new_parent);
        VRObjectPtr getDragParent();
        bool isDragged();

        // Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults
        Line castRay(VRObjectPtr obj = 0, Vec3d dir = Vec3d(0,0,-1));
        VRIntersection intersect(VRObjectPtr obj, Vec3d dir = Vec3d(0,0,-1));

        map<string, VRAnimationPtr> animations;
        void addAnimation(VRAnimationPtr animation);
        vector<VRAnimationPtr> getAnimations();
        VRAnimationPtr animate(PathPtr p, float time, float offset, bool redirect = true, bool loop = false, PathPtr po = 0);
        void stopAnimation();

        void printPos(); // Print the position of the object in local && world coords
        void printTransformationTree(int indent = 0); // Print the positions of all the subtree

        void setConstraint(VRConstraintPtr c);
        VRConstraintPtr getConstraint();

        void apply_constraints(bool force = false);
        static void updateConstraints();
        void attach(VRTransformPtr a, VRConstraintPtr c, VRConstraintPtr s = 0);
        void detachJoint(VRTransformPtr a);
        Vec3d getConstraintAngleWith(VRTransformPtr t, bool rotationOrPosition);
        void setSpringParameters(VRTransformPtr a, int dof, float stiffnes, float damping);

        virtual void updateChange();
        void setup(VRStorageContextPtr context);

#ifndef WITHOUT_BULLET
        VRPhysics* getPhysics();
        void resolvePhysics();
        void updateFromBullet();
        void updatePhysics();
        vector<VRCollision> getCollisions();

        void setNoBltFlag();
        void setBltOverrideFlag();
        void setConvexDecompositionParameters(float cw, float vw, float nc, float nv, float c, bool aedp, bool andp, bool afp);

        void physicalize(bool b, bool dynamic, string shape, float param = 0);
        void setPhysicalizeTree(bool b);
        void setCollisionGroup(vector<int> gv);
        void setCollisionMask(vector<int> gv);
        void setMass(float m);
        void setCollisionMargin(float m);
        void setCollisionShape(string s, float p);
        void setPhysicsActivationMode(int m);
        void applyImpulse(Vec3d i);
        void applyTorqueImpulse(Vec3d i);
        void applyForce(Vec3d f);
        void applyConstantForce(Vec3d f);
        void applyTorque(Vec3d f);
        void applyConstantTorque(Vec3d f);
        void setGravity(Vec3d g);
        void setCenterOfMass(Vec3d g);
        Vec3d getCenterOfMass();
        void setGhost(bool g);
        void setDamping(float ld, float ad);

        bool getPhysicsDynamic();
        void setPhysicsDynamic(bool b);

        Vec3d getForce();
        Vec3d getTorque();
#endif
};

Matrix4f toMatrix4f(Matrix4d);
Matrix4d toMatrix4d(Matrix4f);
bool MatrixLookDir(Matrix4d &result, Pnt3d from, Vec3d dir, Vec3d up);
bool MatrixLookAt (Matrix4d &result, Pnt3d from, Pnt3d at , Vec3d up);

OSG_END_NAMESPACE;

#endif // VR3DENTITY_H_INCLUDED
