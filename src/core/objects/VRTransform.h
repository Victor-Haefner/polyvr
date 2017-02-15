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

        Vec3f _at = Vec3f(0,0,-1);
        Vec3f _from;
        Vec3f _up = Vec3f(0,1,0);
        Vec3f _scale = Vec3f(1,1,1);
        Vec3f _euler;
        OSGObjectPtr coords;
        OSGObjectPtr translator;

        int frame = 0;
        Matrix WorldTransformation;
        VRConstraintPtr constraint;

        bool held = false;//drag n drop
        VRObjectWeakPtr old_parent;
        Matrix old_transformation;
        int old_child_id = 0;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        void computeMatrix();

        //read matrix from doublebuffer && apply it to transformation
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

        static list< VRTransformWeakPtr > changedObjects;
        static list< VRTransformWeakPtr > dynamicObjects;

        uint getLastChange();
        bool changedNow();
        doubleBuffer* getBuffer();
        // Local && world transformation setter && getter

        Vec3f getFrom();
        Vec3f getDir();
        Vec3f getAt();
        Vec3f getUp();
        Vec3f getScale();
        posePtr getPose();
        posePtr getWorldPose();
        Vec3f getEuler();
        void getMatrix(Matrix& _m);
        Matrix getMatrix();
        Matrix getMatrixTo(VRObjectPtr o);

        void setFrom(Vec3f pos);
        void setAt(Vec3f at);
        void setUp(Vec3f up);
        void setDir(Vec3f dir);
        void setScale(float s);
        void setScale(Vec3f s);
        void setOrientation(Vec3f at, Vec3f up);
        void setEuler(Vec3f euler);
        void setPose(posePtr p);
        void setPose(Vec3f from, Vec3f dir, Vec3f up);
        virtual void setMatrix(Matrix m);

        void getWorldMatrix(Matrix& _m, bool parentOnly = false);
        Matrix getWorldMatrix(bool parentOnly = false);
        Vec3f getWorldPosition(bool parentOnly = false);
        Vec3f getWorldDirection(bool parentOnly = false);
        Vec3f getWorldUp(bool parentOnly = false);

        void setWorldPose(posePtr p);
        void setWorldMatrix(Matrix _m);
        void setWorldPosition(Vec3f pos);
        void setWorldOrientation(Vec3f dir, Vec3f up);
        void setWorldDir(Vec3f dir);
        void setWorldUp(Vec3f up);

        int get_orientation_mode();
        void set_orientation_mode(int b);

        void setFixed(bool b);

        //-------------------------------------

        void showCoordAxis(bool b);

        void rotate(float a, Vec3f v = Vec3f(0,1,0));
        void rotateUp(float a);
        void rotateX(float a);
        void rotateAround(float a);
        void translate(Vec3f v);
        void zoom(float d);
        void move(float d);

        virtual void drag(VRTransformPtr new_parent);
        virtual void drop();
        void rebaseDrag(VRObjectPtr new_parent);
        VRObjectPtr getDragParent();
        bool isDragged();

        /** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
        Line castRay(VRObjectPtr obj = 0, Vec3f dir = Vec3f(0,0,-1));

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

OSG_END_NAMESPACE;

#endif // VR3DENTITY_H_INCLUDED
