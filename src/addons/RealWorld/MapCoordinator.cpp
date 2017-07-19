#include "MapCoordinator.h"
#include "Elevation.h"
#include "Altitude.h"
#include "Config.h"

using namespace OSG;

MapCoordinator::MapCoordinator(Vec2d zeroPos, float gridSize) {
    this->zeroPos = zeroPos;
    this->gridSize = gridSize;
    //ele = new Elevation; // TODO: takes very long to start up
    //startElevation = ele->getElevation(zeroPos[0], zeroPos[1]);
};

Vec2d MapCoordinator::realToWorld(Vec2d realPosition) {
    return (realPosition - zeroPos) * SCALE_REAL_TO_WORLD;
}

Vec2d MapCoordinator::worldToReal(Vec2d worldPosition) {
    return zeroPos + worldPosition* (1.0/SCALE_REAL_TO_WORLD);
}

float MapCoordinator::getGridSize() { return gridSize; }
float MapCoordinator::getElevation(float x, float y) { return getElevation(Vec2d(x, y)); }

float MapCoordinator::getElevation(Vec2d v) {
    return 0; // TODO
    Vec2d real = worldToReal(v);
    float res = 0;
    if (ele) res = ele->getElevation(real[0], real[1]) - startElevation - 1;
    return res;
}

//RelBbox?
Vec2d MapCoordinator::getRealBboxPosition(Vec2d worldPosition) {
    Vec2d realPos = worldToReal(worldPosition);
    float gridX = floor(floor((realPos[0]*1000) / (gridSize*1000)) * (gridSize*1000));
    float gridY = floor(floor((realPos[1]*1000) / (gridSize*1000)) * (gridSize*1000));
    return Vec2d(gridX / 1000.0f, gridY / 1000.0f) + offset;
}


