#include "VRPLY.h"
#include "core/objects/geometry/VRGeometry.h"

#include <fstream>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/objects/geometry/VRGeoData.h"

OSG_BEGIN_NAMESPACE;

struct VTKData {
    string name;
    string type; // bit, unsigned_char, char, unsigned_short, short, unsigned_int, int, unsigned_long, long, float, or double
    int N = 0;
    int size = 0;
    Vec3i dimensions;
    Vec3f origin;
    Vec3f spacing;

    vector<Vec3f> vec3fs;
    vector<float> floats;

    void append(vector<string>& data) {
        if (data.size() != size && size != 0) cout << "VTK-WARNING 1\n";
        size = data.size();
        if (size == 3 && type == "float") vec3fs.push_back(toVec3f(data[0] + " " + data[1] + " " + data[2]));
        if (size > 4 && type == "float") {
            for (auto& d : data) floats.push_back( toFloat(d) );
        }
    }
};

struct VTKProject {
    string version;
    string title;
    string format;
    string dataset; // STRUCTURED_POINTS STRUCTURED_GRID UNSTRUCTURED_GRID POLYDATA RECTILINEAR_GRID FIELD
    vector<VTKData> data;

    string toString() {
        string res = " VTK project " + title + ", version " + version + ", format " + format;
        if (dataset.size()) res += ", dataset " + dataset;
        return res;
    }

    VTKData& newData() {
        data.push_back(VTKData());
        return current();
    }

    VTKData& current() {
        if (data.size() == 0) return newData();
        return *data.rbegin();
    }
};

