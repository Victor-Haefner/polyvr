#include "Terrain.h"

using namespace OSG;

Terrain::Terrain(string id){
    this->id = id;
}

vector<Vec2f> Terrain::getCorners(){
    return positions;
}
