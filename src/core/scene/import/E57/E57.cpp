#include "E57.h"

#include <iostream>
#include "E57Foundation.h"
#include "core/objects/geometry/VRGeometry.h"

using namespace e57;
using namespace std;
using namespace OSG;

VRGeometryPtr OSG::loadE57(string path) {
    try {
        ImageFile imf(path, "r"); // Read file from disk

        StructureNode root = imf.root();
        if (!root.isDefined("/data3D")) { cout << "File doesn't contain 3D images" << endl; return 0; }

        e57::Node n = root.get("/data3D");
        if (n.type() != E57_VECTOR) { cout << "bad file" << endl; return 0; }

        VectorNode data3D(n);
        int64_t scanCount = data3D.childCount(); // number of scans in file

        /// For each scan, print out first 4 points in either Cartesian or Spherical coordinates.
        for (int i = 0; i < scanCount; i++) {
            StructureNode scan(data3D.get(i));
            string sname = scan.pathName();

            CompressedVectorNode points( scan.get("points") );
            string pname = points.pathName();
            auto pCount = points.childCount();
            cout << "Got " << pCount << " points\n";

            /// Call subroutine in this file to print the points
            /// Need to figure out if has Cartesian or spherical coordinate system.
            /// Interrogate the CompressedVector's prototype of its records.
            StructureNode proto(points.prototype());

            /// The prototype should have a field named either "cartesianX" or "sphericalRange".
            if (proto.isDefined("cartesianX") && proto.isDefined("cartesianY") && proto.isDefined("cartesianZ")) {
                vector<SourceDestBuffer> destBuffers;
        #if 1  //??? pick one?
                /// Make a list of buffers to receive the xyz values.
                const int N = 4;
                double x[N];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, N, true));
                double y[N];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, N, true));
                double z[N];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, N, true));

                /// Create a reader of the points CompressedVector, try to read first block of N points
                /// Each call to reader.read() will fill the xyz buffers until the points are exhausted.
                CompressedVectorReader reader = points.reader(destBuffers);
                unsigned gotCount = reader.read();
                cout << "  got first " << gotCount << " points" << endl;

                /// Print the coordinates we got
                for (unsigned i=0; i < gotCount; i++)
                    cout << "  " << i << ". x=" << x[i] << " y=" << y[i] << " z=" << z[i] << endl;
        #else
                /// Make a list of buffers to receive the xyz values.
                int64_t columnIndex[10];     destBuffers.push_back(SourceDestBuffer(imf, "columnIndex", columnIndex, 10, true));

                /// Create a reader of the points CompressedVector, try to read first block of 4 columnIndex
                /// Each call to reader.read() will fill the xyz buffers until the points are exhausted.
                CompressedVectorReader reader = points.reader(destBuffers);
                unsigned gotCount = reader.read();
                cout << "  got first " << gotCount << " points" << endl;

                /// Print the coordinates we got
                for (unsigned i=0; i < gotCount; i++)
                    cout << "  " << i << ". columnIndex=" << columnIndex[i] << endl;
        #endif
                reader.close();
            } else if (proto.isDefined("sphericalRange")) {
                //??? not implemented yet
            } else
                cout << "Error: couldn't find either Cartesian or spherical points in scan" << endl;
        }

        imf.close();
    }
    catch (E57Exception& ex) { ex.report(__FILE__, __LINE__, __FUNCTION__); return 0; }
    catch (std::exception& ex) { cerr << "Got an std::exception, what=" << ex.what() << endl; return 0; }
    catch (...) { cerr << "Got an unknown exception" << endl; return 0; }
    return 0;
}

//void writeE57(VRGeometryPtr geo, string path);





