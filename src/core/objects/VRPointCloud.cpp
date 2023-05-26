#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/VRLod.h"
#include "core/math/partitioning/Octree.h"
#include "core/math/partitioning/OctreeT.h"
#include "core/math/pose.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/import/E57/E57.h"

#define GLSL(shader) #shader

using namespace OSG;
using namespace std;

VRPointCloud::VRPointCloud(string name) : VRTransform(name) {
    octree = Octree<PntData>::create(10);
    mat = VRMaterial::create("pcmat", false);
    type = "PointCloud";
}

VRPointCloud::~VRPointCloud() {}

VRPointCloudPtr VRPointCloud::create(string name) { return VRPointCloudPtr( new VRPointCloud(name) ); }

void VRPointCloud::setupMaterial(bool lit, int pointsize, bool doSplat, float splatModifier) {
    mat->setUseGlobalFCMap(false); // the global material FC map is not threadsafe and its a memory leak!
    mat->setLit(lit);
    mat->setPointSize(pointsize);

    if (doSplat) {
        mat->setVertexShader(splatVP, "splatVP");
        mat->setGeometryShader(splatGP, "splatGP");
        mat->setFragmentShader(splatFP, "splatFP");
        mat->setShaderParameter("splatModifier", splatModifier);
    }
}

VRMaterialPtr VRPointCloud::getMaterial() { return mat; }
shared_ptr<Octree<VRPointCloud::PntData>>& VRPointCloud::getOctree() { return octree; }
VRGeometryPtr VRPointCloud::getOctreeVisual() { return octree->getVisualization(); }

void VRPointCloud::applySettings(map<string, string> options) {
    if (options.count("filePath")) filePath = options["filePath"];
    if (options.count("lit")) lit = toInt(options["lit"]);
    if (options.count("leafSize")) leafSize = toInt(options["leafSize"]);
    if (options.count("pointSize")) pointSize = toInt(options["pointSize"]);
    if (options.count("keepOctree")) keepOctree = toInt(options["keepOctree"]);
    if (options.count("partitionLimit")) partitionLimit = toInt(options["partitionLimit"]);

    bool doSplats = false;
    double splatMod = 1.0;
    if (options.count("doSplats")) doSplats = true;
    if (options.count("splatScale")) { splatScale = toFloat(options["splatScale"]); splatMod *= splatScale; }
    if (options.count("splatMod")) splatMod *= toFloat(options["splatMod"]);
    if (options.count("downsampling")) splatMod /= sqrt(toFloat(options["downsampling"]));

    setupMaterial(lit, pointSize, doSplats, splatMod);
    octree->setResolution(leafSize);
    actualLeafSize = octree->getLeafSize();

    for (auto l : {"lod1", "lod2", "lod3", "lod4", "lod5"}) {
        if (options.count(l)) {
            Vec2d lod;
            toValue(options[l], lod);
            addLevel(lod[0], lod[1]);
        }
    }
}

void VRPointCloud::loadChunk(VRLodPtr lod) {
    VRLock lock(mtx);
    auto prxy = lod->getChild(0);
    if (prxy->getChildrenCount() > 0) return;

    Vec3d c = lod->getCenter();
    double L = actualLeafSize*0.5;

    cout << " - - - - - VRPointCloud::loadChunk " << c << ",   " << L << endl;

    vector<double> region = {c[0]-L,c[0]+L, c[1]-L,c[1]+L, c[2]-L,c[2]+L};
    string path = filePath;

    map<string, string> options;
    options["lit"] = toString(mat->isLit());
    options["pointSize"] = toString(pointSize);
    options["downsampling"] = toString(1.0/downsamplingRate[1]);
    options["leafSize"] = toString(leafSize);
    options["keepOctree"] = toString(0);
    options["region"] = toString( region );
    options["splatScale"] = toString( splatScale );
    options["threaded"] = toString( 1 );
    //cout << " ---------------- VRPointCloud::loadChunk, splatScale: " << splatScale << endl;

    bool threaded = false;
    auto chunk = VRImport::get()->load(path, prxy, false, "OSG", threaded, options);
    prxy->addChild(chunk);
    //cout << " VRPointCloud::onLodSwitch loaded " << chunk->getName() << ", parent: " << chunk->getParent() << ", region: " << toString(region) << endl;
}

void VRPointCloud::onImportEvent(VRImportJob params) {
    VRLock lock(mtx);
    //cout << " ---------------------- VRPointCloud::onImportEvent " << params.path << endl;
    if (!params.res) return;
    auto p = params.res->getParent();
    if (!p) return;
    auto pc = params.res->getChild(0);
    if (!pc) return;
    p->clearLinks();
    params.res->subChild(pc); // workaround for thread issues
    params.res->addChild(pc);
}

