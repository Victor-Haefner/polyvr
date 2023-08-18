#include "E57.h"

#include <iostream>

#include "E57Foundation.h"
#include "E57Simple.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRPointCloud.h"
#include "core/objects/VRLod.h"
#include "core/math/pose.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/partitioning/Octree.h"
#include "core/utils/VRProgress.h"
#include "core/utils/toString.h"
#include "core/utils/MemMonitor.h"

#ifndef __EMSCRIPTEN__
#ifndef _WIN32
#include <fcntl.h> // to tell the system how we will read data
#include <ext/stdio_filebuf.h>
#endif
#endif // __EMSCRIPTEN__

using namespace e57;
using namespace std;
using namespace OSG;

class E57Scan {
    public:
        string name;
        size_t pointCount = 0;
        bool hasPos;
        bool hasCol;
        bool hasInt;
        Vec3d pointOffset; // TODO: add as pointcloud option
        CompressedVectorNode points;
        PosePtr pose;

    public:
        E57Scan(StructureNode node) : points( node.get("points") ) {
            name = node.pathName();
            string pname = points.pathName();
            pointCount = points.childCount();

            StructureNode proto(points.prototype());
            hasPos = (proto.isDefined("cartesianX") && proto.isDefined("cartesianY") && proto.isDefined("cartesianZ"));
            hasCol = (proto.isDefined("colorRed") && proto.isDefined("colorGreen") && proto.isDefined("colorBlue"));
            hasInt = proto.isDefined("intensity");
            pose = extractPose(node);

            //cout << "  scan " << scan_i << " dump: " << endl; scan.dump();
            /*cout << "  scan " << i << " contains " << cN << " points\n";
            if (hasCol) cout << "   scan has colors\n";
            if (hasInt) cout << "   scan has intensities\n";
            for (int i=0; i<proto.childCount(); i++) {
                auto child = proto.get(i);
                cout << "    scan data: " << child.pathName() << endl;
            }*/
        }

        PosePtr extractPose(StructureNode& node) {
            if (!node.isDefined("pose")) return Pose::create();

            StructureNode ep(node.get("pose"));
            StructureNode tn(ep.get("translation"));
            StructureNode rn(ep.get("rotation"));

            Pnt3d p;
            p[0] = e57::FloatNode(tn.get("x")).value();
            p[1] = e57::FloatNode(tn.get("y")).value();
            p[2] = e57::FloatNode(tn.get("z")).value();

            OSG::Quaterniond q;
            q[0] = e57::FloatNode(rn.get("x")).value();
            q[1] = e57::FloatNode(rn.get("y")).value();
            q[2] = e57::FloatNode(rn.get("z")).value();
            q[3] = e57::FloatNode(rn.get("w")).value();

            p -= pointOffset;

            cout << "  scan, extractPose p: " << p << " q: " << q << endl;

            Matrix4d m;
            m.setTranslate(p);
            m.setRotate(q);
            return Pose::create(m);
        }
};

class E57Loader {
    public:
        string path;
        int64_t scanCount = 0;
        int64_t pointCount = 0;
        bool hasCol = false;
        bool hasInt = false;
        vector<E57Scan> scans;
        ImageFile imf;
        bool valid = false;

        struct CBData {
            Vec3d P;
            Vec3ub C;
        };

        typedef function<void(E57Scan&, CBData&)> ProcessCB;