void loadVtk(string path, VRTransformPtr res) {
    cout << "load VTK file " << path << endl;
    ifstream file(path.c_str());
    string line;

    /*int Npoints = 0;
    int Nverts = 0;
    int Nlines = 0;
    int Npolys = 0;
    int NtriStrips = 0;
    int Ncells = 0;
    int Nscalars = 0;
    int Ncscalars = 0;
    int NlookupTable = 1;
    int Nfield = 1;
    int vertSize = 0;
    int lineSize = 0;
    int polySize = 0;
    int cellSize = 0;
    int triStripSize = 0;
    int Xcoords = 0;
    int Ycoords = 0;
    int Zcoords = 0;
    string scalarsName;
    string cscalarsName;
    string vectorsName;
    string normalsName;
    string tcName;
    string tensorsName;
    string fieldName;
    string pointType;
    string cellType;
    string scalarsType;
    string vectorsType;
    string normalsType;
    string tcType;
    string tensorsType;
    string XcoordsType;
    string YcoordsType;
    string ZcoordsType;
    string lookupTable;
    Vec3i dimensions;
    Vec3f origin, spacing;*/

    VTKProject project;

    for (int i=0; getline(file, line); i++) {
        if (line.size() == 0) continue;
        vector<string> data = splitString(line, ' ');
        if (data.size() == 0) continue;

        if (data[0][0] >= 'A' && data[0][0] <= 'Z' || data[0][0] >= 'a' && data[0][0] <= 'z') {
            // main header
            if (i == 0) project.version = data[4];
            if (i == 1) project.title = line;
            if (i == 2) project.format = line;
            if (data[0] == "DATASET") project.dataset = data[1];

            // data set parameter
            if (data[0] == "DIMENSIONS") { project.current().dimensions = toVec3i( data[1] + " " + data[2] + " " + data[3] ); }
            if (data[0] == "ORIGIN") { project.current().origin = toVec3f( data[1] + " " + data[2] + " " + data[3] ); }
            if (data[0] == "SPACING" || data[0] == "ASPECT_RATIO") { project.current().spacing = toVec3f( data[1] + " " + data[2] + " " + data[3] ); }
            if (data[0] == "POINTS") { project.current().N = toInt(data[1]); project.current().type = data[2]; }
            if (data[0] == "X_COORDINATES") { project.current().N = toInt(data[1]); project.current().type = data[2]; }
            if (data[0] == "Y_COORDINATES") { project.current().N = toInt(data[1]); project.current().type = data[2]; }
            if (data[0] == "Z_COORDINATES") { project.current().N = toInt(data[1]); project.current().type = data[2]; }
            if (data[0] == "VERTICES") { project.current().N = toInt(data[1]); project.current().size = toInt(data[2]); }
            if (data[0] == "LINES") { project.current().N = toInt(data[1]); project.current().size = toInt(data[2]); }
            if (data[0] == "POLYGONS") { project.current().N = toInt(data[1]); project.current().size = toInt(data[2]); }
            if (data[0] == "TRIANGLE_STRIPS") { project.current().N = toInt(data[1]); project.current().size = toInt(data[2]); }
            if (data[0] == "CELLS") { project.current().N = toInt(data[1]); project.current().size = toInt(data[2]); }
            if (data[0] == "CELL_TYPES") { project.current().N = toInt(data[1]); }

            // POINT and CELL data
            if (data[0] == "POINT_DATA") { project.newData().N = toInt(data[1]); }
            if (data[0] == "CELL_DATA") { project.newData().N = toInt(data[1]); }

            // POINT and CELL data attributes
            /*if (data[0] == "SCALARS") { scalarsName = data[1]; scalarsType = data[2]; Nscalars = toInt(data[3]); }
            if (data[0] == "VECTORS") { vectorsName = data[1]; vectorsType = data[2]; }
            if (data[0] == "NORMALS") { normalsName = data[1]; normalsType = data[2]; }
            if (data[0] == "TEXTURE_COORDINATES") { tcName = data[1]; tcType = data[2]; }
            if (data[0] == "TENSORS") { tensorsName = data[1]; tensorsType = data[2]; }
            if (data[0] == "FIELD") { fieldName = data[1]; Nfield = toInt( data[2] ); }
            if (data[0] == "LOOKUP_TABLE") { lookupTable = data[1]; if (data.size() >= 2) NlookupTable = toInt(data[2]); }
            if (data[0] == "COLOR_SCALARS") { cscalarsName = data[1]; Ncscalars = toInt(data[2]); }
            */

        } else { // parse actual data rows
            auto& d = project.current();
            d.append(data);
        }
    }

    // parsing finished
    cout << project.toString() << endl;
    file.close();

    // build geometry
    VRGeoData geo;

    if (project.dataset == "STRUCTURED_POINTS") {
        auto& d = project.data[0];
        for (int k=0; k<d.dimensions[2]; k++) {
            for (int j=0; j<d.dimensions[1]; j++) {
                for (int i=0; i<d.dimensions[0]; i++) {
                    geo.pushVert(d.origin + Vec3f(d.spacing[0]*i, d.spacing[1]*j, d.spacing[2]*k) );
                    geo.pushPoint();
                }
            }
        }
    }

    if (project.dataset == "STRUCTURED_GRID") {
        auto& d = project.data[0];
        for (int k=0; k<d.dimensions[2]; k++) {
            for (int j=0; j<d.dimensions[1]; j++) {
                for (int i=0; i<d.dimensions[0]; i++) {
                    int pi = k*d.dimensions[0]*d.dimensions[1] + j*d.dimensions[0] + i;
                    geo.pushVert( d.vec3fs[pi] );
                    geo.pushPoint();
                }
            }
        }
    }

    if (project.dataset == "RECTILINEAR_GRID") {
        auto& d = project.data[0];
        for (int k=0; k<d.dimensions[2]; k++) {
            for (int j=0; j<d.dimensions[1]; j++) {
                for (int i=0; i<d.dimensions[0]; i++) {
                    Vec3f p = Vec3f( project.data[0].floats[i], project.data[1].floats[j], project.data[2].floats[k] );
                    geo.pushVert( p );
                    geo.pushPoint();
                }
            }
        }
    }

    if (project.dataset == "UNSTRUCTURED_GRID") {
        ;
    }

    if (project.dataset == "POLYDATA") {

    }

    if (project.dataset == "FIELD") {

    }

    res->addChild( geo.asGeometry(project.title) );
}

OSG_END_NAMESPACE;