void VRPointCloud::onLodSwitch(VRLodEventPtr e) { // for streaming
    //int i0 = e->getLast();
    int i1 = e->getCurrent();
    VRLodPtr lod = e->getLod();

    if (i1 == 0) {
        //cout << "VRPointCloud::onLodSwitch " << lod->getName() << ", load region " << Vec2i(i0, i1) << endl;
        loadChunk(lod);
    }

    if (i1 == 1) {
        //cout << "VRPointCloud::onLodSwitch " << lod->getName() << ", unload region " << Vec2i(i0, i1) << endl;
        VRLock lock(mtx);
        auto prxy = lod->getChild(0);
        prxy->clearChildren();
        prxy->addLink( lod->getChild(1) );
    }
}

void VRPointCloud::addLevel(float distance, int downsampling, bool stream) {
    levels++;
    downsamplingRate.push_back(downsampling);
    lodDistances.push_back(distance);

    if (!onImport) {
        onImport = VRImportCb::create("pc chunk import", bind(&VRPointCloud::onImportEvent, this, placeholders::_1));
        VRImport::get()->addEventCallback(onImport);
    }

    if (lodsSetUp && stream) {
        VRLodCbPtr streamCB = VRLodCb::create( "pointcloudStreamCB", bind(&VRPointCloud::onLodSwitch, this, placeholders::_1 ) );

        for (auto c : getChildren()) {
            auto lod = dynamic_pointer_cast<VRLod>(c);
            if (!lod) continue;

            //loadChunk(lod);
            lod->setCallback(streamCB);
            lod->addDistance(distance);

            auto obj = VRObject::create("pcStreamPrxy");
            auto geo = lod->getChild(0);
            lod->subChild( geo );

            lod->addChild( obj );
            lod->addChild( geo );
            obj->addLink( geo );
        }
    }
}

void VRPointCloud::addPoint(Vec3d p, Splat s) {
    if (pointType == NONE) pointType = SPLAT;
    if (pointType != SPLAT) return;
    PntData d;
    d.c = s.c;
    d.v1 = s.v1;
    d.v2 = s.v2;
    d.w = s.w;
    octree->add(p, d, -1, true, partitionLimit);
}

void VRPointCloud::addPoint(Vec3d p, Color3ub c) {
    if (pointType == NONE) pointType = COLOR;
    if (pointType != COLOR) return;
    PntData d;
    d.c = c;
    octree->add(p, d, -1, true, partitionLimit);
}

void VRPointCloud::setupLODs() {
    lodsSetUp = true;

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
                PntData& data = leaf->getData(i);
                Vec3d pos = leaf->getPoint(i);
                chunk.pushVert(pos - center, Vec3d(0,1,0));
                chunk.pushColor(data.c);
                if (pointType == SPLAT) {
                    chunk.pushTexCoord(Vec2d(data.v1), 0);
                    chunk.pushTexCoord(Vec2d(data.v2), 1);
                    chunk.pushTexCoord(Vec2d(data.w,0), 2);
                }
                chunk.pushPoint();

            }
            if (chunk.size() > 0) chunk.apply( geo );
        }

        if (!keepOctree) leaf->clearContent();
    }

    //addChild(octree->getVisualization());
}

void VRPointCloud::convert(string pathIn, string pathOut) {
    if (pathOut == "") pathOut = pathIn+".pcb";
    convertE57({pathIn}, pathOut);
}

