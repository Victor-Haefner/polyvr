#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/VRLod.h"
#include "core/math/partitioning/Octree.h"
#include "core/math/pose.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/import/E57/E57.h"

#define GLSL(shader) #shader

using namespace OSG;
using namespace std;

VRPointCloud::VRPointCloud(string name) : VRTransform(name) {
    octree = Octree::create(10);
    mat = VRMaterial::create("pcmat");
    type = "PointCloud";
}

VRPointCloud::~VRPointCloud() {}

VRPointCloudPtr VRPointCloud::create(string name) { return VRPointCloudPtr( new VRPointCloud(name) ); }

void VRPointCloud::setupMaterial(bool lit, int pointsize, bool doSplat, float splatModifier) {
    mat->setLit(lit);
    mat->setPointSize(pointsize);

    if (doSplat) {
        mat->setVertexShader(splatVP, "splatVP");
        mat->setGeometryShader(splatGP, "splatGP");
        mat->setFragmentShader(splatFP, "splatFP");
        mat->setShaderParameter("splatModifier", splatModifier);
    }

    // TODO: add splatting option
    //  - compute tangent space at each point
    //  - compute density at each point
}

VRMaterialPtr VRPointCloud::getMaterial() { return mat; }
OctreePtr VRPointCloud::getOctree() { return octree; }

void VRPointCloud::applySettings(map<string, string> options) {
    bool lit = 0;
    int pointSize = 1;
    int leafSize = 10;
    if (options.count("lit")) lit = toInt(options["lit"]);
    if (options.count("leafSize")) leafSize = toInt(options["leafSize"]);
    if (options.count("pointSize")) pointSize = toInt(options["pointSize"]);
    if (options.count("keepOctree")) keepOctree = toInt(options["keepOctree"]);

    setupMaterial(lit, pointSize);
    octree->setResolution(leafSize);

    for (auto l : {"lod1", "lod2", "lod3", "lod4", "lod5"}) {
        if (options.count(l)) {
            Vec2d lod;
            toValue(options[l], lod);
            addLevel(lod[0], lod[1]);
        }
    }
}

void VRPointCloud::addLevel(float distance, int downsampling) {
    levels++;
    downsamplingRate.push_back(downsampling);
    lodDistances.push_back(distance);
}

void VRPointCloud::addPoint(Vec3d p, Splat c) {
    if (pointType == NONE) pointType = SPLAT;
    if (pointType != SPLAT) return;
    octree->add(p, new Splat(c), -1, true, 1e5);
}

void VRPointCloud::addPoint(Vec3d p, Color3ub c) {
    if (pointType == NONE) pointType = COLOR;
    if (pointType != COLOR) return;
    octree->add(p, new Color3ub(c), -1, true, 1e5);
}

void VRPointCloud::setupLODs() {
    for (auto leaf : octree->getAllLeafs()) {
        Vec3d center = leaf->getCenter();

        auto lod = VRLod::create("chunk");
        lod->setCenter(center);
        addChild(lod);

        for (int lvl=0; lvl<levels; lvl++) {
            auto geo = VRGeometry::create("lvl"+toString(lvl+1));
            geo->setMaterial(mat);
            geo->setFrom(center);
            lod->addChild(geo);
            if (lvl > 0) lod->addDistance(lodDistances[lvl-1]);

            VRGeoData chunk;
            for (int i = 0; i < leaf->dataSize(); i+=downsamplingRate[lvl]) {
                void* data = leaf->getData(i);
                Vec3d pos = leaf->getPoint(i);
                chunk.pushVert(pos - center, Vec3d(0,1,0));
                if (pointType == COLOR) {
                    Color3ub col = *((Color3ub*)data);
                    chunk.pushColor(col);
                } else if (pointType == SPLAT) {
                    Splat splat = *((Splat*)data);
                    chunk.pushColor(splat.c);
                    chunk.pushTexCoord(Vec2d(splat.v1), 0);
                    chunk.pushTexCoord(Vec2d(splat.v2), 1);
                    chunk.pushTexCoord(Vec2d(splat.w,0), 2);
                }
                chunk.pushPoint();

            }
            if (chunk.size() > 0) chunk.apply( geo );
        }

        if (!keepOctree) leaf->delContent<Color3ub>();
    }

    //addChild(octree->getVisualization());
}

void VRPointCloud::convert(string pathIn, string pathOut) {
    if (pathOut == "") pathOut = pathIn+".pcb";
    convertE57(pathIn, pathOut);
}