    public:
        E57Loader(string path) : path(path), imf(path, "r") {
            try {
                StructureNode root = imf.root();
                if (!root.isDefined("/data3D")) { cout << "File doesn't contain 3D images" << endl; return; }

                //root.dump();

                e57::Node n = root.get("/data3D");
                if (n.type() != E57_VECTOR) { cout << "bad file" << endl; return; }

                VectorNode data3D(n);
                scanCount = data3D.childCount(); // number of scans in file
                cout << " file read succefully, it contains " << scanCount << " scans" << endl;

                for (int i = 0; i < scanCount; i++) {
                    StructureNode scanNode(data3D.get(i));
                    E57Scan scan(scanNode);
                    if (!scan.hasPos) continue;
                    scans.push_back(scan);
                    pointCount += scan.pointCount;
                    if (scan.hasCol) hasCol = true;
                    if (scan.hasInt) hasInt = true;
                }

                valid = true;
            }
            catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return; }
            catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
            catch (...) { cerr << "Got an unknown exception" << endl; return; }
        }

        ~E57Loader() {
            imf.close();
        }

        void process(ProcessCB cb, int N = 4, size_t Nskip = 0) {
            for (int scanI = 0; scanI < scanCount; scanI++) {
                E57Scan& scan = scans[scanI];

                vector<SourceDestBuffer> destBuffers;
                double x[N];
                double y[N];
                double z[N];
                double r[N];
                double g[N];
                double b[N];
                double i[N];

                destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, N, true));
                if (scan.hasCol) {
                    destBuffers.push_back(SourceDestBuffer(imf, "colorRed", r, N, true));
                    destBuffers.push_back(SourceDestBuffer(imf, "colorGreen", g, N, true));
                    destBuffers.push_back(SourceDestBuffer(imf, "colorBlue", b, N, true));
                } else if (scan.hasInt) {
                    destBuffers.push_back(SourceDestBuffer(imf, "intensity", i, N, true));
                }

                int gotCount = 0;
                int Nskipped = 0;
                CBData data;
                CompressedVectorReader reader = scan.points.reader(destBuffers);

                if (Nskip == 0) {
                    do {
                        gotCount = (int)reader.read();
                        for (int j=0; j < gotCount; j++) {
                            data.P = Vec3d(x[j], y[j], z[j]);
                            if (scan.hasCol) data.C = Vec3ub(r[j], g[j], b[j]);
                            if (scan.hasInt) data.C = Vec3ub(i[j], i[j], i[j]);
                            cb(scan, data);
                        }
                    } while(gotCount);
                } else {
                    do {
                        gotCount = (int)reader.read();
                        if (gotCount > 0) {
                            if (Nskipped+gotCount <= Nskip) Nskipped += gotCount;
                            else for (int j=0; j < gotCount; j++) {
                                Nskipped++;
                                if (Nskipped >= Nskip) {
                                    data.P = Vec3d(x[j], y[j], z[j]);
                                    if (scan.hasCol) data.C = Vec3ub(r[j], g[j], b[j]);
                                    if (scan.hasInt) data.C = Vec3ub(i[j], i[j], i[j]);
                                    cb(scan, data);
                                    Nskipped = 0;
                                }
                            }
                        }
                    } while(gotCount);
                }
                reader.close();
            }
        }
};

void OSG::convertE57(vector<string> pathsIn, string pathOut) {
    auto progress = VRProgress::create();
    size_t cN = 0;
    bool hasCol = false;
    for (auto pathIn : pathsIn) {
        E57Loader loader(pathIn);
        cN += loader.pointCount;
        if (loader.hasCol || loader.hasInt) hasCol = true;
    }
    progress->setup("process points ", cN);
    progress->reset();

    map<string, string> params;
    params["N_points"] = toString(cN);
    params["format"] = "x8y8z8";
    if (hasCol) params["format"] += "r1g1b1";
    VRPointCloud::writePCBHeader(pathOut, params);

    ofstream stream(pathOut, ios::app);

    Boundingbox bb;
    auto onPoints = [&](E57Scan& scan, E57Loader::CBData& data) {
        Vec3d P = scan.pose->transform(data.P);
        stream.write((const char*)&P[0], sizeof(Vec3d));
        if (hasCol) stream.write((const char*)&data.C[0], sizeof(Vec3ub));
        bb.update(P);
        progress->update(1);
    };

    for (auto pathIn : pathsIn) {
        E57Loader loader(pathIn);
        loader.process(onPoints);
    }
    cout << " convertE57 final BB, center: " << bb.center() << " size: " << bb.size() << endl;

    stream.close();
}

// nice reference implementation
//  https://github.com/CloudCompare/CloudCompare/blob/master/plugins/core/IO/qE57IO/src/E57Filter.cpp#L1329

