#ifndef NATURALTREE_H
#define	NATURALTREE_H

using namespace OSG;
using namespace std;

namespace realworld {
    class NaturalTree {
    public:
        Vec2f position;
        string id;
        vector<string> segmentIds;
        string info;

        NaturalTree(Vec2f position, string id) {
            this->position = position;
            this->id = id;
            this->calcSegPoints_ = false;
        }

        void generateRandomTree(){
            //to do
        }
    };
};



#endif	/* NATURALTREE_H */


