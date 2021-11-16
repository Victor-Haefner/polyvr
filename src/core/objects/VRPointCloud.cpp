#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/VRLod.h"
#include "core/math/partitioning/Octree.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
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

void VRPointCloud::addPoint(Vec3d p, Color3f c) {
    octree->add(p, new Color3f(c), -1, true, 1e5);
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
                Color3f col = *((Color3f*)data);
                chunk.pushVert(pos - center, Vec3d(0,1,0), col);
                chunk.pushPoint();

            }
            if (chunk.size() > 0) chunk.apply( geo );
        }

        if (!keepOctree) leaf->delContent<Color3f>();
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

void sortChunk(string& buffer) {
    ;
}

struct PntCol {
    Vec3d p;
    Vec3ub c;
};

void VRPointCloud::externalSort(string path, size_t NchunkMax, double binSize) {
    ifstream stream(path);
    string h1, h2;
    getline(stream, h1);
    getline(stream, h2);
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

    auto sameBin = [&](double& a, double& b){
        return bool( abs(b-a)<binSize );
    };

    auto compPoints = [&](PntCol& p1, PntCol& p2) -> bool {
        return bool(p1.p[2] > p2.p[2]); // test

        if (!sameBin(p1.p[1],p2.p[1])) return bool(p1.p[1] < p2.p[1]);
        if (!sameBin(p1.p[0],p2.p[0])) return bool(p1.p[0] < p2.p[0]);
        return bool(p1.p[2] < p2.p[2]);
    };

    vector<PntCol> buffer;
    buffer.resize(chunkSize);

    for (size_t i = 0; i<Nchunks; i++) { // write sorted chunks to disk
        for (size_t j = 0; j<chunkSize; j++) stream.read((char*)&buffer[j], N);
        sort(buffer.begin(), buffer.end(), compPoints);
        string cPath = path+".chunk."+toString(i);
        ofstream cStream(cPath);
        for (size_t j = 0; j<chunkSize; j++) cStream.write((char*)&buffer[j], N);
        cStream.close();
    }

    stream.close();

    auto mergeChunks = [&](int lvl, int ID, vector<string>& currentChunks, const vector<string>& lastChunks) {
        cout << " lvl " << lvl << " ID " << ID << endl;
        string wPath = path+".lvl"+toString(lvl)+"."+toString(ID);
        currentChunks.push_back(wPath);
        /*ofstream wStream(wPath);
        for (size_t i = 0; i<Nchunks; i++) {
            string cPath = path+".chunk."+toString(i);
            ifstream cStream(cPath);
            for (size_t j = 0; j<chunkSize; j++) cStream.read((char*)&buffer[j], N);
            if (i == 0) for (auto b : buffer) cout << i << ") " << b.p << endl;
            cStream.close();
            for (size_t j = 0; j<chunkSize; j++) wStream.write((char*)&buffer[j], N);
        }
        wStream.close();*/
    };

    size_t NlvlChunks = Nchunks;
    vector<string> currentChunks;
    vector<string> lastChunks;
    for (int i=0; NlvlChunks>1; i++) {
        NlvlChunks /= Mchunks;
        for (int j=0; j<NlvlChunks; j++) {
            if (NlvlChunks > 1) mergeChunks(i, j, currentChunks, lastChunks);
            else { // final file
                ofstream wStream(path);
                wStream << h1 << "\n" << h2 << "\n";
                for (size_t i = 0; i<Nchunks; i++) {
                    string cPath = path+".chunk."+toString(i);
                    ifstream cStream(cPath);
                    for (size_t j = 0; j<chunkSize; j++) cStream.read((char*)&buffer[j], N);
                    if (i == 0) for (auto b : buffer) cout << i << ") " << b.p << endl;
                    cStream.close();
                    for (size_t j = 0; j<chunkSize; j++) wStream.write((char*)&buffer[j], N);
                }
                wStream.close();
            }
        }
        lastChunks = currentChunks;
    }

}