void OSG::loadE57(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load e57 pointcloud " << path << endl;
    importOptions["filePath"] = path;
    res->setName(path);

    float downsampling = 1;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);

    E57Loader loader(path);
    size_t cN = loader.pointCount;

    auto progress = VRProgress::create();
    progress->setup("process points ", cN);
    progress->reset();

    auto pointcloud = VRPointCloud::create("pointcloud");
    pointcloud->applySettings(importOptions);

    auto onPoint = [&](E57Scan& scan, E57Loader::CBData& data) {
        pointcloud->addPoint(data.P, data.C);
        progress->update(1);
    };

    cout << "fill octree" << endl;
    size_t Nskip = round(1.0/downsampling) - 1;
    loader.process(onPoint, 1, Nskip);

    pointcloud->setupLODs();
    for (auto& scan : loader.scans) {
        if (scan.pose) {
            pointcloud->setPose(scan.pose);
            cout << " scan pose: " << toString(scan.pose) << endl;
        }
    }
    res->addChild(pointcloud);
}

vector<size_t> extractSortedRegionBounds(string path, vector<double> region) {
    if (region.size() != 6) return {};

    auto params = VRPointCloud::readPCBHeader(path);
    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);

    auto cN = toValue<size_t>(params["N_points"]);
    if (cN == 0) return {};

    bool hasCol = contains( params["format"], "r");
    bool hasSpl = contains( params["format"], "u");
    double binSize = toValue<double>(params["binSize"]);
    if (binSize == 0) {
        cout << " extractRegionBounds failed, binSize is 0!" << endl;
        return {};
    }

    size_t s0 = stream.tellg();
    int N = sizeof(Vec3d);
    if (hasCol) N = VRPointCloud::PntCol::size;
    if (hasSpl) N = VRPointCloud::Splat::size;
    VRPointCloud::Splat pnt;

    cout << " extractRegionBounds - headers: " << toString(params) << ", cN: " << cN << endl;

    auto getPoint = [&](size_t pID) -> Vec3d {
        stream.seekg(s0+N*pID, ios::beg);
        stream.read((char*)&pnt, N);
        return pnt.p;
    };

    static const double eps = 1e-6;

    struct Segment {
        int coord;
        double min;
        double max;

        Segment(int c, double binSize) {
            coord = c;
            min = coord*binSize - binSize - eps;
            max = min + binSize;
        }

        Segment(double& v, double binSize) {
            coord = round(0.5 + eps + v/binSize);
            min = coord*binSize - binSize - eps;
            max = min + binSize;
        }
    };

    auto homeIn = [&](size_t& A, size_t& B, int axis, double value) {
        //cout << " homeIn on " << v << " in range " << Vec2i(A, B) << endl;
        if (A >= B) {
            cout << " homeIn error, inverted range " << Vec2i(A, B) << endl;
            return;
        }

        while(B-A > 1) {
            size_t M = (A + B)*0.5;
            Vec3d PM = getPoint(M);
            if (PM[axis] < value) A = M;
            if (PM[axis] > value) B = M;
            //cout << "  M " << M << ", AB " << Vec2i(A, B) << endl;
        }
    };

    Segment segX1(region[0], binSize);
    Segment segX2(region[1], binSize);
    Segment segY1(region[2], binSize);
    Segment segY2(region[3], binSize);
    Segment segZ1(region[4], binSize);
    Segment segZ2(region[5], binSize);
    vector<size_t> bounds;

    // first find bounds on Y axis and all layers
    vector<size_t> Ybounds = { 0, cN, cN };
    homeIn(Ybounds[0], Ybounds[1], 1, segY1.min); // Y lower bound
    homeIn(Ybounds[0], Ybounds[2], 1, segY2.max); // Y upper bound

    struct Range {
        Segment segment;
        size_t b0 = 0;
        size_t b1 = 0;

        Range(int i, double binSize) : segment(i, binSize) {}
    };

    //bounds.push_back(Ybounds[1]);
    //bounds.push_back(Ybounds[2]);

    for (int i = segY1.coord; i <= segY2.coord; i++) {
        Range layer(i, binSize);

        size_t lb = Ybounds[1];
        layer.b0 = Ybounds[2];
        layer.b1 = Ybounds[2];
        if (lb == layer.b0) continue;
        homeIn(lb, layer.b0, 1, layer.segment.min); // layer Y lower bound
        if (lb == layer.b1) continue;
        homeIn(lb, layer.b1, 1, layer.segment.max); // layer Y upper bound

        //cout << "  found layer " << i << ", " << Vec2d(layer.b0, layer.b1) << endl;

        for (int j = segX1.coord; j <= segX2.coord; j++) { // then find all row bounds in X
            Range row(j, binSize);
            size_t lb = layer.b0;
            row.b0 = layer.b1;
            row.b1 = layer.b1;
            if (lb == row.b0) continue;
            homeIn(lb, row.b0, 0, row.segment.min); // row X lower bound
            if (lb == row.b1) continue;
            homeIn(lb, row.b1, 0, row.segment.max); // row X upper bound

            //bounds.push_back(row.b0);
            //bounds.push_back(row.b1);


            lb = row.b0;
            size_t cb0 = row.b1;
            size_t cb1 = row.b1;
            if (lb == cb0) continue;
            homeIn(lb, cb0, 2, segZ1.min); // cell Z lower bound
            if (lb == cb1) continue;
            homeIn(lb, cb1, 2, segZ2.max); // cell Z upper bound

            if (cb1 > cb0) {
                bounds.push_back(cb0);
                bounds.push_back(cb1);
            }

            //cout << "   found row " << j << ", " << Vec2d(row.b0, row.b1) << ", Z " << Vec2d(cb0, cb1) << endl;
        }
    }

    //cout << "extractRegionBounds " << toString(bounds) << endl;
    return bounds;
}