void VRPointCloud::genTestFile(string path, size_t N, bool doColor) {
    size_t n = cbrt(N);
    double f = 255.0/n;
    double s = 0.01;
    N = n*n*n;

    map<string, string> params;
    params["N_points"] = toString(N);
    params["format"] = "x8y8z8";
    if (doColor) params["format"] += "r1g1b1";
    writePCBHeader(path, params);

    auto progress = VRProgress::create();
    progress->setup("generate points ", N);
    progress->reset();

    ofstream stream(path, ios::app);

    for (size_t i=0; i<n; i++) {
        for (size_t j=0; j<n; j++) {
            for (size_t k=0; k<n; k++) {
                Vec3d P(i*s, j*s, k*s);
                Vec3ub C(i*f, j*f, k*f);
                stream.write((const char*)&P[0], sizeof(Vec3d));
                if (doColor) stream.write((const char*)&C[0], sizeof(Vec3ub));
            }
            progress->update( n );
        }
    }

    stream.close();
}

void VRPointCloud::genTestFile2(string path, size_t N_itr, bool doColor) {
    auto sphere = VRGeometry::create("spherePC");
    sphere->setPrimitive("Sphere 1 "+toString(N_itr));
    VRGeoData data(sphere);
    size_t N = data.size();

    map<string, string> params;
    params["N_points"] = toString(N);
    params["format"] = "x8y8z8";
    if (doColor) params["format"] += "r1g1b1";
    params["format"] += "u2v2s1"; // splatting data
    writePCBHeader(path, params);

    auto progress = VRProgress::create();
    progress->setup("generate points ", N);
    progress->reset();

    ofstream stream(path, ios::app);

    Vec3d up(0,1,0);
    Vec3d x(1,0,0);

    cout << "gen PC sphere " << N << endl;

    auto toSpherical = [](Vec3d v) {
        double a = acos(v[1]);
        double b = atan2(v[2],v[0]) + Pi;
        return Vec2ub(255.0*a/Pi, 255.0*b/(2*Pi));
    };

    for (int i=0; i<data.size(); i++) {
        Vec3d P = Vec3d( data.getPosition(i) );
        Vec3ub C = Vec3ub(255*abs(P[0]), 255*abs(P[1]), 255*abs(P[2]));

        Vec3d n = data.getNormal(i);
        Pose O(P, n);
        O.makeUpOrthogonal();
        Vec3d u = O.x();
        Vec3d v = O.up();
        Vec2ub U = toSpherical(u);
        Vec2ub V = toSpherical(v);
        char W = 46; // mm

        stream.write((const char*)&P[0], sizeof(Vec3d));
        if (doColor) stream.write((const char*)&C[0], sizeof(Vec3ub));
        stream.write((const char*)&U[0], sizeof(Vec2ub));
        stream.write((const char*)&V[0], sizeof(Vec2ub));
        stream.write((const char*)&W, sizeof(char));

        progress->update( 1 );
    }

    stream.close();
}

map<string, string> VRPointCloud::readPCBHeader(string path) {
    cout << "readPCBHeader " << path << endl;
    ifstream stream(path);
    string header;
    getline(stream, header);
    auto pairs = splitString(header, '|');

    map<string, string> params;
    for (auto p : pairs) {
        auto param = splitString(p, '$');
        params[param[0]] = param[1];
    }
    size_t pos = stream.tellg();
    params["headerLength"] = toString(pos);

    stream.close();
    return params;
}

void VRPointCloud::writePCBHeader(string path, map<string, string> params) {
    ofstream stream(path);
    int i=0;
    for (auto p : params) {
        if (i > 0) stream << "|";
        stream << p.first << "$" << p.second;
        i++;
    }
    stream << endl;
    stream.close();
}

struct MergeHead {
    size_t pointer = 0;
    bool done = false;
    VRPointCloud::PntCol point;
};

