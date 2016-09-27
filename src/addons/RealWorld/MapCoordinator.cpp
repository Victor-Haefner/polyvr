#include "MapCoordinator.h"
#include "Elevation.h"
#include "Altitude.h"
#include "Config.h"

using namespace OSG;

MapCoordinator::MapCoordinator(Vec2f zeroPos, float gridSize) {
    this->zeroPos = zeroPos;
    this->gridSize = gridSize;
    ele = new Elevation;
    startElevation = ele->getElevation(zeroPos[0], zeroPos[1]);
};

Vec2f MapCoordinator::realToWorld(Vec2f realPosition) {
    return (realPosition - zeroPos) * SCALE_REAL_TO_WORLD;
}

Vec2f MapCoordinator::worldToReal(Vec2f worldPosition) {
    return zeroPos + worldPosition* (1.0/SCALE_REAL_TO_WORLD);
}

float MapCoordinator::getGridSize() { return gridSize; }
float MapCoordinator::getElevation(float x, float y) { return getElevation(Vec2f(x, y)); }

float MapCoordinator::getElevation(Vec2f v) {
    return 0; // TODO
    Vec2f real = worldToReal(v);
    float res = ele->getElevation(real[0], real[1]) - startElevation - 1;
    return res;
}

//RelBbox?
Vec2f MapCoordinator::getRealBboxPosition(Vec2f worldPosition) {
    Vec2f realPos = worldToReal(worldPosition);
    float gridX = floor(floor((realPos[0]*1000) / (gridSize*1000)) * (gridSize*1000));
    float gridY = floor(floor((realPos[1]*1000) / (gridSize*1000)) * (gridSize*1000));
    return Vec2f(gridX / 1000.0f, gridY / 1000.0f) + offset;
}

/** returns normals for 3D planes **/
Vec3f MapCoordinator::getSurfaceNormal(Vec3f v1, Vec3f v2){
    float v1x = v1[0];
    float v1y = v1[1];
    float v1z = v1[2];
    float v2x = v2[0];
    float v2y = v2[1];
    float v2z = v2[2];

    Vec3f n = Vec3f(v1y*v2z - v1z*v2y, v1z*v2x - v1x*v2z, v1x*v2y - v1y*v2x);
    return n[1] < 0 ? -n : n; // makes normal point upwards
}