void VRPointCloud::convertMerge(vector<string> pathIn, string pathOut) {
    if (pathIn.size() == 0) return;
    if (pathOut == "") pathOut = pathIn[0]+".pcb";
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

Vec2ub VRPointCloud::toSpherical(const Vec3d& v) {
    double a = acos(v[1]);
    double b = atan2(v[2],v[0]) + Pi;
    return Vec2ub(255.0*a/Pi, 255.0*b/(2*Pi));
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

VRProgressPtr VRPointCloud::addProgress(string head, size_t N) {
    auto progress = VRProgress::create();
    progress->setup(head, N);
    progress->reset();
    return progress;
}

void VRPointCloud::externalPartition(string path) {
    /**
    1) create an octree with resolution binSize
    2) put all points in that octree without storing them inside
    3) for each leaf create a file and put the points inside
    5) serialize the octree and write it into a file
    6) append each bin file to the octree file
    */

    auto params = readPCBHeader(path);
    cout << "  externalPartition headers " << toString(params) << endl;
    if (!params.count("binSize")) { cout << "  externalPartition needs a sorted PCB, please run externalSort on it first!" << endl; return; }

    bool hasCol = contains( params["format"], "r");
    auto cN = toValue<size_t>( params["N_points"] );

    int N1 = sizeof(Vec3d);
    if (hasCol) N1 += sizeof(Vec3ub);

    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);

    auto progress = addProgress("process points ", cN);

    float binSize = toFloat(params["binSize"]);
    auto oc = Octree<bool>::create(binSize);

    map<void*, ocChunkRef> chunkRefs;

    size_t Nwritten1 = 0;
    VRPointCloud::Splat splat;
    for (size_t i = 0; i<cN; i++) {
        stream.read((char*)&splat, N1);
        if (splat.p.length() < 1e-6) cout << " A GOT P0 " << splat.p << endl;
        auto node = oc->add( splat.p, 0 );
        node->clearContent();
        progress->update(1);

        void* key = node;
        if (!chunkRefs.count(key)) {
            string path = "ocChnk"+toString(key)+".dat";
            chunkRefs[key].path = path;
            chunkRefs[key].stream.open( path );
            if (!chunkRefs[key].stream) {
                cout << " ERROR! cannot create more files to store chunks: " << chunkRefs.size() << endl;
                break;
            }
        }

        chunkRefs[key].size += 1;
        chunkRefs[key].stream.write((char*)&splat, N1);
        Nwritten1 += 1;
    }
    cout << " written N in chunks: " << Nwritten1 << endl;
    stream.close();

    for (auto& ref : chunkRefs) ref.second.stream.close();

    size_t ocNodeBinSize = sizeof(ocSerialNode);
    string wpath = path+".tmp.pcb";
    params["partitionStructure"] = "octree";
    params["ocRootSize"] = toString(oc->getRoot()->size);
    params["ocRootCenter"] = toString(oc->getRoot()->center);
    params["ocNodeCount"] = toString(oc->getNodesCount());
    writePCBHeader(wpath, params);
    ofstream wstream(wpath, ios::app);

    // compute/predict the binary offsets of chunks and write them into the refs
    size_t chunksOffset = size_t(wstream.tellp()) + ocNodeBinSize * oc->getNodesCount();
    for (auto& ref : chunkRefs) {
        ref.second.offset = chunksOffset;
        chunksOffset += ref.second.size * N1;
    }

    function<void(OctreeNode<bool>*, ofstream&, int&)> writeOutOcNode = [&](OctreeNode<bool>* node, ofstream& wstream, int& nOffset) {
        ocSerialNode sNode;
        for (int i=0; i<8; i++) {
            auto c = node->getChild(i);
            if (c) {
                writeOutOcNode(c, wstream, nOffset);
                sNode.children[i] = nOffset - ocNodeBinSize;
            }
        }
        void* key = node;
        if (chunkRefs.count(key)) {
            sNode.chunkOffset = chunkRefs[key].offset;
            sNode.chunkSize = chunkRefs[key].size;
        }

        //sNode.size = node->getSize(); // TEMP
        //sNode.center = node->getCenter(); // TEMP
        wstream.write((char*)&sNode, ocNodeBinSize);
        nOffset +=ocNodeBinSize;
    };

    int nOffset = 0;
    writeOutOcNode(oc->getRoot(), wstream, nOffset);

    cout << "---- start writing points at " << wstream.tellp() << endl;
    size_t Nwritten = 0;
    for (auto& ref : chunkRefs) {
        VRPointCloud::Splat splat;
        ifstream stream(ref.second.path);
        while(stream.read((char*)&splat, N1)) {
            if (splat.p.length() < 1e-6) cout << " B GOT P0 " << splat.p << endl;
            wstream.write((char*)&splat, N1);
            Nwritten += 1;
        }

        removeFile(ref.second.path);
    }
    cout << "---- writing points " << Nwritten << endl;

    wstream.close();
    rename(wpath.c_str(), path.c_str());
}

void VRPointCloud::externalSort(string path, size_t chunkSize, double binSize) {
    auto params = readPCBHeader(path);
    params["binSize"] = toString(binSize);
    cout << "  externalSort headers " << toString(params) << endl;

    bool hasCol = contains( params["format"], "r");
    auto cN = toValue<size_t>( params["N_points"] );
    cout << "  externalSort scan '" << path << "' contains " << cN << " points" << endl;

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
            size_t kNext = 0;
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
                wStream.open(wPath, ios::app);
            } else {
                wStream.open(wPath);
            }

            mergeChunks(levelSize, inStreams, wStream, progress, false);
            wStream.close();

            for (size_t k=0; k<inStreams.size(); k++) {
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

void VRPointCloud::externalComputeSplats(string path) {
    auto params = readPCBHeader(path);
    cout << "  externalComputeSplats headers " << toString(params) << endl;
    if (!params.count("binSize")) { cout << "  externalPartition needs a sorted PCB, please run externalSort on it first!" << endl; return; }

    bool hasCol = contains( params["format"], "r");
    auto cN = toValue<size_t>( params["N_points"] );

    auto progress = addProgress("process splats ", cN);

    int N1 = sizeof(Vec3d);
    if (hasCol) N1 += sizeof(Vec3ub);

    string wpath = path+".tmp.pcb";
    params["format"] = "x8y8z8r1g1b1u2v2s1";
    params["splatMod"] = "0.001";
    writePCBHeader(wpath, params);
    ofstream wstream(wpath, ios::app);

    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);

    double threshhold = 0.1;
    size_t Nn = 6; // number of neighbors to get
    int Nb = 100; // half of buffer size
    vector<VRPointCloud::Splat> buffer;
    buffer.resize(Nb*2+1);
    double meanSplatSize = threshhold;
    size_t meanCount = 0;

    auto writeSplat = [&](VRPointCloud::Splat& splat, Vec3d n, char W) {
        Pose O(splat.p, n);
        O.makeUpOrthogonal();
        Vec3d u = O.x();
        Vec3d v = O.up();
        Vec2ub U = toSpherical(u);
        Vec2ub V = toSpherical(v);

        wstream.write((const char*)&splat.p[0], sizeof(Vec3d));
        wstream.write((const char*)&splat.c[0], sizeof(Vec3ub));
        wstream.write((const char*)&U[0], sizeof(Vec2ub));
        wstream.write((const char*)&V[0], sizeof(Vec2ub));
        wstream.write((const char*)&W, sizeof(char));
    };

    auto getNeighbors = [&](size_t i, size_t n) {
        Vec3d p0 = buffer[i].p;

        map<int, double> pnts;
        for (size_t j=0; j<buffer.size(); j++) {
            if (i == j) continue;

            double d = p0.dist(buffer[j].p);

            if (d > threshhold) continue;

            if (pnts.size() < n) {
                pnts[j] = d;
                continue;
            }

            int kmax = -1;
            for (auto p : pnts) if (kmax == -1 || p.second > pnts[kmax]) kmax = p.first;

            if (d < pnts[kmax]) {
                pnts.erase(kmax);
                pnts[j] = d;
            }
        }

        vector<int> res;
        for (auto p : pnts) res.push_back(p.first);
        return res;
    };

    auto computeNormal = [&](int i, vector<int>& neighbors) {
        Vec3d p0 = buffer[i].p;
        Vec3d dl;
        vector<Vec3d> normals;
        for (size_t j=0; j<neighbors.size(); j++) {
            int ni = neighbors[j];
            Vec3d dj = buffer[ni].p - p0;
            if (j > 0) {
                Vec3d n = dl.cross(dj);
                n.normalize();
                normals.push_back(n);
            }
            dl = dj;
        }

        for (size_t j=1; j<normals.size(); j++) {
            if (normals[j-1].dot(normals[j]) < 0) normals[j] *= -1;
        }

        Vec3d res;
        for (auto& n : normals) res += n;
        res.normalize();
        return res;
    };

    auto computeSize = [&](int i, vector<int>& neighbors) -> char {
        if (neighbors.size() < Nn) return 1; // 1 mm

        Vec3d p0 = buffer[i].p;

        double d = threshhold;
        for (size_t j=0; j<neighbors.size(); j++) {
            int ni = neighbors[j];
            Vec3d dj = buffer[ni].p - p0;
            if (dj.length() < d) d = dj.length();
        }
        d *= 1.2;

        // catch outliers
        if (meanCount == 0) meanSplatSize = d;
        else meanSplatSize = (meanSplatSize*meanCount+d)/(meanCount+1);
        meanCount++;
        if (d > meanSplatSize*2) d = meanSplatSize;

        d *= 1000.0; // in mm
        char c = min(int(d), 255);
        return c;
    };

    size_t wi = 0;
    for (size_t i = 0; i<cN; i++) {
        stream.read((char*)&buffer[i%buffer.size()], N1);

        if (i >= buffer.size()-1 || i+1 == cN) { // buffer full
            while (wi <= i) {
                int Wi = wi%buffer.size();
                VRPointCloud::Splat& splat = buffer[Wi];
                vector<int> neighbors = getNeighbors(Wi, Nn);
                Vec3d n = computeNormal(Wi, neighbors);
                char W = computeSize(Wi, neighbors);
                writeSplat(splat, n, W);
                wi++;
                progress->update(1);
            }
        }
    }

    stream.close();
    wstream.close();
    rename(wpath.c_str(), path.c_str());
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
