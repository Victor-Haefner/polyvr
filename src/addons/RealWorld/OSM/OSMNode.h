#ifndef OSMNODE_H
#define OSMNODE_H

using namespace OSG;
using namespace std;

namespace realworld {

    class OSMNode
    {
        public:
            string id;
            double lat;
            double lon;

            map<string, string> tags;

            OSMNode(string id, double lat, double lon) {
                this->id = id;
                this->lat = lat;
                this->lon = lon;
            }
        protected:
        private:
    };
};

#endif // OSMNODE_H
