#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRPrimitive.h"
#include "core/objects/VRLod.h"
#include "core/math/partitioning/Octree.h"
#include "core/math/partitioning/OctreeT.h"
#include "core/math/partitioning/Quadtree.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/math/PCA.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/tools/VRPathtool.h"
#include "core/scene/import/VRImport.h"
#include "core/scene/import/E57/E57.h"
#include "addons/WorldGenerator/terrain/VRPlanet.h"

#define GLSL(shader) #shader

using namespace OSG;
using namespace std;

template<> string typeName(const VRPointCloud::Splat* o) { return "VRPointCloud::Splat"; }

template<> int toValue(stringstream& ss, VRPointCloud::Splat& e) {
    string s = ss.str();
    // TODO;
    return false;
}

VRExternalPointCloud::VRExternalPointCloud(string path) : path(path) {
    params = readPCBHeader(path);
    if (params.size() > 0) valid = true;

    hasColors = contains( params["format"], "r");
    hasSplats = contains( params["format"], "u");
    isSorted = bool(params.count("binSize"));
    hasOctree = bool(params.count("partitionStructure") && params["partitionStructure"] == "octree");

    size = toValue<size_t>( params["N_points"] );
    headerLength = toInt(params["headerLength"]);

    binPntsStart = headerLength;
    if (hasOctree) {
        size_t Nnodes = toInt( params["ocNodeCount"] );
        size_t ocNodeBinSize = sizeof(OcSerialNode);
        binPntsStart += Nnodes * ocNodeBinSize;
        //cout << " ... octree params: " << headerLength << ", " << Nnodes << ", " << ocNodeBinSize << ", " << Nnodes * ocNodeBinSize << ", " << binPntsStart << endl;
    }

    binPntSize = sizeof(Vec3d);
    if (hasColors) binPntSize = VRPointCloud::PntCol::size;
    if (hasSplats) binPntSize = VRPointCloud::Splat::size;
}

VRExternalPointCloud::~VRExternalPointCloud() {}