vector<size_t> extractOctreeRegionBounds(string path, vector<double> region) {
    if (region.size() != 6) return {};

    auto params = VRPointCloud::readPCBHeader(path);
    ifstream stream(path);
    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);
    size_t ocTree = stream.tellg();

    auto cN = toValue<size_t>(params["N_points"]);
    if (cN == 0) return {};

    bool hasCol = contains( params["format"], "r");
    bool hasSpl = contains( params["format"], "u");
    double binSize = toValue<double>(params["binSize"]);
    if (binSize == 0) {
        cout << " extractRegionBounds failed, binSize is 0!" << endl;
        return {};
    }

    int N = sizeof(Vec3d);
    if (hasCol) N = VRPointCloud::PntCol::size;
    if (hasSpl) N = VRPointCloud::Splat::size;
    VRPointCloud::Splat pnt;

    //cout << " extractRegionBounds - headers: " << toString(params) << ", cN: " << cN << endl;
    double regionSize = region[1]-region[0]; // xmax-xmin
    Vec3d regionCenter = Vec3d((region[0]+region[1])*0.5, (region[2]+region[3])*0.5, (region[4]+region[5])*0.5);
    double rootSize = toValue<double>(params["ocRootSize"]);
    Vec3d rootCenter = toValue<Vec3d>(params["ocRootCenter"]);
    size_t nodeCount = toValue<size_t>(params["ocNodeCount"]);

    // get correct octree node based on region
    size_t ocNodeBinSize = sizeof(ocSerialNode);
    size_t rOffset = (nodeCount-1) * ocNodeBinSize; // root is last node written
    ocSerialNode ocNode;
    stream.seekg(ocTree + rOffset, ios::beg);
    //cout << " get child from " << ocTree + rOffset << ", " << ocNode.chunkOffset << ", " << ocNode.chunkSize << endl;
    stream.read((char*)&ocNode, ocNodeBinSize); // read tree root

    int jumps = 0;
    double nodeSize = rootSize;
    Vec3d nodeCenter = rootCenter; // root is on 0, 0, 0
    while (/*nodeSize > regionSize*1.01*/ true) { // traverse tree until node size corresponds to region
        // compute child octant
        //Vec3d rp = nodeCenter - regionCenter;
        Vec3d rp = regionCenter - nodeCenter;
        int octant = 0;
        if (rp[0] < 0) octant += 1;
        if (rp[1] < 0) octant += 2;
        if (rp[2] < 0) octant += 4;

        // get child address
        int cOffset = ocNode.children[octant];
        if (cOffset == 0) {
            //cout << "  WARNING! child is 0! ..after " << jumps << " jumps, regionCenter: " << regionCenter << ", nodeCenter: " << nodeCenter << " / " << ocNode.center << ", nodeSize: " << nodeSize << " / " << ocNode.size << ", regionSize: " << regionSize << ", octant: " << octant << endl;
            //for (int i=0; i<8; i++) cout << "   child " << i << ": " << ocNode.children[i] << endl;
            break;
        } else {
            //cout << "  Got child after " << jumps << " jumps, regionCenter: " << regionCenter << ", nodeCenter: " << nodeCenter << " / " << ocNode.center << ", nodeSize: " << nodeSize << " / " << ocNode.size << ", regionSize: " << regionSize << ", octant: " << octant << endl;
            //for (int i=0; i<8; i++) cout << "   child " << i << ": " << ocNode.children[i] << endl;
        }

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

    //cout << "  + + found node with chunk at " << ocNode.chunkOffset << " and chunk size " << ocNode.chunkSize << endl;

    // print all tree nodes
    /*if (false) {
        ofstream log("tmp.log");
        ocSerialNode tmp;
        stream.seekg(ocTree, ios::beg);
        for (int i=0; i<nodeCount; i++) {
            stream.read((char*)&tmp, ocNodeBinSize);
            log << "node " << i << " at " << i*ocNodeBinSize << ": chunk " << tmp.chunkOffset << ", " << tmp.chunkSize << ", children: ";
            for (int j=0; j<8; j++) log << tmp.children[j] << " ";
            log << endl;
        }
        log.close();
    }*/

    //return {37089059,37089059}; // TEMP

    vector<size_t> bounds;
    size_t bound0 = (ocNode.chunkOffset - ocTree - nodeCount * ocNodeBinSize) / N; // bounds are offsets relative to the first point
    size_t bound1 = bound0 + ocNode.chunkSize; // N;
    bounds.push_back(bound0);
    bounds.push_back(bound1);
    //cout << "extractOctreeRegionBounds " << toString(bounds) << endl;
    return bounds;
}

