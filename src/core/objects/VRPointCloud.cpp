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

using namespace OSG;
using namespace std;

VRPointCloud::VRPointCloud(string name) : VRTransform(name) {
    octree = Octree::create(10);
    mat = VRMaterial::create("pcmat");
    type = "PointCloud";
}

VRPointCloud::~VRPointCloud() {}

VRPointCloudPtr VRPointCloud::create(string name) { return VRPointCloudPtr( new VRPointCloud(name) ); }

void VRPointCloud::setupMaterial(bool lit, int pointsize) {
    mat->setLit(lit);
    mat->setPointSize(pointsize);

    // TODO: add splatting option
    //  - compute tangent space at each point
    //  - compute density at each point
    //  - render patches for each point, needs tangents and size
    //  - render disks on the quads?
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
                Color3ub col = *((Color3ub*)data);
                chunk.pushVert(pos - center, Vec3d(0,1,0));
                chunk.pushColor(col);
                chunk.pushPoint();

            }
            if (chunk.size() > 0) chunk.apply( geo );
        }

        if (!keepOctree) leaf->delContent<Color3ub>();
    }

    //addChild(octree->getVisualization());
}

void VRPointCloud::convert(string pathIn) {
    string pathOut = pathIn+".pcb";
    convertE57(pathIn, pathOut);
}

void VRPointCloud::genTestFile(string path, size_t N, bool doColor) {
    genTestPC(path, N, doColor);
}

void VRPointCloud::genTestFile2(string path, size_t N, bool doColor) {
    ofstream stream(path);
    stream << "x8y8z8";
    if (doColor) stream << "r1g1b1";
    stream << "u2v2s1"; // splatting data
    stream << "\n" << toString(N) << "\n0\n";

    auto sphere = VRGeometry::create("spherePC");
    sphere->setPrimitive("Sphere 1 "+toString(N));
    VRGeoData data(sphere);

    auto progress = VRProgress::create();
    progress->setup("generate points ", data.size());
    progress->reset();

    Vec3d up(0,1,0);
    Vec3d x(1,0,0);

    cout << "gen PC sphere " << data.size() << endl;

    for (int i=0; i<data.size(); i++) {
        Vec3d P = Vec3d( data.getPosition(i) );
        Vec3ub C = Vec3ub(255*abs(P[0]), 255*abs(P[1]), 255*abs(P[2]));

        Vec3d n = data.getNormal(i);
        Pose O(P, n);
        O.makeUpOrthogonal();
        Vec3d u = O.x();
        Vec3d v = O.up();
        Vec2ub U(255*u.dot(up), 255*u.dot(x));
        Vec2ub V(255*u.dot(up), 255*u.dot(x));
        char W = 10; // mm

        stream.write((const char*)&P[0], sizeof(Vec3d));
        if (doColor) stream.write((const char*)&C[0], sizeof(Vec3ub));
        stream.write((const char*)&U[0], sizeof(Vec2ub));
        stream.write((const char*)&V[0], sizeof(Vec2ub));
        stream.write((const char*)&W, sizeof(char));

        progress->update( 1 );
    }

    stream.close();
}