map<string, string> VRExternalPointCloud::readPCBHeader(string path) {
    //cout << "readPCBHeader " << path << endl;
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

void VRExternalPointCloud::writePCBHeader(string wpath, map<string, string> params) {
    ofstream wstream(wpath);
    int i=0;
    for (auto p : params) {
        if (i > 0) wstream << "|";
        wstream << p.first << "$" << p.second;
        i++;
    }
    wstream << endl;
    wstream.close();
}

void VRExternalPointCloud::copyPCBOctree(string wpath, const VRExternalPointCloud& epc) {
    if (!epc.hasOctree) return;

    ifstream stream(epc.path);
    stream.seekg(epc.headerLength);
    size_t N = epc.binPntsStart - epc.headerLength;
    vector<char> buffer(N);
    stream.read(&buffer[0], N);
    stream.close();

    ofstream wstream(wpath, ios::app);
    wstream.write(&buffer[0], N);
    wstream.close();
}

void VRExternalPointCloud::printOctree() {
    if (!hasOctree) {
        cout << "Error in printOctree: no octree!" << endl;
        return;
    }

    double rootSize = toValue<double>(params["ocRootSize"]);
    Vec3d rootCenter = toValue<Vec3d>(params["ocRootCenter"]);
    size_t nodeCount = toValue<size_t>(params["ocNodeCount"]);
    cout << "Octree, root size: " << rootSize << ", root center" << rootCenter << ", node count: " << nodeCount << endl;

    // get correct octree node based on region
    size_t ocNodeBinSize = sizeof(OcSerialNode);
    size_t rOffset = (nodeCount-1) * ocNodeBinSize; // root is last node written
    OcSerialNode ocNode;

    ifstream stream(path);
    if (!stream.is_open()) { cout << "ERROR! could not open " << path << endl; return; }
    stream.seekg(headerLength);
    if (stream.fail()) { cout << "ERROR! could not seek to " << headerLength << endl; return; }
    size_t ocTree = stream.tellg();

    stream.seekg(ocTree + rOffset, ios::beg);
    cout << "get root at " << stream.tellg() << ", " << headerLength << endl;
    stream.read((char*)&ocNode, ocNodeBinSize); // read tree root


    auto printNode = [&](OcSerialNode node, string indent) {
        cout << indent << " node: " << node.chunkOffset << ", " << node.chunkSize << endl;
    };

    function<void(OcSerialNode, string)> iterateNode = [&](OcSerialNode node, string indent) {
        printNode(node, indent);

        for (int i=0; i<8; i++) {
            int cOffset = node.children[i];
            if (cOffset != 0) {
                stream.seekg(ocTree + cOffset, ios::beg);
                OcSerialNode child;
                stream.read((char*)&child, ocNodeBinSize);
                iterateNode(child, indent+" ");
            }
        }
    };

    cout << "Print octree structure:" << endl;
    iterateNode(ocNode, "");
    cout << "Print octree nodes list, oc:" << ocTree << " bs: " << ocNodeBinSize << endl;
    stream.seekg(ocTree, ios::beg);
    for (int i=0; i<nodeCount; i++) {
        OcSerialNode node;
        stream.read((char*)&node, ocNodeBinSize);
        cout << stream.tellg() << ": ";
        printNode(node, "");
    }
}

VRExternalPointCloud::OcSerialNode VRExternalPointCloud::getOctreeNode(Vec3d p) {
    if (!hasOctree) {
        cout << "Error in getOctreeNode: no octree!" << endl;
        return OcSerialNode();
    }

    double rootSize = toValue<double>(params["ocRootSize"]);
    Vec3d rootCenter = toValue<Vec3d>(params["ocRootCenter"]);
    size_t nodeCount = toValue<size_t>(params["ocNodeCount"]);

    // get correct octree node based on region
    size_t ocNodeBinSize = sizeof(OcSerialNode);
    size_t rOffset = (nodeCount-1) * ocNodeBinSize; // root is last node written
    OcSerialNode ocNode;

    ifstream stream(path);
    stream.seekg(headerLength);
    size_t ocTree = stream.tellg();

    stream.seekg(ocTree + rOffset, ios::beg);
    stream.read((char*)&ocNode, ocNodeBinSize); // read tree root

    int jumps = 0;
    double nodeSize = rootSize;
    Vec3d nodeCenter = rootCenter; // root is on 0, 0, 0
    while (true) { // traverse tree
        // compute child octant
        //Vec3d rp = nodeCenter - p;
        Vec3d rp = p - nodeCenter;
        int octant = 0;
        if (rp[0] < 0) octant += 1;
        if (rp[1] < 0) octant += 2;
        if (rp[2] < 0) octant += 4;

        // get child address
        int cOffset = ocNode.children[octant];
        if (cOffset == 0) break;

        // compute new node size and center
        nodeSize *= 0.5;
        Vec3d c = Vec3d(nodeSize, nodeSize, nodeSize)*0.5;
        if (rp[0] < 0) c[0] -= nodeSize;
        if (rp[1] < 0) c[1] -= nodeSize;
        if (rp[2] < 0) c[2] -= nodeSize;
        nodeCenter += c;
        jumps++;

        // get child node
        stream.seekg(ocTree + cOffset, ios::beg);
        //cout << " get child from " << ocTree + cOffset << ", " << ocNode.chunkOffset << ", " << ocNode.chunkSize << endl;
        stream.read((char*)&ocNode, ocNodeBinSize);
    }

    return ocNode;
}



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
    if (options.count("geoLocationN")) geoLocationN = toFloat(options["geoLocationN"]);
    if (options.count("geoLocationE")) geoLocationE = toFloat(options["geoLocationE"]);

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
            cout << " .. add lod stream " << l << ", " << lod << endl;
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
    cout << " VRPointCloud::setupLODs" << endl;
    lodsSetUp = true;

    auto leafs = octree->getAllLeafs();

    auto progress = VRProgress::create();
    progress->setup("setup pointcloud LODs ", leafs.size());
    progress->reset();

    for (auto leaf : leafs) {
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
        progress->update(1);
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
    VRExternalPointCloud::writePCBHeader(path, params);

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

void VRPointCloud::genTestFile2(string path, size_t N_itr, bool doColor, int splatSize) {
    auto sphere = VRGeometry::create("spherePC");
    sphere->setPrimitive("Sphere 1 "+toString(N_itr));
    VRGeoData data(sphere);
    size_t N = data.size();

    map<string, string> params;
    params["N_points"] = toString(N);
    params["format"] = "x8y8z8";
    if (doColor) params["format"] += "r1g1b1";
    params["format"] += "u2v2s1"; // splatting data
    VRExternalPointCloud::writePCBHeader(path, params);

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
        Vec2ub U = toSpherical(O.x());
        Vec2ub V = toSpherical(O.up());
        char W = splatSize; // mm

        stream.write((const char*)&P[0], sizeof(Vec3d));
        if (doColor) stream.write((const char*)&C[0], sizeof(Vec3ub));
        stream.write((const char*)&U[0], sizeof(Vec2ub));
        stream.write((const char*)&V[0], sizeof(Vec2ub));
        stream.write((const char*)&W, sizeof(char));

        progress->update( 1 );
    }

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

    VRExternalPointCloud epc(path);
    cout << "  externalPartition headers " << toString(epc.params) << endl;
    if (!epc.isSorted) { cout << "  externalPartition needs a sorted PCB, please run externalSort on it first!" << endl; return; }

    ifstream stream(path);
    stream.seekg(epc.binPntsStart);
    auto progress = addProgress("process points ", epc.size);

    float binSize = toFloat(epc.params["binSize"]);
    auto oc = Octree<bool>::create(binSize);

    map<void*, VRExternalPointCloud::OcChunkRef> chunkRefs;

    size_t openStreams = 0;
    size_t Nwritten1 = 0;
    VRPointCloud::Splat splat;
    for (size_t i = 0; i<epc.size; i++) {
        stream.read((char*)&splat, epc.binPntSize);
        //if (splat.p.length() < 1e-6) cout << " A GOT P0 " << splat.p << endl;
        auto node = oc->add( splat.p, 0 );
        node->clearContent();
        progress->update(1);

        void* key = node;
        if (!chunkRefs.count(key)) { // intial open
            string path = "ocChnk"+toString(key)+".dat";
            chunkRefs[key].path = path;
            chunkRefs[key].stream.open( chunkRefs[key].path );
            if (!chunkRefs[key].stream) {
                cout << " ERROR1! cannot create more files to store chunks: " << openStreams << " / " << chunkRefs.size() << " / " << epc.size << ", err: " << strerror(errno) << endl;
                for (auto& ref : chunkRefs) removeFile(ref.second.path);
                return;
            }
            chunkRefs[key].isOpen = true;
            openStreams++;
        }

        if (!chunkRefs[key].isOpen) { // reopen if necessary
            chunkRefs[key].stream.open( chunkRefs[key].path, ios::app );
            if (!chunkRefs[key].stream) {
                cout << " ERROR2! cannot create more files to store chunks: " << openStreams << " / " << chunkRefs.size() << " / " << epc.size << ", err: " << strerror(errno) << endl;
                for (auto& ref : chunkRefs) removeFile(ref.second.path);
                return;
            }
            chunkRefs[key].isOpen = true;
            openStreams++;
        }

        chunkRefs[key].size += 1;
        chunkRefs[key].stream.write((char*)&splat, epc.binPntSize);
        Nwritten1 += 1;

        if (openStreams > 200) { // close streams if too many open to avoid system issues
            for (auto& ref : chunkRefs) {
                if (ref.second.isOpen) {
                    ref.second.stream.close();
                    ref.second.isOpen = false;
                    openStreams--;
                    if (openStreams < 50) break;
                }
            }
        }
    }
    cout << " written " << Nwritten1 << " points in " << chunkRefs.size() << " chunks" << endl;
    stream.close();

    for (auto& ref : chunkRefs) if (ref.second.isOpen) { ref.second.stream.close(); ref.second.isOpen = false; openStreams--; }

    size_t ocNodeBinSize = sizeof(VRExternalPointCloud::OcSerialNode);
    string wpath = path+".tmp.pcb";
    epc.params["partitionStructure"] = "octree";
    epc.params["ocRootSize"] = toString(oc->getRoot()->size);
    epc.params["ocRootCenter"] = toString(oc->getRoot()->center);
    epc.params["ocNodeCount"] = toString(oc->getNodesCount());
    VRExternalPointCloud::writePCBHeader(wpath, epc.params);
    ofstream wstream(wpath, ios::app);

    // compute/predict the binary offsets of chunks and write them into the refs
    size_t chunksOffset = size_t(wstream.tellp()) + ocNodeBinSize * oc->getNodesCount();
    for (auto& ref : chunkRefs) {
        ref.second.offset = chunksOffset;
        chunksOffset += ref.second.size * epc.binPntSize;
    }

    function<void(OctreeNode<bool>*, ofstream&, int&)> writeOutOcNode = [&](OctreeNode<bool>* node, ofstream& wstream, int& nOffset) {
        VRExternalPointCloud::OcSerialNode sNode;
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
        //cout << " writeOutOcNode " << sNode.chunkOffset << ", " << sNode.chunkSize << ", " << toString(vector<int>(sNode.children,sNode.children+8)) << endl;
        wstream.write((char*)&sNode, ocNodeBinSize);
        nOffset += ocNodeBinSize;
    };

    int nOffset = 0;
    writeOutOcNode(oc->getRoot(), wstream, nOffset);

    cout << "---- start writing points at " << wstream.tellp() << ", nOffset " << nOffset << endl;
    size_t Nwritten = 0;
    for (auto& ref : chunkRefs) {
        VRPointCloud::Splat splat;
        ifstream stream(ref.second.path);
        while(stream.read((char*)&splat, epc.binPntSize)) {
            if (splat.p.length() < 1e-6) cout << " B GOT P0 " << splat.p << endl;
            wstream.write((char*)&splat, epc.binPntSize);
            Nwritten += 1;
        }

        removeFile(ref.second.path);
    }
    cout << "---- writing points " << Nwritten << endl;

    wstream.close();
    rename(wpath.c_str(), path.c_str());
}

void VRPointCloud::externalSort(string path, size_t chunkSize, double binSize) {
    VRExternalPointCloud epc(path);
    epc.params["binSize"] = toString(binSize);
    cout << "  externalSort headers " << toString(epc.params) << endl;
    cout << "  externalSort scan '" << path << "' contains " << epc.size << " points" << endl;

    size_t Nchunks = epc.size/chunkSize;
    size_t lastChunkSize = epc.size - Nchunks*chunkSize;
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
        stream.seekg(epc.binPntsStart);

        vector<PntCol> buffer;
        buffer.resize(chunkSize);
        auto progressChunks = addProgress("process chunk ", Nchunks+1);

        for (size_t i = 0; i<=Nchunks; i++) { // write sorted chunks to disk, +1 for last partial chunk
            if (i == Nchunks) buffer.resize(lastChunkSize);
            if (buffer.size() > 0) {
                for (size_t j = 0; j<buffer.size(); j++) stream.read((char*)&buffer[j], epc.binPntSize);
                sort(buffer.begin(), buffer.end(), compPoints);
                string cPath = path+".chunk."+toString(i);
                ofstream cStream(cPath);
                for (size_t j = 0; j<buffer.size(); j++) cStream.write((char*)&buffer[j], epc.binPntSize);
                cStream.close();
            }
            progressChunks->update(1);
        }

        stream.close();
    };

    auto mergeChunks = [compPoints, epc](size_t levelSize, vector<ifstream>& inStreams, ofstream& wStream, VRProgressPtr progress, bool verbose = false) {
        size_t Nstreams = inStreams.size();
        vector<MergeHead> mergeHeads;
        mergeHeads.resize(Nstreams, MergeHead());

        for (size_t i = 0; i<Nstreams; i++) {
            inStreams[i].read((char*)&mergeHeads[i].point, epc.binPntSize); // read first point
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

            wStream.write((char*)&mergeHeads[kNext].point, epc.binPntSize);
            progress->update(1);
            mergeHeads[kNext].pointer++;
            if (inStreams[kNext]) inStreams[kNext].read((char*)&mergeHeads[kNext].point, epc.binPntSize); // read next point
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

        auto progress = addProgress("process level "+toString(lvl)+" ", epc.size);

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
                VRExternalPointCloud::writePCBHeader(wPath, epc.params);
                VRExternalPointCloud::copyPCBOctree(wPath, epc);
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

vector<VRPointCloud::Splat> VRPointCloud::radiusSearch(Vec3d p, double r) {
    vector<Vec3d> points = octree->radiusPointSearch(p, r);
    vector<PntData> data = octree->radiusSearch(p, r);

    int N = min(points.size(), data.size());
    vector<Splat> res;
    for (int i=0; i<N; i++) {
        Splat s;
        s.p = points[i];
        s.c = data[i].c;
        s.v1 = data[i].v1;
        s.v2 = data[i].v2;
        s.w = data[i].w;
        res.push_back(s);
    }
    return res;
}

vector<VRPointCloud::Splat> VRPointCloud::externalRadiusSearch(string path, Vec3d p, double r) {
    //cout << "VRPointCloud::externalRadiusSearch " << path << endl;
    VRExternalPointCloud epc(path);

    //epc.printOctree();

    // TODO: instead of only getting the chunk containing the center, get all chunks the sphere is contained in!
    //        then for each chunk get its points and do a radius search on it

    auto node = epc.getOctreeNode(p);
    //cout << " getOctreeNode: " << node.chunkOffset << ", " << node.chunkSize << endl;

    vector<Splat> res;
    Splat splat;

    ifstream stream(path);
    stream.seekg(node.chunkOffset, ios::beg);
    if (stream.fail()) cout << " ERROR: seek failed!!!" << endl;
    //auto progress = addProgress("search points ", node.chunkSize);
    for (size_t i = 0; i<node.chunkSize; i++) { // TODO: optimize using octree structure
        stream.read((char*)&splat, epc.binPntSize);
        //if (i < 5) cout << splat.p << endl;
        //res.push_back(splat);
        double D = splat.p.dist(p);
        if (D < r) res.push_back(splat);
        //progress->update(1); // just to see the total time..
    }
    stream.close();
    return res;
}

void VRPointCloud::externalTransform(string path, PosePtr p) {
    cout << "VRPointCloud::externalTransform " << path << endl;
    VRExternalPointCloud epc(path);
    auto progress = addProgress("transform points ", epc.size);

    ifstream stream(path);
    stream.seekg(epc.binPntsStart);

    string wpath = path+".tmp";
    VRExternalPointCloud::writePCBHeader(wpath, epc.params);
    VRExternalPointCloud::copyPCBOctree(wpath, epc);
    ofstream wstream(wpath, ios::app);

    vector<Splat> res;
    Splat splat;
    for (size_t i = 0; i<epc.size; i++) {
        stream.read((char*)&splat, epc.binPntSize);
        splat.p = p->transform(splat.p, true);
        progress->update(1);
        wstream.write((char*)&splat, epc.binPntSize);
    }

    stream.close();
    wstream.close();
    rename(wpath.c_str(), path.c_str());
}

VRPointCloud::Splat VRPointCloud::computeSplat(Vec3d p0, vector<Splat> neighbors) {
    Splat res;

    PCA pca;
    for (auto& splat : neighbors) pca.add(splat.p);
    Pose P = pca.compute();
    Vec3d n = P.up();
    n.normalize();
    Pose O(p0, n);
    O.makeUpOrthogonal();
    res.v1 = toSpherical(O.x());
    res.v2 = toSpherical(O.up());

    res.w = 1; // 1 mm
    int N = neighbors.size();
    if (N >= 3) {
        float d = 0.255;
        for (auto& splat : neighbors) {
            Vec3d dj = splat.p - p0;
            float L = dj.length();
            if (L > 1e-4 && L < d) d = L;
        }
        res.w = d * 1000.0; // in mm
    }

    return res;
}

void VRPointCloud::externalComputeSplats(string path, float neighborsRadius) {
    VRExternalPointCloud epc(path);
    cout << "  externalComputeSplats headers " << toString(epc.params) << endl;
    if (!epc.hasOctree) { cout << "  externalPartition needs a PCB with an octree partition, please run externalSort and externalPartition on it first!" << endl; return; }

    auto progress = addProgress("process splats ", epc.size);

    string wpath = path+".tmp.pcb";
    epc.params["format"] = "x8y8z8r1g1b1u2v2s1";
    epc.params["splatMod"] = "0.001";
    VRExternalPointCloud::writePCBHeader(wpath, epc.params);
    VRExternalPointCloud::copyPCBOctree(wpath, epc);
    ofstream wstream(wpath, ios::app);

    ifstream stream(path);
    stream.seekg(epc.binPntsStart);

    VRPointCloud::Splat splat;
    for (size_t i = 0; i<epc.size; i++) {
        stream.read((char*)&splat, epc.binPntSize);
        vector<Splat> neighbors = externalRadiusSearch(path, splat.p, neighborsRadius);
        auto nsplat = computeSplat(splat.p, neighbors);
        nsplat.p = splat.p;
        nsplat.c = splat.c;
        wstream.write((const char*)&nsplat, Splat::size);
        progress->update(1);
    }

    stream.close();
    wstream.close();
    rename(wpath.c_str(), path.c_str());
}

void VRPointCloud::externalColorize(string path, string imgTable, PosePtr pcPose, float localNorth, float localEast, float pDist, int i1, int i2) {
    cout << "externalColorize " << path << endl;

    // crawl images and create image table and path
    string line;
    ifstream imgData(imgTable);

    struct Viewpoint {
        string imgPath;
        double lat;
        double lon;
        Pose pose;
        int index = 0;
    };

    VRPlanet earth("earth");
    earth.localize(localNorth, localEast);

    // load viewpoints
    string imgDir = getFolderName(imgTable);
    vector<Viewpoint> viewpoints;
    while (getline(imgData, line)) {
        Viewpoint vp;
        auto data = splitString(line, '|');

        vp.imgPath = imgDir+"/"+data[0];
        vp.lat = toFloat(data[2]) + toFloat(data[3])/60.0 + toFloat(data[4]+"."+data[5])/3600.0;
        vp.lon = toFloat(data[7]) + toFloat(data[8])/60.0 + toFloat(data[9]+"."+data[10])/3600.0;

        Vec3d p = earth.fromLatLongPosition(vp.lat, vp.lon, true);
        vp.pose.set(p);

        //cout << toString(data) << ", " << vp.lat << ", " << vp.lon << "  " << p << endl;
        viewpoints.push_back(vp);
    }

    cout << " sort viewpoints" << endl;
    sort(viewpoints.begin(), viewpoints.end(), [](const Viewpoint& a, const Viewpoint& b) -> bool {
        return a.imgPath > b.imgPath;
    });
    cout << "  sort done" << endl;

    int j = 0;
    Vec3d lp = viewpoints[1].pose.pos();
    for (auto& vp : viewpoints) { // recompute directions AFTER sort!
        Vec3d p = vp.pose.pos();
        Vec3d d = p-lp;
        if (j == 0) d = lp - p;
        d.normalize();
        vp.pose.setDir(d);
        vp.index = j;
        lp = p;
        j++;
    }

    // quadtree and path
    auto vpTree = Quadtree::create(1.0);
    vector<PathPtr> vpPaths;
    auto vpPath = Path::create();
    int i=0;
    for (auto& vp : viewpoints) {
        Vec3d p = vp.pose.pos();
        vpTree->add(p, &vp);

        double d = p.dist(lp);
        if (i > i1 && i < i2) {
            if (d > 1e-5) {
                Vec3d D = p-lp;
                D.normalize();
                if (d > pDist) {
                    if (vpPath->size() > 2) {
                        vpPath->compute(2);
                        vpPaths.push_back(vpPath);
                    }
                    vpPath = Path::create();
                } else {
                    //cout << "  add point: " << vp.pose.pos() << "   " << vp.pose.dir() << "   " << vp.pose.dir().dot(D) << endl;
                    vpPath->addPoint(vp.pose);
                }
            }
        }

        lp = p;
        i++;
        //if (i > 1000) break;
    }
    if (vpPath->size() > 2) {
        vpPath->compute(2);
        vpPaths.push_back(vpPath);
    }

    cout << "add " << vpPaths.size() << " paths" << endl;

    // pathtool
    auto pathTool = VRPathtool::create();
    for (auto p : vpPaths) pathTool->addPath(p);
    addChild(pathTool);
    pathTool->setVisuals(0,1);
    pathTool->setBezierVisuals(0,0);
    pathTool->update();


    // go through pc and change color
    VRExternalPointCloud epc(path);
    auto progress = addProgress("process points ", epc.size);

    string wpath = path+".tmp.pcb";
    VRExternalPointCloud::writePCBHeader(wpath, epc.params);
    VRExternalPointCloud::copyPCBOctree(wpath, epc);
    ofstream wstream(wpath, ios::app);

    ifstream stream(path);
    stream.seekg(epc.binPntsStart);

    Vec3ub c(255,255,0);
    VRPointCloud::Splat splat;
    map<string, VRTexturePtr> images;

    auto getImage = [&](string path) {
        if (images.size() > 100) images.clear();
        if (!images.count(path)) {
            VRTexturePtr tex = VRTexture::create();
            tex->read(path);
            images[path] = tex;
        }
        return images[path];
    };

    auto heightTooLow = [&](Vec3d P, Viewpoint* vP) {
        Vec3d D = P - vP->pose.pos();
        D.normalize();
        double z = D.dot(vP->pose.up());
        double v = asin(z)/Pi + 0.5;
        //return bool(v < 0.38);
        //return bool(v < 0.45);
        return bool(v < 0.48);
    };

    for (size_t i = 0; i<epc.size; i++) {
        stream.read((char*)&splat, epc.binPntSize);
        splat.c = c;

        Vec3d P = pcPose->transform(splat.p);
        Viewpoint* vP = (Viewpoint*)vpTree->getClosest(P);
        int dir = 1;
        while (vP && heightTooLow(P, vP)) {
            int ix = vP->index + dir;
            if (ix >= 0 && ix < viewpoints.size()) vP = &viewpoints[ix];
            else dir *= -1;
        }

        if (vP) {
            auto tex = getImage(vP->imgPath);
            splat.c = projectOnPanorama(P, tex, Pose::create(vP->pose));
        }

        // get closest path point
        wstream.write((const char*)&splat, epc.binPntSize);
        progress->update(1);
    }

    stream.close();
    wstream.close();
    rename(wpath.c_str(), path.c_str());
}

Vec3ub VRPointCloud::projectOnPanorama(Vec3d P, VRTexturePtr tex, PosePtr vP) {
    Vec3d vPd = vP->dir();
    Vec3d vPx = vP->x();
    Vec3d vPu = vP->up();
    Vec3d D = P - vP->pos();
    D.normalize();

    double y = -D.dot(vPx);
    double x = D.dot(vPd);
    double z = D.dot(vPu);
    double u = atan2(y,x)/(2*Pi) + 0.5;
    double v = asin(z)/Pi + 0.5;

    //return Vec3ub(u*255, v*255, 0);

    Color4f C = tex->getPixelUV( Vec2d(u,v) );
    Vec3ub c;
    c[0] = C[0]*255;
    c[1] = C[1]*255;
    c[2] = C[2]*255;
    return c;
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
    //float size = 0.1;
	tangentU = vecFromAngles(u)*size;
	tangentV = vecFromAngles(v)*size;
    mwp = gl_ModelViewProjectionMatrix;
    gl_Position = osg_Vertex;
}
);

string VRPointCloud::splatGP =
"#version 150\n"
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