void OSG::loadPCB(string path, VRTransformPtr res, map<string, string> importOptions) {
    //cout << "load PCB pointcloud " << path << endl;
    importOptions["filePath"] = path;
    res->setName(path);

    float downsampling = 1;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);

    auto params = VRPointCloud::readPCBHeader(path);
    ifstream stream(path);

    int hL = toInt(params["headerLength"]);
    stream.seekg(hL);
    //cout << "  headers: " << toString(params) << endl;

    auto cN = toValue<size_t>(params["N_points"]);
    bool hasCol = contains( params["format"], "r");
    bool hasSpl = contains( params["format"], "u");
    bool hasOct = params.count("partitionStructure");
    //double binSize = toValue<double>(params["binarySize"]);
    //cout << "  scan contains " << cN << " points" << endl;

    if (hasSpl) importOptions["doSplats"] = "1";
    if (params.count("splatMod")) importOptions["splatMod"] = params["splatMod"];
    else importOptions["splatMod"] = "0.001"; // in mm

    auto progress = VRProgress::create();
    progress->setup("process points ", cN);
    progress->reset();

    /*if (hasCol) cout << "   scan has colors\n";
    else cout << "   scan has no colors\n";
    if (hasSpl) cout << "   scan has splats\n";
    else cout << "   scan has no splats\n";*/

    auto pointcloud = VRPointCloud::create("pointcloud");
    pointcloud->applySettings(importOptions);

    size_t Nskip = round(1.0/downsampling) - 1;
    //size_t count = 0;
    //int Nskipped = 0;

    int N = sizeof(Vec3d);
    if (hasCol) N = VRPointCloud::PntCol::size;
    if (hasSpl) N = VRPointCloud::Splat::size;
    VRPointCloud::Splat pnt;

    auto skipBin = [&](size_t Nskip) { // skip N bytes
        if (Nskip < 50) stream.ignore(Nskip, EOF); // ignore processes all chars
        else stream.seekg(Nskip, ios::cur); // seek jumps directly, better with increasing jump length
    };

    auto skip = [&](size_t Nskip) { // skip N points
        size_t Nprocessed = min(Nskip, progress->left());
        if (Nprocessed == 0) return;
        skipBin(N*Nprocessed);
        progress->update( Nprocessed );
    };

    // get binary bounds of region
    auto region = toValue<vector<double>>(importOptions["region"]);
    vector<size_t> bounds;
    if (hasOct) bounds = extractOctreeRegionBounds(path, region);
    else bounds = extractSortedRegionBounds(path, region);

    // skip octree
    if (hasOct) {
        //cout << "---- got octree at " << stream.tellg() << endl;
        size_t Nnodes = toInt( params["ocNodeCount"] );
        size_t ocNodeBinSize = sizeof(ocSerialNode);
        size_t ocBinSize = Nnodes * ocNodeBinSize;
        skipBin(ocBinSize);
        //cout << "---- end of octree at " << stream.tellg() << endl;
    }

    if (!hasOct || true) { // read from a sorted PC // TODO: is this also valid for octrees?
        int boundsID = 0;
        int Nbounds = bounds.size()*0.5;
        if (Nbounds > 0 && bounds[0] > 0) {
            skip(bounds[0]);
        }

        size_t pointsRead = 0;
        Vec3d pSum;
        //if (Nbounds > 0) cout << endl << " + + try read points " << bounds[1]-bounds[0] << " at " << bounds[0] << ", N: " << N << endl;
        while (stream.read((char*)&pnt, N)) {
            Vec3d  pos = pnt.p;
            if (pos.length() < 1e-6) continue; // ignore zeros..
            Vec3ub rgb = pnt.c;
            if (hasSpl) pointcloud->addPoint(pos, pnt);
            else pointcloud->addPoint(pos, rgb);
            pointsRead += 1;
            pSum += pos;
            progress->update( 1 );
            if (Nskip>0) skip(Nskip);

            if (boundsID < Nbounds) {
                size_t upperBound = bounds[boundsID*2+1];
                if ( upperBound > 0 && progress->current() >= upperBound) {
                    boundsID++;
                    if (boundsID < Nbounds) {
                        size_t nextLowerBound = bounds[boundsID*2];
                        skip(nextLowerBound-upperBound);
                    } else break;
                }
            }
        }

        //cout << endl << " + + read " << pointsRead << endl;
    }

    pointcloud->setupLODs();
    res->addChild(pointcloud); // TODO: threading -> problems with states, re-adding it as child in main thread fixes issue!
    stream.close();
    cout << "  PCB import finished" << endl;
}

