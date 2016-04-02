#include "E57.h"

#include <iostream>
#include "E57Foundation.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"

using namespace e57;
using namespace std;
using namespace OSG;

void OSG::loadE57(string path, VRTransformPtr res) {
    res->setName(path);

    try {
        ImageFile imf(path, "r"); // Read file from disk

        StructureNode root = imf.root();
        if (!root.isDefined("/data3D")) { cout << "File doesn't contain 3D images" << endl; return; }

        e57::Node n = root.get("/data3D");
        if (n.type() != E57_VECTOR) { cout << "bad file" << endl; return; }

        VectorNode data3D(n);
        int64_t scanCount = data3D.childCount(); // number of scans in file

        /// For each scan, print out first 4 points in either Cartesian or Spherical coordinates.
        for (int i = 0; i < scanCount; i++) {
            StructureNode scan(data3D.get(i));
            string sname = scan.pathName();

            CompressedVectorNode points( scan.get("points") );
            string pname = points.pathName();
            auto cN = points.childCount();
            cout << "Got " << cN << " points\n";

            /// Call subroutine in this file to print the points
            /// Need to figure out if has Cartesian or spherical coordinate system.
            /// Interrogate the CompressedVector's prototype of its records.
            StructureNode proto(points.prototype());
            bool hasPos = (proto.isDefined("cartesianX") && proto.isDefined("cartesianY") && proto.isDefined("cartesianZ"));
            bool hasCol = (proto.isDefined("colorRed") && proto.isDefined("colorGreen") && proto.isDefined("colorBlue"));
            if (!hasPos) continue;

            VRGeoData data;
            vector<SourceDestBuffer> destBuffers;
            const int N = 4;
            double x[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, N, true));
            double y[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, N, true));
            double z[N]; destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, N, true));
            double r[N];
            double g[N];
            double b[N];
            if (hasCol) {
                destBuffers.push_back(SourceDestBuffer(imf, "colorRed", r, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "colorGreen", g, N, true));
                destBuffers.push_back(SourceDestBuffer(imf, "colorBlue", b, N, true));
            }

            unsigned int gotCount = 0;
            CompressedVectorReader reader = points.reader(destBuffers);
            do {
                gotCount = reader.read();
                for (unsigned j=0; j < gotCount; j++) {
                    int v;
                    if (hasCol) v = data.pushVert(Pnt3f(x[j], y[j], z[j]), Vec3f(0,1,0), Vec3f(r[j]/255.0, g[j]/255.0, b[j]/255.0));
                    else v = data.pushVert(Pnt3f(x[j], y[j], z[j]), Vec3f(0,1,0));
                    data.pushPoint(v);
                }
            } while(gotCount);
            reader.close();

            auto geo = data.asGeometry(pname);
            res->addChild(geo);
        }

        imf.close();
    }
    catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return; }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return; }
    catch (...) { cerr << "Got an unknown exception" << endl; return; }
}

//void writeE57(VRGeometryPtr geo, string path);





