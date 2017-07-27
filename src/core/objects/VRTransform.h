#ifndef VR3DENTITY_H_INCLUDED
#define VR3DENTITY_H_INCLUDED

#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>

#include "object/VRObject.h"
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/VRMathFwd.h"

class VRPhysics;

OSG_BEGIN_NAMESPACE;
using namespace std;

class doubleBuffer;
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
        doubleBuffer* dm = 0;
        OSGTransformPtr t;
        bool noBlt = false;
        VRPhysics* physics = 0;
        VRAnimCbPtr pathAnimPtr;

        unsigned int change_time_stamp = 0;
        unsigned int wchange_time_stamp = 0;
        bool change = false;
        bool fixed = true;
        bool cam_invert_z = false;
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

        Matrix4d old_transformation; //drag n drop

        VRObjectPtr copy(vector<VRObjectPtr> children);

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
        VRTransform(string name = "");
        virtual ~VRTransform();

        static VRTransformPtr create(string name = "None");
        VRTransformPtr ptr();

        static VRTransformPtr getParentTransform(VRObjectPtr o);

        static list< VRTransformWeakPtr > changedObjects;
        static list< VRTransformWeakPtr > dynamicObjects;

        uint getLastChange();
        bool changedNow();
        doubleBuffer* getBuffer();
        // Local && world transformation setter && getter

        Vec3d getFrom();
        Vec3d getDir();
        Vec3d getAt();
        Vec3d getUp();
        Vec3d getScale();
        posePtr getPose();
        posePtr getPoseTo(VRObjectPtr o);
        posePtr getWorldPose();
        Vec3d getEuler();
        void getMatrix(Matrix4d& _m);
        Matrix4d getMatrix();
        Matrix4d getMatrixTo(VRObjectPtr o, bool parentOnly = false);
        Matrix4d getRotationMatrix();

        void setFrom(Vec3d pos);
        void setAt(Vec3d at);
        void setUp(Vec3d up);
        void setDir(Vec3d dir);
        void setScale(float s);
        void setScale(Vec3d s);
        void setOrientation(Vec3d at, Vec3d up);
        void setEuler(Vec3d euler);
        void setPose(posePtr p);
        void setPose(Vec3d from, Vec3d dir, Vec3d up);
        virtual void setMatrix(Matrix4d m);

        void getWorldMatrix(Matrix4d& _m, bool parentOnly = false);
        Matrix4d getWorldMatrix(bool parentOnly = false);
        Vec3d getWorldPosition(bool parentOnly = false);
        Vec3d getWorldDirection(bool parentOnly = false);
        Vec3d getWorldUp(bool parentOnly = false);

        void setWorldPose(posePtr p);
        void setWorldMatrix(Matrix4d _m);
        void setWorldPosition(Vec3d pos);
        void setWorldOrientation(Vec3d dir, Vec3d up);
        void setWorldDir(Vec3d dir);
        void setWorldUp(Vec3d up);

        void getRelativeMatrix(Matrix4d& m, VRObjectPtr o, bool parentOnly = false);
        Matrix4d getRelativeMatrix(VRObjectPtr o, bool parentOnly = false);
        posePtr getRelativePose(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativePosition(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativeDirection(VRObjectPtr o, bool parentOnly = false);
        Vec3d getRelativeUp(VRObjectPtr o, bool parentOnly = false);

        void setRelativePosition(Vec3d pos, VRObjectPtr o);
        void setRelativeDir(Vec3d pos, VRObjectPtr o);
        void setRelativeUp(Vec3d pos, VRObjectPtr o);
        void setRelativePose(posePtr p, VRObjectPtr o);

        int get_orientation_mode();
        void set_orientation_mode(int b);

        void setFixed(bool b);

        //-------------------------------------

        void showCoordAxis(bool b);

        void rotate(float a, Vec3d v = Vec3d(0,1,0));
        void rotateUp(float a);
        void rotateX(float a);
        void rotateAround(float a);
        void translate(Vec3d v);
        void zoom(float d);
        void move(float d);
        void rotateYonZ();

        virtual void drag(VRTransformPtr new_parent);
        virtual void drop();
        void rebaseDrag(VRObjectPtr new_parent);
        VRObjectPtr getDragParent();
        bool isDragged();

        /** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
        Line castRay(VRObjectPtr obj = 0, Vec3d dir = Vec3d(0,0,-1));

        map<string, VRAnimationPtr> animations;
        void addAnimation(VRAnimationPtr animation);
        vector<VRAnimationPtr> getAnimations();
        VRAnimationPtr startPathAnimation(pathPtr p, float time, float offset, bool redirect = true, bool loop = false);
        void stopAnimation();

        void printPos(); // Print the position of the object in local && world coords
        void printTransformationTree(int indent = 0); // Print the positions of all the subtree

        void setConstraint(VRConstraintPtr c);
        VRConstraintPtr getConstraint();

        /** enable constraints on the object when dragged, 0 leaves the dof free, 1 restricts it **/
        void apply_constraints();

        /** Set the physics object **/
        VRPhysics* getPhysics();
        void updateFromBullet();

        /** Do not update the transform in the physics context for the next frame **/
        void setNoBltFlag();

        /** Update the object OSG transformation **/
        virtual void update();
        void setup();
        void updatePhysics();
};

Matrix4f toMatrix4f(Matrix4d);
Matrix4d toMatrix4d(Matrix4f);
bool MatrixLookDir(Matrix4d &result, Pnt3d from, Vec3d dir, Vec3d up);
bool MatrixLookAt (Matrix4d &result, Pnt3d from, Pnt3d at , Vec3d up);

OSG_END_NAMESPACE;

#endif // VR3DENTITY_H_INCLUDED
