#ifndef VR3DENTITY_H_INCLUDED
#define VR3DENTITY_H_INCLUDED

#include "object/VRObject.h"
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGLine.h>
#include <OpenSG/OSGFieldContainerFields.h>

class VRPhysics;

OSG_BEGIN_NAMESPACE;
using namespace std;

class doubleBuffer;
class path;
class Transform; OSG_GEN_CONTAINERPTR(Transform);
class VRAnimation;

class VRTransform : public VRObject {
    public:
        enum ORIANTATION_MODE {
            OM_AT = 0,
            OM_DIR = 1,
            OM_EULER = 2
        };

    protected:
        doubleBuffer* dm;
        TransformRecPtr t;//OSG Transform
        bool noBlt;
        VRPhysics* physics;

        unsigned int change_time_stamp = 0;
        unsigned int wchange_time_stamp = 0;
        unsigned int apply_time_stamp = 0;
        bool change;
        bool fixed;
        bool cam_invert_z;
        int orientation_mode = OM_DIR;

        Vec3f _at;
        Vec3f _from;
        Vec3f _up;
        Vec3f _scale;
        Vec3f _euler;
        NodeRecPtr coords;
        NodeRecPtr translator;

        int frame;
        Matrix WorldTransformation;

        Matrix constraints_reference;
        bool doTConstraint;
        bool doRConstraint;
        bool tConPlane;
        Vec3f tConstraint;
        Vec3i rConstraint;

        bool held = false;//drag n drop
        VRObject* old_parent = 0;
        int old_child_id = 0;

        VRObject* copy(vector<VRObject*> children);

        void computeMatrix();

        //read matrix from doublebuffer && apply it to transformation
        //should be called from the main thread only
        void updateTransformation();

        void reg_change();

        bool checkWorldChange();

        void printInformation();

        void initCoords();
        void initTranslator();

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

    public:
        VRTransform(string name = "");
        virtual ~VRTransform();

        static list<VRTransform* > changedObjects;
        static list<VRTransform* > dynamicObjects;

        uint getLastChange();
        bool changedNow();
        doubleBuffer* getBuffer();
        // Local && world transformation setter && getter

        Vec3f getFrom();
        Vec3f getDir();
        Vec3f getAt();
        Vec3f getUp();
        Vec3f getScale();
        Vec3f getEuler();
        void getMatrix(Matrix& _m);
        Matrix getMatrix();

        void setFrom(Vec3f pos);
        void setAt(Vec3f at);
        void setUp(Vec3f up);
        void setDir(Vec3f dir);
        void setScale(float s);
        void setScale(Vec3f s);
        void setOrientation(Vec3f at, Vec3f up);
        void setEuler(Vec3f euler);
        void setPose(Vec3f from, Vec3f dir, Vec3f up);
        void setMatrix(Matrix _m);

        void getWorldMatrix(Matrix& _m, bool parentOnly = false);
        Matrix getWorldMatrix(bool parentOnly = false);
        Vec3f getWorldPosition(bool parentOnly = false);
        Vec3f getWorldDirection(bool parentOnly = false);

        void setWorldMatrix(Matrix _m);
        void setWorldPosition(Vec3f pos);
        void setWorldOrientation(Vec3f dir, Vec3f up);

        int get_orientation_mode();
        void set_orientation_mode(int b);

        void setFixed(bool b);

        //-------------------------------------

        void showCoordAxis(bool b);

        void rotate(float a);
        void rotate(float a, Vec3f v);
        void rotateUp(float a);
        void rotateX(float a);
        void rotateAround(float a);
        void translate(Vec3f v);
        void zoom(float d);
        void move(float d);

        void drag(VRTransform* new_parent);
        void drop();
        void rebaseDrag(VRObject* new_parent);
        VRObject* getDragParent();

        /** Cast a ray in world coordinates from the object in its local coordinates, -z axis defaults **/
        Line castRay(VRObject* obj = 0, Vec3f dir = Vec3f(0,0,-1));

        map<string, VRAnimation*> animations;
        void addAnimation(VRAnimation* animation);
        vector<VRAnimation*> getAnimations();
        void startPathAnimation(path* p, float time, float offset, bool redirect = true, bool loop = false);
        void stopAnimation();

        /** Print the position of the object in local && world coords **/
        void printPos();

        /** Print the positions of all the subtree **/
        void printTransformationTree(int indent = 0);

        void setRestrictionReference(Matrix m);
        void toggleTConstraint(bool b);
        void toggleRConstraint(bool b);
        void setTConstraint(Vec3f trans);
        void setTConstraintMode(bool plane);
        bool getTConstraintMode();
        void setRConstraint(Vec3i rot);

        Vec3f getTConstraint();
        Vec3i getRConstraint();

        bool hasTConstraint();
        bool hasRConstraint();

        /** enable constraints on the object when dragged, 0 leaves the dof free, 1 restricts it **/
        void apply_constraints();

        /** Set the physics object **/
        VRPhysics* getPhysics();
        void updateFromBullet();

        /** Do not update the transform in the physics context for the next frame **/
        void setNoBltFlag();

        /** Update the object OSG transformation **/
        virtual void update();
        void updatePhysics();
};

OSG_END_NAMESPACE;

#endif // VR3DENTITY_H_INCLUDED