void VRPointCloud::externalSort(string path, size_t chunkSize, double binSize) {
    auto addProgress = [](string head, size_t N) {
        auto progress = VRProgress::create();
        progress->setup(head, N);
        progress->reset();
        return progress;
    };

    auto params = readPCBHeader(path);
    params["binSize"] = toString(binSize);
    cout << "  externalSort headers " << toString(params) << endl;

    auto cN = toValue<size_t>( params["N_points"] );
    cout << "  externalSort scan '" << path << "' contains " << cN << " points" << endl;

    bool hasCol = contains( params["format"], "r");
    if (hasCol) cout << "   scan has colors\n";
    else cout << "   scan has no colors\n";

    int N = sizeof(Vec3d);
    if (hasCol) N += sizeof(Vec3ub);

    size_t Nchunks = cN/chunkSize;
    size_t lastChunkSize = cN - Nchunks*chunkSize;
    cout << "  externalSort Nchunks: " << Nchunks << ", chunkSize: " << chunkSize << ", lastChunkSize: " << lastChunkSize << endl;

    auto calcBin = [&](double& a) {
        static const double eps = 1e-6;
        return round(0.5 + eps + a/binSize);
    };

    auto sameBin = [calcBin](double& a, double& b) {
        return bool( calcBin(a) == calcBin(b) );
        //return bool( abs(b-a)<binSize );
    };

    auto compPoints = [sameBin](PntCol& p1, PntCol& p2) -> bool {
        //return bool(p1.p[2] > p2.p[2]); // test
        if (!sameBin(p1.p[1],p2.p[1])) return bool(p1.p[1] < p2.p[1]);
        if (!sameBin(p1.p[0],p2.p[0])) return bool(p1.p[0] < p2.p[0]);
        return bool(p1.p[2] < p2.p[2]);
    };

    auto createChunks = [&]() {
        ifstream stream(path);
        int hL = toInt(params["headerLength"]);
        stream.seekg(hL);

        vector<PntCol> buffer;
        buffer.resize(chunkSize);
        auto progressChunks = addProgress("process chunk ", Nchunks+1);

        for (size_t i = 0; i<=Nchunks; i++) { // write sorted chunks to disk, +1 for last partial chunk
            if (i == Nchunks) buffer.resize(lastChunkSize);
            if (buffer.size() > 0) {
                for (size_t j = 0; j<buffer.size(); j++) stream.read((char*)&buffer[j], N);
                sort(buffer.begin(), buffer.end(), compPoints);
                string cPath = path+".chunk."+toString(i);
                ofstream cStream(cPath);
                for (size_t j = 0; j<buffer.size(); j++) cStream.write((char*)&buffer[j], N);
                cStream.close();
            }
            progressChunks->update(1);
        }

        stream.close();
    };

    auto mergeChunks = [compPoints, N](size_t levelSize, vector<ifstream>& inStreams, ofstream& wStream, VRProgressPtr progress, bool verbose = false) {
        size_t Nstreams = inStreams.size();
        vector<MergeHead> mergeHeads;
        mergeHeads.resize(Nstreams, MergeHead());

        for (size_t i = 0; i<Nstreams; i++) {
            inStreams[i].read((char*)&mergeHeads[i].point, N); // read first point
            if (!inStreams[i]) mergeHeads[i].done = true; // end of stream reached
            if (verbose) cout << "   start point " << i << ": " << mergeHeads[i].point.p << endl;
        }

        while (true) {
            int kNext = 0;
            while (mergeHeads[kNext].done) kNext++;
            if (kNext >= Nstreams) break; // all done!

            for (size_t j=kNext+1; j<Nstreams; j++) {
                if (mergeHeads[j].done) continue;
                bool b = compPoints(mergeHeads[j].point, mergeHeads[kNext].point);
                if (b) kNext = j;
            }
            if (kNext >= Nstreams) break; // all done!

            wStream.write((char*)&mergeHeads[kNext].point, N);
            progress->update(1);
            mergeHeads[kNext].pointer++;
            if (inStreams[kNext]) inStreams[kNext].read((char*)&mergeHeads[kNext].point, N); // read next point
            if (!inStreams[kNext]) mergeHeads[kNext].done = true; // end of stream reached

            if (verbose) {
                cout << "     write point " << mergeHeads[kNext].pointer << " (stream " << kNext << "/" << Nstreams << ") " << mergeHeads[kNext].point.p;
                cout << " sstate: " << bool(inStreams[kNext]) << " " << int(inStreams[kNext].gcount()) << endl;
            }

            //if (!inStreams[kNext] && inStreams[kNext].gcount() == 0) mergeHeads[kNext].done = true; // end of stream reached
        }
    };

    auto mergeChunksLevel = [&](int lvl, size_t& NlvlChunks, size_t& levelSize, vector<string>& lastChunks) -> bool {
        vector<string> newChunks;
        int Mchunks = min(3,int(lastChunks.size())); // N chunks to merge at once!
        NlvlChunks = lastChunks.size()/Mchunks;
        if (lastChunks.size()%Mchunks > 0) NlvlChunks++;
        bool finalPass = (NlvlChunks == 1);
        cout << " pass " << lvl << ", NlvlChunks: " << NlvlChunks << endl;

        auto progress = addProgress("process level "+toString(lvl)+" ", cN);

        for (size_t j=0; j<NlvlChunks; j++) {
            // prepare in streams
            vector<ifstream> inStreams;
            for (int k=0; k<Mchunks; k++) {
                size_t cI = j*Mchunks+k;
                if (cI < lastChunks.size()) {
                    string cPath = lastChunks[j*Mchunks+k];
                    inStreams.push_back(ifstream(cPath));
                }
            }

            // prepare out stream
            string wPath = path+".lvl"+toString(lvl)+"."+toString(j);
            if (finalPass) wPath = path;//+".sorted.pcb";
            newChunks.push_back(wPath);
            ofstream wStream;
            if (finalPass) {
                cout << "  sort, write final pass to: " << wPath << endl;
                writePCBHeader(wPath, params);
                wStream.open(wPath, std::ios_base::app);
            } else {
                wStream.open(wPath);
            }

            mergeChunks(levelSize, inStreams, wStream, progress, false);
            wStream.close();

            for (int k=0; k<inStreams.size(); k++) {
                string cPath = lastChunks[j*Mchunks+k];
                inStreams[k].close();
                removeFile(cPath);
            }
        }

        levelSize *= Mchunks;
        lastChunks = newChunks;
        return finalPass;
    };

    auto mergeAllChunks = [&]() {
        size_t levelSize = chunkSize;

        size_t NlvlChunks = Nchunks;
        if (lastChunkSize > 0) NlvlChunks++;

        vector<string> lastChunks;
        for (size_t i=0; i<NlvlChunks; i++) lastChunks.push_back( path+".chunk."+toString(i) );

        cout << "mergeAllChunks N chunks: " << lastChunks.size() << ", NlvlChunks: " << NlvlChunks << endl;
        int lvl = 0;
        while (true) {
            if (mergeChunksLevel(lvl, NlvlChunks, levelSize, lastChunks)) break;
            lvl++;
        }
    };

    createChunks();
    mergeAllChunks();
}