void VRPointCloud::externalSort(string path, size_t NchunkMax, double binSize) {
    ifstream stream(path);
    string h1, h2, h3;
    getline(stream, h1);
    getline(stream, h2);
    getline(stream, h3);
    cout << "  externalSort headers " << h1 << " " << h2 << endl;

    auto cN = toValue<size_t>(h2);
    cout << "  externalSort scan '" << path << "' contains " << cN << " points" << endl;

    auto progress = VRProgress::create();
    progress->setup("process points ", cN);
    progress->reset();

    bool hasCol = contains(h1, "r");
    if (hasCol) cout << "   scan has colors\n";
    else cout << "   scan has no colors\n";

    int N = sizeof(Vec3d);
    if (hasCol) N += sizeof(Vec3ub);

    int Mchunks = 3; // chunks to merge at once!

    size_t Nchunks = 1;
    while(cN / Nchunks > NchunkMax) Nchunks *= Mchunks;

    size_t chunkSize = cN / Nchunks;
    size_t lastChunkSize = cN - Nchunks*chunkSize;
    cN -= lastChunkSize; // TODO: for now we crop the PC
    h2 = toString(cN);

    cout << "  externalSort Nchunks: " << Nchunks << ", with Mchunks: " << Mchunks << endl;
    cout << "  externalSort chunkSize: " << chunkSize << ", lastChunkSize: " << lastChunkSize << endl;

    auto calcBin = [&](double& a) {
        static const double eps = 1e-6;
        return round(0.5 + eps + a/binSize);
    };

    auto sameBin = [&](double& a, double& b) {
        return bool( calcBin(a) == calcBin(b) );
        //return bool( abs(b-a)<binSize );
    };

    auto compPoints = [&](PntCol& p1, PntCol& p2) -> bool {
        //return bool(p1.p[2] > p2.p[2]); // test
        if (!sameBin(p1.p[1],p2.p[1])) return bool(p1.p[1] < p2.p[1]);
        if (!sameBin(p1.p[0],p2.p[0])) return bool(p1.p[0] < p2.p[0]);
        return bool(p1.p[2] < p2.p[2]);
    };

    vector<PntCol> buffer;
    buffer.resize(chunkSize);

    for (size_t i = 0; i<Nchunks; i++) { // write sorted chunks to disk
        for (size_t j = 0; j<chunkSize; j++) stream.read((char*)&buffer[j], N);
        //for (size_t j = 0; j<chunkSize; j++) cout << "  read chunk pnt " << j << ") " << buffer[j].p << endl;
        sort(buffer.begin(), buffer.end(), compPoints);
        //for (size_t j = 0; j<chunkSize; j++) cout << "  sort chunk pnt " << j << ") " << buffer[j].p << endl;
        string cPath = path+".chunk."+toString(i);
        ofstream cStream(cPath);
        for (size_t j = 0; j<chunkSize; j++) cStream.write((char*)&buffer[j], N);
        cStream.close();
    }

    stream.close();

    auto mergeChunks = [&](size_t L, vector<ifstream>& inStreams, ofstream& wStream, bool verbose = false) {
        vector<size_t> mergeHeads;
        vector<PntCol> mergeHeadData;
        mergeHeads.resize(inStreams.size(), 0);
        mergeHeadData.resize(inStreams.size(), PntCol());

        for (size_t i = 0; i<inStreams.size(); i++) {
            inStreams[i].read((char*)&mergeHeadData[i], N);
            //if (verbose) cout << "   start point " << i << ": " << mergeHeadData[i].p << endl;
        }

        for (size_t i = 0; i<L*3; i++) {
            int kNext = 0;
            while(mergeHeads[kNext] == L) kNext++;
            for (size_t j=kNext+1; j<inStreams.size(); j++) {
                if (mergeHeads[j] == L) continue;
                bool b = compPoints(mergeHeadData[j], mergeHeadData[kNext]);
                //if (verbose) cout << "    compare [" << mergeHeadData[j].p << "] (" << j << ") with [" << mergeHeadData[kNext].p << "] (" << kNext << ") -> " << b << endl;
                if (b) kNext = j;
            }

            //if (verbose) cout << "     write point " << kNext << ") " << mergeHeadData[kNext].p << endl;
            wStream.write((char*)&mergeHeadData[kNext], N);
            mergeHeads[kNext]++;
            inStreams[kNext].read((char*)&mergeHeadData[kNext], N);
        }
    };

    size_t NlvlChunks = Nchunks;
    size_t Lchunks = chunkSize;
    vector<string> currentChunks;
    vector<string> lastChunks;
    for (int i=0; i<Nchunks; i++) lastChunks.push_back( path+".chunk."+toString(i) );

    //cout << "start merge" << endl;
    for (int i=0; NlvlChunks>1; i++) {
        NlvlChunks /= Mchunks;
        //cout << " pass " << i << ", NlvlChunks: " << NlvlChunks << endl;
        for (int j=0; j<NlvlChunks; j++) {
            bool finalPass = (NlvlChunks == 1);

            vector<ifstream> inStreams;
            for (int k=0; k<Mchunks; k++) {
                string cPath = lastChunks[j*Mchunks+k];
                //cout << " > open input stream " << cPath << endl;
                inStreams.push_back(ifstream(cPath));
            }

            string wPath = path;
            if (!finalPass) wPath = path+".lvl"+toString(i)+"."+toString(j);
            currentChunks.push_back(wPath);

            //cout << "  > open output stream " << wPath << endl;
            ofstream wStream(wPath);
            if (finalPass) {
                cout << "  sort, write final pass!! binSize: " << binSize << endl;
                wStream << h1 << "\n" << h2 << "\n" << toString(binSize) << "\n";
            }

            //cout << "    merge to " << j << endl;
            mergeChunks(Lchunks, inStreams, wStream, i == 2);
            //cout << "  < close output stream " << wPath << endl;
            wStream.close();

            for (int k=0; k<Mchunks; k++) {
                string cPath = lastChunks[j*Mchunks+k];
                //cout << " < close input stream " << cPath << endl;
                inStreams[k].close();
            }
        }

        for (auto& p : lastChunks) removeFile(p);
        lastChunks = currentChunks;
        currentChunks.clear();
        Lchunks *= Mchunks;
    }
}





