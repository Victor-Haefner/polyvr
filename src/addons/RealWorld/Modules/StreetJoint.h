#ifndef STREETJOINT_H
#define	STREETJOINT_H

#include <boost/foreach.hpp>

using namespace std;

namespace realworld {
    struct JointPoints {
        OSG::Vec2f left;
        OSG::Vec2f right;
        OSG::Vec2f leftExt;
    };

    class StreetJoint {
    public:
        OSG::Vec2f position;
        string id;
        vector<string> segmentIds;
        string info;
        bool bridge;
        bool smallBridge;

        vector<JointPoints*> jointPointCache_;
        bool calcSegPoints_;

        StreetJoint(OSG::Vec2f position, string id) {
            this->position = position;
            this->id = id;
            this->calcSegPoints_ = false;
            this->bridge = false;
            this->smallBridge = false;
        }

        void merge(StreetJoint* streetJoint) {
            BOOST_FOREACH(string segId, streetJoint->segmentIds) {
                if (find(this->segmentIds.begin(), this->segmentIds.end(), segId) == this->segmentIds.end()) {
                    this->segmentIds.push_back(segId);
                }
            }
            this->info = this->info + " +merge(" + streetJoint->info + ")";
            this->resetCaches();
        }

        // call this, when joint needs recalculation
        void resetCaches() {
            this->calcSegPoints_ = false;
            this->jointPointCache_.clear();
        }
    };
};



#endif	/* STREETJOINT_H */

