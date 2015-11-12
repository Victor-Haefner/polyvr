#include "Wall.h"

using namespace OSG;

Wall::Wall(string id){
    this->id = id;
}

vector<Vec2f*> Wall::getSides() {
    vector<Vec2f*> result;

    for (unsigned int i=0; i<this->positions.size()-1; i++) {
        Vec2f* side = new Vec2f[2];
        side[0] = this->positions[i];
        side[1] = this->positions[(i+1) % this->positions.size()];
        result.push_back(side);
    }

    return result;
}

vector<Vec2f> Wall::getCorners(){
    return positions;
}

bool Wall::sortVerticesX(Vec2fWithAdjIdx* vai1, Vec2fWithAdjIdx* vai2) {
    return (vai1->pos.getValues()[0] > vai2->pos.getValues()[0]);
}
