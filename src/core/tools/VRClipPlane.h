#ifndef VRCLIPPLANE_H_INCLUDED
#define VRCLIPPLANE_H_INCLUDED

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRClipPlane {
    private:
        VR3DEntity* beacon;
        VRGeometry* handPlane;
        Vec4f plane;
        list<VRShader*> shaderList;
        bool active;

    public:
        VRClipPlane() {
            beacon = 0;
            active = false;
            plane = Vec4f(0, -1, 0, 0.1);
        }

        void setBeacon(VR3DEntity* b) {
            beacon = b;
            handPlane = new VRGeometry("handPlane");
            handPlane->setMesh(makePlaneGeo(0.2, 0.2, 1,1));
            handPlane->setPose(Vec3f(0,0,-0.3), Vec3f(0,1,0), Vec3f(1,0,0));
            b->addChild(handPlane);
            handPlane->setVisible(active);
        }
        void setEquation(Vec4f eq) { plane = eq; }

        void addShader(VRShader* shader) {
            shaderList.push_back(shader);
        }

        void removeShader(VRShader* shader) {
            shaderList.remove(shader);
        }

        void update() {
            if (beacon == 0) return;
            if (plane == 0) return;
            if (active == 0) return;

            plane = Vec4f(0.0, -1.0, 0.0, 0.0);
            Matrix m;
            beacon->getWorldMatrix(m);
            m.mult(plane,plane);
            //plane[3] = m[3][1];//n*r
            plane[3] = -plane.dot(m[3]);//n*r

            /*Line l = beacon->castRay();
            Plane p = Plane(l.getDirection(), 0.0);
            Pnt3f zp;
            p.intersect(l, zp);
            plane = Vec4f(0.0, -1.0, 0.0, zp[1]);*/

            list<VRShader*>::iterator itr;
            for (itr = shaderList.begin(); itr != shaderList.end(); itr++) {
                (*itr)->setParameter("cpeq", plane);
            }
        }

        void toggleActive() { setActive(!active); }
        void setActive(bool a) {
            active = a;
            handPlane->setVisible(a);
        }


        /*void updateClipPlane() {
            if (beacon == 0) {
                Plane p = Plane(VRMouse::get()->getRay().getDirection(), 0.0);
                Pnt3f zp;
                p.intersect(VRMouse::get()->getRay(), zp);
                clipplane = Vec4f(0.0, -1.0, 0.0, zp[1]);

//TEST code
//                Matrix m;
//                VRMouse::get()->getBeacon()->getWorldMatrix(m);
//
//                Vec3f n = Vec3f(0,-1,0);
//                m.mult(n, n);
//                Vec3f p0 = Vec3f(m[3]);
//                float d = n.dot(p0);
//
//                clipplane = Vec4f(n[0], n[1], n[2], -d);
//                cout << endl << m;
            } else {
                //to be tested
                clipplane = Vec4f(0.0, -1.0, 0.0, 0);
                Matrix m;
                flytracker->getWorldMatrix(m);
                m.mult(clipplane, clipplane);
            }

            nfs->setParameter("cpeq", clipplane);
        }*/
};

OSG_END_NAMESPACE;

#endif // VRCLIPPLANE_H_INCLUDED
