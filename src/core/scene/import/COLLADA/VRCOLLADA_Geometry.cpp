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
    //cout << "VRCOLLADA_Geometry::handleInput " << type << " " << sourceID << " " << set << endl;
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

            if (type == "COLOR" && source.stride == 3) {
                for (int i=0; i<source.count; i++) {
                    int k = i*source.stride;
                    Color3f col(source.data[k], source.data[k+1], source.data[k+2]);
                    currentGeoData->pushColor(col);
                }
            }

            if (type == "COLOR" && source.stride == 4) {
                for (int i=0; i<source.count; i++) {
                    int k = i*source.stride;
                    Color4f col(source.data[k], source.data[k+1], source.data[k+2], source.data[k+3]);
                    currentGeoData->pushColor(col);
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
    if (!library_geometries.count(geoID)) return;
    lastInstantiatedGeo = dynamic_pointer_cast<VRGeometry>( library_geometries[geoID]->duplicate() );
    parent->addChild( lastInstantiatedGeo );
}

void VRCOLLADA_Geometry::setMaterial(VRMaterialPtr mat) {
    if (lastInstantiatedGeo) lastInstantiatedGeo->setMaterial(mat);
}

void VRCOLLADA_Geometry::newPrimitive(string name, string count) {
    //cout << "VRCOLLADA_Geometry::newPrimitive " << name << " " << count << " " << stride << endl;
    inPrimitive = true;
    currentPrimitive = Primitive();
    currentPrimitive.name = name;
    currentPrimitive.count = toInt(count);
}

void VRCOLLADA_Geometry::closeGeometry() {
    //cout << "VRCOLLADA_Geometry::closeGeometry" << endl;
    currentGeoData->apply(currentGeometry, 1, 1);
    currentGeometry = 0;
    currentGeoData = 0;
    sources.clear();
}

void VRCOLLADA_Geometry::closePrimitive() {
    //cout << "VRCOLLADA_Geometry::closePrimitive" << endl;
    inPrimitive = false;
}

void VRCOLLADA_Geometry::setSourceData(string data) {
    if (currentSource != "") {
        sources[currentSource].data = toValue<vector<float>>(data);
    }
}

void VRCOLLADA_Geometry::handleVCount(string data) {
    if (currentGeoData && inPrimitive) {
        auto lengths = toValue<vector<int>>(data);
        for (auto l : lengths) {
            if (l == 1) currentGeoData->updateType(GL_POINTS, 1);
            if (l == 2) currentGeoData->updateType(GL_LINES, 2);
            if (l == 3) currentGeoData->updateType(GL_TRIANGLES, 3);
            if (l == 4) currentGeoData->updateType(GL_QUADS, 4);
            if (l >= 5) currentGeoData->updateType(GL_POLYGON, l);
        }
    }
}

void VRCOLLADA_Geometry::handleIndices(string data) {
    if (currentGeoData && inPrimitive) {
        auto indices = toValue<vector<int>>(data);
        //cout << "VRCOLLADA_Geometry::handleIndices " << currentPrimitive.name << " " << indices.size() << endl;

        if (currentPrimitive.name == "triangles") currentGeoData->pushType(GL_TRIANGLES);
        if (currentPrimitive.name == "trifans") currentGeoData->pushType(GL_TRIANGLE_FAN);
        if (currentPrimitive.name == "tristrips") currentGeoData->pushType(GL_TRIANGLE_STRIP);

        int N = indices.size() / currentPrimitive.inputs.size();
        if (currentPrimitive.name != "polylist") currentGeoData->pushLength(N);

        for (int i=0; i<N; i++) {
            for (auto& input : currentPrimitive.inputs) { // for each input
                int m = i*currentPrimitive.inputs.size() + input.offset;
                if (m >= indices.size()) continue;
                if (input.type == "VERTEX") currentGeoData->pushIndex(indices[m]);
                if (input.type == "NORMAL") currentGeoData->pushNormalIndex(indices[m]);
                if (input.type == "COLOR") currentGeoData->pushColorIndex(indices[m]);
                if (input.type == "TEXCOORD") currentGeoData->pushTexCoordIndex(indices[m]);
            }
        }
    }
}
