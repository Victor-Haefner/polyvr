#ifndef TESTMOVER_H
#define TESTMOVER_H

using namespace OSG;
using namespace std;

 {
    class TestMover;
    TestMover* currentInstanceTestMover;

    class TestMover {
        private:
            VRScene* scene;
            VRTransformPtr obj;
            bool shouldRotateLeft;
            bool shouldRotateRight;
            bool shouldMoveForward;
            bool shouldMoveBackward;
            bool shouldMoveUp;
            bool shouldMoveDown;
            float timeLastFrame;

        public:
            TestMover(VRScene* scene, VRTransformPtr obj) {
                this->scene = scene;
                this->obj = obj;

                shouldRotateLeft = false; shouldRotateRight = false; shouldMoveBackward = false; shouldMoveForward = false; shouldMoveUp = false; shouldMoveDown= false;

                // register keyboard functions
                currentInstanceTestMover = this;
                glutKeyboardFunc(glutKeyDown);
                glutKeyboardUpFunc(glutKeyUp);

                // register update function
                VRFunction<int>* ufkt = new VRFunction<int>("realworld update fkt", boost::bind(&TestMover::update, this));
                scene->addUpdateFkt(ufkt, 1);

                timeLastFrame = glutGet(GLUT_ELAPSED_TIME);
            }

            void update() {
                // calc time delta in seconds
                float timeNow = glutGet(GLUT_ELAPSED_TIME);
                float dt = (timeNow - timeLastFrame) * 0.001f;
                timeLastFrame = timeNow;

                // do movement
                if (shouldRotateLeft) {
                    obj->rotate(+2.0f * dt);
                }
                if (shouldRotateRight) {
                    obj->rotate(-2.0f * dt);
                }
                if (shouldMoveForward) {
                    obj->move(40 * dt);
                }
                if (shouldMoveBackward) {
                    obj->move(-40 * dt);
                }
                if (shouldMoveUp) {
                    obj->translate(Vec3f(0, 10, 0) * dt);
                }
                if (shouldMoveDown) {
                    obj->translate(Vec3f(0, -10, 0) * dt);
                }
            }

            static void glutKeyDown(unsigned char key, int x, int y) { currentInstanceTestMover->onKeyDown(key); }
            void onKeyDown(unsigned char key) {
                if (key == 'e') {
                    shouldMoveUp = true;
                }
                if (key == 'q') {
                    shouldMoveDown = true;
                }
                if (key == 'a') {
                    shouldRotateLeft = true;
                }
                if (key == 'd') {
                    shouldRotateRight = true;
                }
                if (key == 's') {
                    shouldMoveBackward = true;
                }
                if (key == 'w') {
                    shouldMoveForward = true;
                }
            }

            static void glutKeyUp(unsigned char key, int x, int y) { currentInstanceTestMover->onKeyUp(key); }
            void onKeyUp(unsigned char key) { //VRDevicePtr dev) {
                if (key == 'e') {
                    shouldMoveUp = false;
                }
                if (key == 'q') {
                    shouldMoveDown = false;
                }
                if (key == 'a') {
                    shouldRotateLeft = false;
                }
                if (key == 'd') {
                    shouldRotateRight = false;
                }
                if (key == 's') {
                    shouldMoveBackward = false;
                }
                if (key == 'w') {
                    shouldMoveForward = false;
                }
            }
    };
}

#endif // TESTMOVER_H