string VRPointCloud::splatFP =
"#version 120\n"
GLSL(
varying vec4 Color;
varying vec2 tcoords;

void main( void ) {
	float r = tcoords.x*tcoords.x+tcoords.y*tcoords.y;
	if (r > 1.0) discard;
	gl_FragColor = Color;
}
);

string VRPointCloud::splatVP =
"#version 120\n"
GLSL(
varying vec4 color;
varying vec4 vertex;
varying vec3 normal;
varying vec4 tangentU;
varying vec4 tangentV;
varying mat4 mwp;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;
attribute vec4 osg_Color;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;
attribute vec2 osg_MultiTexCoord2;

uniform float splatModifier;

vec4 vecFromAngles(vec2 ab) {
	ab.x = ab.x/255.0*3.1416;
	ab.y = ab.y/255.0*3.1416*2 + 3.1416;
	return vec4(cos(ab.y)*sin(ab.x), cos(ab.x), sin(ab.y)*sin(ab.x), 0);
}

void main( void ) {
    color = vec4(osg_Color.x/255.0, osg_Color.y/255.0, osg_Color.z/255.0, 1.0);
    normal = osg_Normal.xyz;
    vertex = osg_Vertex;

    vec2 u = osg_MultiTexCoord0;
    vec2 v = osg_MultiTexCoord1;
    float size = osg_MultiTexCoord2.x*splatModifier; // mm
	tangentU = vecFromAngles(u)*size;
	tangentV = vecFromAngles(v)*size;
    mwp = gl_ModelViewProjectionMatrix;
    gl_Position = osg_Vertex;
}
);

string VRPointCloud::splatGP =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=6) out;

uniform vec2 OSGViewportSize;

in vec4 color[];
in vec4 vertex[];
in vec3 normal[];
in vec4 tangentU[];
in vec4 tangentV[];
in mat4 mwp[];
out vec4 Color;
out vec2 tcoords;

void emitVertex(in vec4 p, in vec2 tc) {
	gl_Position = p;
	tcoords = tc;
	EmitVertex();
}

void emitQuad(in float s, in vec4 tc) {
	vec4 p = vertex[0];

	float a = OSGViewportSize.y/OSGViewportSize.x;

	vec4 u = tangentU[0];
	vec4 v = tangentV[0];

	vec4 p1 = mwp[0]*(p -u -v);
	vec4 p2 = mwp[0]*(p -u +v);
	vec4 p3 = mwp[0]*(p +u +v);
	vec4 p4 = mwp[0]*(p +u -v);

	emitVertex(p1, vec2(-1,-1));
	emitVertex(p2, vec2( 1,-1));
	emitVertex(p3, vec2( 1, 1));
	EndPrimitive();
	emitVertex(p1, vec2(-1,-1));
	emitVertex(p3, vec2( 1, 1));
	emitVertex(p4, vec2(-1, 1));
	EndPrimitive();
}

void main() {
	Color = color[0];
	emitQuad(0.13, vec4(0,1,0,1));
}
);
