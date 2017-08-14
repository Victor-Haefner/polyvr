#include "VRRoad.h"
#include "core/utils/toString.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRStroke.h"
#include "addons/Semantics/Reasoning/VRProperty.h"
#include "addons/Semantics/Reasoning/VREntity.h"

using namespace OSG;

VRRoad::VRRoad() : VRRoadBase("Road") {}
VRRoad::~VRRoad() {}

VRRoadPtr VRRoad::create() { return VRRoadPtr( new VRRoad() ); }

float VRRoad::getWidth() {
    float width = 0;
    for (auto lane : entity->getAllEntities("lanes")) width += toFloat( lane->get("width")->value );
    return width;
}

VREntityPtr VRRoad::getNodeEntry( VREntityPtr node ) {
    /*string rN = entity->getName();
    string nN = node->getName();
    auto nodeEntry = entity->ontology.lock()->process("q(e):NodeEntry(e);Node("+nN+");Road("+rN+");has("+rN+".path,e);has("+nN+",e)");
    return nodeEntry[0];*/

    for (auto rp : entity->getAllEntities("path")) {
        for (auto rnE : rp->getAllEntities("nodes")) {
            for (auto nE : node->getAllEntities("paths")) {
                if (rnE == nE) return nE;
            }
        }
    }
    return 0;
}

VRRoad::edgePoint& VRRoad::getEdgePoints( VREntityPtr node ) {
    if (edgePoints.count(node) == 0) {
        float width = getWidth();
        VREntityPtr rEntry = getNodeEntry( node );
        Vec3d norm = rEntry->getVec3f("direction") * toInt(rEntry->get("sign")->value);
        Vec3d x = Vec3d(0,1,0).cross(norm);
        x.normalize();
        Vec3d pNode = node->getVec3f("position");
        Vec3d p1 = pNode - x * 0.5 * width; // right
        Vec3d p2 = pNode + x * 0.5 * width; // left
        edgePoints[node] = edgePoint(p1,p2,norm,rEntry);
    }
    return edgePoints[node];
}

VRGeometryPtr VRRoad::createGeometry() {
	auto strokeGeometry = [&]() {
	    float width = getWidth();
		float W = width*0.5;
		vector<Vec3d> profile;
		profile.push_back(Vec3d(-W,0,0));
		profile.push_back(Vec3d(W,0,0));

		auto geo = VRStroke::create("road");
		vector<pathPtr> paths;
		for (auto p : entity->getAllEntities("path")) {
            paths.push_back( toPath(p,64) );
		}
		geo->setPaths( paths );
		geo->strokeProfile(profile, 0, 0);
		return geo;
	};

	auto geo = strokeGeometry();
	setupTexCoords( geo, entity );
	return geo;
}


void VRRoad::computeMarkings() {
    float mw = 0.15;

    // road data
    vector<VREntityPtr> nodes;
    vector<Vec3d> normals;
    VREntityPtr pathEnt = entity->getEntity("path");
    if (!pathEnt) return;

    VREntityPtr nodeEntryIn = pathEnt->getEntity("nodes",0);
    VREntityPtr nodeEntryOut = pathEnt->getEntity("nodes",1);
    VREntityPtr nodeIn = nodeEntryIn->getEntity("node");
    VREntityPtr nodeOut = nodeEntryOut->getEntity("node");
    Vec3d normIn = nodeEntryIn->getVec3f("direction");
    Vec3d normOut = nodeEntryOut->getVec3f("direction");

    float roadWidth = getWidth();
    auto lanes = entity->getAllEntities("lanes");
    int Nlanes = lanes.size();

    // compute markings nodes
    auto path = toPath(pathEnt, 12);
    for (auto point : path->getPoints()) {
        Vec3d p = point.pos();
        Vec3d n = point.dir();
        Vec3d x = point.x();
        x.normalize();

        float widthSum = -roadWidth*0.5;
        for (int li=0; li<Nlanes; li++) {
            auto lane = lanes[li];
            float width = toFloat( lane->get("width")->value );
            float k = widthSum + mw*0.5;

            Vec3d pi = x*k + p;
            nodes.push_back(addNode(pi));
            normals.push_back(n);
            widthSum += width;
        }
        nodes.push_back(addNode(x*(roadWidth*0.5 - mw*0.5) + p));
        normals.push_back(n);
    }

    // markings
    int pathN = path->size();
    float L = path->getLength();
    string Ndots = toString(int(L*0.5));
    int lastDir = 0;
    for (int li=0; li<Nlanes+1; li++) {
        vector<VREntityPtr> nodes2;
        vector<Vec3d> normals2;
        for (int pi=0; pi<pathN; pi++) {
            int i = pi*(Nlanes+1)+li;
            nodes2.push_back( nodes[i] );
            normals2.push_back( normals[i] );
        }
        auto mL = addPath("RoadMarking", name, nodes2, normals2);
        mL->set("width", toString(mw));
        entity->add("markings", mL->getName());

        if (li != Nlanes) {
            auto lane = lanes[li];
            if (!lane->is_a("Lane")) { lastDir = 0; continue; }
            int direction = toInt( lane->get("direction")->value );
            if (li != 0 && lastDir*direction > 0) {
                mL->set("style", "dashed");
                mL->set("dashNumber", Ndots);
            }
            lastDir = direction;
        }
    }
}





