#ifndef OSMWAY_H
#define OSMWAY_H

using namespace OSG;
using namespace std;

namespace realworld {

    class OSMWay
    {
        public:
            string id;
            map<string, string> tags;
            vector<string> nodeRefs;

            OSMWay(string id) {
                this->id = id;
            }
        protected:
        private:
    };
};

#endif // OSMWAY_H
