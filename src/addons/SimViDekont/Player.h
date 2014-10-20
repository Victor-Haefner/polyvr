#ifndef Player_H
#define Player_H

#include "core/utils/VRFunction.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Player {

    private:
        int currentFrame;
        int playDir;
        int start;
        int end;
        VRFunction<int>* callback;
        bool playStep;

    public:
        Player(VRFunction<int>* fkt, int start, int end) : currentFrame(0), playDir(0), start(start), end(end), callback(fkt) {}

        void play(bool back = false) { playStep=0;playDir = 1 - back*2; }
        void stop() { playDir = 0; }
        void toggle() { playDir *= -1; }
        void jump(int i) { currentFrame = i; }

        void step(bool back = false){
            playStep=1;
            int stepDir = 1 - back*2;
            currentFrame += stepDir;

            if (currentFrame < start) currentFrame = start;
            if (currentFrame >= end) currentFrame = end-1;
            (*callback)(currentFrame);
        }

        void update() {
            if(!playStep)
                currentFrame += playDir;

            if (currentFrame < start) currentFrame = start;
            if (currentFrame >= end) currentFrame = end-1;

            (*callback)(currentFrame);
        }
};


OSG_END_NAMESPACE
#endif // Player_H
