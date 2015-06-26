#ifndef MAPCOORDINATOR_H
#define	MAPCOORDINATOR_H

#include "Altitude.h"
#include "Config.h"
#include "Elevation.h"

using namespace OSG;
using namespace std;

namespace realworld {
    class MapCoordinator {
    public:
        Vec2f zeroPos;
        float gridSize;
//        static const float SCALE_REAL_TO_WORLD = 4000000.0f;
        //static const float SCALE_REAL_TO_WORLD = 80000.0f; //meters 111000?
        float SCALE_REAL_TO_WORLD;
        //static const float SCALE_REAL_TO_WORLD = 111000.0f;
        Elevation* ele;
        float startElevation;

        MapCoordinator(Vec2f zeroPos, float gridSize) {
            SCALE_REAL_TO_WORLD = 111000.0f;
            this->zeroPos = zeroPos;
            this->gridSize = gridSize;
            this->ele = new Elevation;
            Vec2f real = worldToReal(zeroPos);
            this->startElevation = ele->getElevation(real.getValues()[0], real.getValues()[1]);
        };

        Vec2f realToWorld(Vec2f realPosition) {
            return Vec2f(
                    (realPosition.getValues()[0] - this->zeroPos.getValues()[0]) * SCALE_REAL_TO_WORLD,
                    (realPosition.getValues()[1] - this->zeroPos.getValues()[1]) * SCALE_REAL_TO_WORLD);
        }

        Vec2f worldToReal(Vec2f worldPosition) {
            return Vec2f(
                    this->zeroPos.getValues()[0] + (worldPosition.getValues()[0] / SCALE_REAL_TO_WORLD),
                    this->zeroPos.getValues()[1] + (worldPosition.getValues()[1] / SCALE_REAL_TO_WORLD));
        }

        float getElevation(float x, float y){
            return getElevation(Vec2f(x, y));
        }

        float getElevation(Vec2f v) {
            return 0; // TODO
            Vec2f real = worldToReal(v);
            float res = ele->getElevation(real.getValues()[0], real.getValues()[1]) - startElevation - 1;
            //cout << "Elevation: " << res << endl;
            return res;
        }

        //RelBbox?
        Vec2f getRealBboxPosition(Vec2f worldPosition) {
            Vec2f realPos = this->worldToReal(worldPosition);
            float gridX = floor(floor((realPos.getValues()[0]*1000) / (this->gridSize*1000)) * (this->gridSize*1000));
            float gridY = floor(floor((realPos.getValues()[1]*1000) / (this->gridSize*1000)) * (this->gridSize*1000));
            return Vec2f(gridX / 1000.0f, gridY / 1000.0f);
        }

        /** returns normals for 3D planes **/
        static Vec3f getNormal3D(Vec3f v1, Vec3f v2){
            float v1x = v1.getValues()[0];
            float v1y = v1.getValues()[1];
            float v1z = v1.getValues()[2];
            float v2x = v2.getValues()[0];
            float v2y = v2.getValues()[1];
            float v2z = v2.getValues()[2];

            return Vec3f(v1y*v2z - v1z*v2y,
                v1z*v2x - v1x*v2z,
                v1x*v2y - v1y*v2x);
        }

        /** makes normal show up, if it shows down */
        static Vec3f getPositioveNormal3D(Vec3f v1, Vec3f v2){
            Vec3f res = MapCoordinator::getNormal3D(v1, v2);
            if(res.getValues()[1] < 0) return -res;
            else return res;
        }

    };
};

#endif	/* MAPCOORDINATOR_H */

