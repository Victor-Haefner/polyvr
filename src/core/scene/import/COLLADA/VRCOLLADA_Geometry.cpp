#include "VRCOLLADA_Geometry.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace OSG;

VRCOLLADA_Geometry::VRCOLLADA_Geometry() {}
VRCOLLADA_Geometry::~VRCOLLADA_Geometry() {}

VRCOLLADA_GeometryPtr VRCOLLADA_Geometry::create() { return VRCOLLADA_GeometryPtr( new VRCOLLADA_Geometry() ); }
VRCOLLADA_GeometryPtr VRCOLLADA_Geometry::ptr() { return static_pointer_cast<VRCOLLADA_Geometry>(shared_from_this()); }

void VRCOLLADA_Geometry::newGeometry(string name, string id) {
    auto m = VRGeometry::create(name);
    library_geometries[id] = m;
    currentGeometry = m;
    currentGeoData = VRGeoData::create();
}

void VRCOLLADA_Geometry::newSource(string id) {
    currentSource = id;
    sources[currentSource] = Source();
}

void VRCOLLADA_Geometry::handleAccessor(string count, string stride) {
    if (currentSource != "") {
        sources[currentSource].count  = toInt(count);
        sources[currentSource].stride = toInt(stride);
    }
}

void VRCOLLADA_Geometry::handleInput(string type, string sourceID, string offsetStr, string set) {
    int offset = toInt(offsetStr);

    if (inPrimitive) {
        Input input;
        input.type = type;
        input.source = sourceID;
        input.offset = offset;
        currentPrimitive.inputs.push_back(input);
    }

    if (sources.count(sourceID)) {
        auto& source = sources[sourceID];

        if (currentGeoData) {
            if (type == "POSITION" && source.stride == 3) {
                for (int i=0; i<source.count; i++) {
                    int k = i*source.stride;
                    Vec3d pos(source.data[k], source.data[k+1], source.data[k+2]);
                    currentGeoData->pushVert(pos);
                }
            }

            if (type == "NORMAL" && source.stride == 3) {
                for (int i=0; i<source.count; i++) {
                    int k = i*source.stride;
                    Vec3d norm(source.data[k], source.data[k+1], source.data[k+2]);
                    currentGeoData->pushNorm(norm);
                }
            }

            if (type == "TEXCOORD" && source.stride == 2) {
                int tcSlot = toInt( set );
                for (int i=0; i<source.count; i++) {
                    int k = i*source.stride;
                    Vec2d tc(source.data[k], source.data[k+1]);
                    if (tcSlot == 0) currentGeoData->pushTexCoord(tc);
                    if (tcSlot == 1) currentGeoData->pushTexCoord2(tc);
                }
            }
        }
    }
}

void VRCOLLADA_Geometry::instantiateGeometry(string geoID, VRObjectPtr parent) {
    parent->addChild( library_geometries[geoID]->duplicate() );
}

void VRCOLLADA_Geometry::newPrimitive(string name, string count, int stride) {
    inPrimitive = true;
    currentPrimitive = Primitive();
    currentPrimitive.name = name;
    currentPrimitive.count = toInt(count);
    currentPrimitive.stride = stride;
}

void VRCOLLADA_Geometry::closeGeometry() {
    currentGeoData->apply(currentGeometry);
    currentGeometry = 0;
    currentGeoData = 0;
    sources.clear();
}

void VRCOLLADA_Geometry::closePrimitive() {
    inPrimitive = false;
}

void VRCOLLADA_Geometry::setSourceData(string data) {
    if (currentSource != "") {
        sources[currentSource].data = toValue<vector<float>>(data);
    }
}

void VRCOLLADA_Geometry::handleIndices(string data) {
    if (currentGeoData && inPrimitive) {
        auto indices = toValue<vector<int>>(data);

        if (currentPrimitive.name == "triangles") {
            currentGeoData->pushType(GL_TRIANGLES);
            currentGeoData->pushLength(3*currentPrimitive.count);
            for (int i=0; i<currentPrimitive.count; i++) { // for each triangle
                int k = i * currentPrimitive.stride * currentPrimitive.inputs.size();
                for (int j=0; j<currentPrimitive.stride; j++) { // for each vertex
                    int l = k + j * currentPrimitive.inputs.size();
                    for (auto& input : currentPrimitive.inputs) { // for each input
                        int m = l + input.offset;
                        if (input.type == "VERTEX") currentGeoData->pushIndex(indices[m]);
                        if (input.type == "NORMAL") currentGeoData->pushNormalIndex(indices[m]);
                        if (input.type == "TEXCOORD") currentGeoData->pushTexCoordIndex(indices[m]);
                    }
                }
            }
        }
    }
}