void OSG::loadXYZ(string path, VRTransformPtr res, map<string, string> importOptions) {
    cout << "load xyz pointcloud " << path << endl;
    importOptions["filePath"] = path;
    res->setName(path);

    float downsampling = 1;
    bool swapYZ = 0;
    bool xyzNoColor = 0;
    if (importOptions.count("downsampling")) downsampling = toFloat(importOptions["downsampling"]);
    if (importOptions.count("swapYZ")) swapYZ = toInt(importOptions["swapYZ"]);
    if (importOptions.count("xyzNoColor")) xyzNoColor = toInt(importOptions["xyzNoColor"]);

    /*cout << "OSG::loadXYZ, swapYZ: " << swapYZ << endl;
    for (auto o : importOptions) {
        cout << " importOption " << o.first << " " << o.second << endl;
    }*/
    try {
        auto pointcloud = VRPointCloud::create("pointcloud");
        pointcloud->applySettings(importOptions);

        VRGeoData data;
        if (!xyzNoColor){
            vector<float> vertex = vector<float>(6);
            int i=0;
            int Nskip = round(1.0/downsampling);
            int Nskipped = 0;

            ifstream file(path);
            while (file >> vertex[i]) {
                i++;
                if (i >= 6) {
                    i = 0;
                    Nskipped++;
                    if (Nskipped >= Nskip) {
                        //data.pushVert(Pnt3d(vertex[0], vertex[1], vertex[2]), Vec3d(0,1,0), Color3f(vertex[3]/255.0, vertex[4]/255.0, vertex[5]/255.0));
                        //data.pushPoint();
                        Vec3d pos;
                        if (swapYZ) pos = Vec3d(vertex[0], vertex[2], -vertex[1]);
                        else pos = Vec3d(vertex[0], vertex[1], vertex[2]);
                        Color3ub col(vertex[3], vertex[4], vertex[5]);
                        pointcloud->addPoint(pos, col);
                        Nskipped = 0;
                    }
                }
            }
        } else {
            vector<float> vertex = vector<float>(3);
            cout << " pointcloud, no Color" << endl;
            int i=0;
            int Nskip = round(1.0/downsampling);
            int Nskipped = 0;

            ifstream file(path);
            while (file >> vertex[i]) {
                i++;

                if (i >= 3) {
                    i = 0;
                    Nskipped++;
                    if (Nskipped >= Nskip) {

                        /*
                        if (swapYZ) data.pushVert(Pnt3d(vertex[0], vertex[2], -vertex[1]), Vec3d(0,1,0), Color3f(0.0, 0.0, 0.0));
                        else data.pushVert(Pnt3d(vertex[0], vertex[1], vertex[2]), Vec3d(0,1,0), Color3f(0.0, 0.0, 0.0));
                        data.pushPoint();
                        */

                        Vec3d pos;
                        if (swapYZ) pos = Vec3d(vertex[0], vertex[2], -vertex[1]);
                        else pos = Vec3d(vertex[0], vertex[1], vertex[2]);
                        Color3ub col(0, 0, 0);
                        pointcloud->addPoint(pos, col);
                        Nskipped = 0;
                    }
                }
            }
        }

        if (data.size()) {
            cout << "  assemble geometry.. " << endl;
            auto geo = data.asGeometry("points");
            res->addChild(geo);
        }

        pointcloud->setupLODs();
        res->addChild(pointcloud);
    }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

void OSG::writeE57(VRPointCloudPtr pcloud, string path) {
    if (!pcloud) return;

    vector<double>  cartesianX;
    vector<double>  cartesianY;
    vector<double>  cartesianZ;
    vector<int>  colorRed;
    vector<int>  colorGreen;
    vector<int>  colorBlue;

    for (auto g : pcloud->getChildren(true, "Geometry")) {
        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(g);
        Matrix4d M = pcloud->getMatrixTo(geo);
        if (!geo) continue;
        cout << "OSG::writeE57 " << geo->getName() << endl;
        VRGeoData data(geo);
        for (int i=0; i<data.size(); i++) {
            Pnt3d P = data.getPosition(i);
            M.mult(P, P);
            Color3ub C = data.getColor3ub(i);
            cartesianX.push_back(P[0]);
            cartesianY.push_back(P[1]);
            cartesianZ.push_back(P[2]);
            colorRed.push_back(round(C[0]*255));
            colorGreen.push_back(round(C[1]*255));
            colorBlue.push_back(round(C[2]*255));
        }
    }

    try {
        ImageFile imf(path, "w");
        StructureNode root = imf.root();

        root.set("formatName", StringNode(imf, "ASTM E57 3D Imaging Data File"));
        root.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3300"));

        int astmMajor;
        int astmMinor;
        ustring libraryId;
        E57Utilities().getVersions(astmMajor, astmMinor, libraryId);
        root.set("versionMajor", IntegerNode(imf, astmMajor));
        root.set("versionMinor", IntegerNode(imf, astmMinor));
        root.set("coordinateMetadata", StringNode(imf, "Not used."));

        StructureNode creationDateTime = StructureNode(imf);
        root.set("creationDateTime", creationDateTime);
        creationDateTime.set("dateTimeValue", FloatNode(imf));
        creationDateTime.set("isAtomicClockReferenced", IntegerNode(imf));

        VectorNode data3D = VectorNode(imf, true);
        root.set("data3D", data3D);

        StructureNode scan0 = StructureNode(imf);
        data3D.append(scan0);
        scan0.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3301"));

        StructureNode proto = StructureNode(imf);
        proto.set("cartesianX",  FloatNode(imf));
        proto.set("cartesianY",  FloatNode(imf));
        proto.set("cartesianZ",  FloatNode(imf));
        proto.set("colorRed",    IntegerNode(imf, 0, 0, 255));
        proto.set("colorGreen",  IntegerNode(imf, 0, 0, 255));
        proto.set("colorBlue",   IntegerNode(imf, 0, 0, 255));

        VectorNode codecs = VectorNode(imf, true);
        CompressedVectorNode points = CompressedVectorNode(imf, proto, codecs);
        scan0.set("points", points);
        scan0.set("name", StringNode(imf, pcloud->getName()));
        scan0.set("description", StringNode(imf, "PolyVR pointcloud export"));

        const int N = cartesianX.size();
        vector<SourceDestBuffer> sourceBuffers;
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianX",  &cartesianX[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianY",  &cartesianY[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianZ",  &cartesianZ[0],  N, true, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorRed",    &colorRed[0],    N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorGreen",  &colorGreen[0],  N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorBlue",   &colorBlue[0],   N, true));

        CompressedVectorWriter writer = points.writer(sourceBuffers);
        writer.write(N);
        writer.close();
        imf.close();
    }
    catch(E57Exception& ex)     { ex.report(__FILE__, __LINE__, __FUNCTION__); }
    catch(std::exception& ex)   { cerr << "Got an std::exception, what=" << ex.what() << endl; }
    catch(...)                  { cerr << "Got an unknown exception" << endl; }
}





