#ifndef VRCOLLADA_GEOMETRY_H_INCLUDED
#define VRCOLLADA_GEOMETRY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/import/VRImportFwd.h"
#include "core/objects/VRObjectFwd.h"

#include <vector>
#include <string>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCOLLADA_Geometry : public std::enable_shared_from_this<VRCOLLADA_Geometry> {
	private:
        struct Source {
            int count = 0;
            int stride = 0;
            map<VRGeoDataPtr, size_t> users;
            vector<float> data;
        };

        struct Input {
            string source;
            string type;
            int offset = 0;
            int set = 0;
        };

        struct Primitive {
            string name;
            int count;
            vector<Input> inputs;
        };

        map<string, VRGeometryPtr> library_geometries;
        VRGeometryPtr currentGeometry;
        VRGeoDataPtr currentGeoData;
        Primitive currentPrimitive;
        bool inPrimitive = false;

        map<string, Source> sources;
        string currentSource;
	public:
		VRCOLLADA_Geometry();
		~VRCOLLADA_Geometry();

		static VRCOLLADA_GeometryPtr create();
		VRCOLLADA_GeometryPtr ptr();

		void finalize();
		VRGeometryPtr getGeometry(string gid);

		void newGeometry(string name, string id);
		void newSource(string id);
        void handleAccessor(string count, string stride);
        void handleInput(string type, string sourceID, string offsetStr, string set);
        void handleVCount(string data);
        void newPrimitive(string name, string count);

        void setSourceData(string data);
        void handleIndices(string data);
        void closeGeometry();
        void closePrimitive();
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_GEOMETRY_H_INCLUDED
