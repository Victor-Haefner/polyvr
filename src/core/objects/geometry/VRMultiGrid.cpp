#include "VRMultiGrid.h"
#include "VRGeoData.h"
#include "core/utils/toString.h"

using namespace OSG;

VRMultiGrid::VRMultiGrid(string name) : VRGeometry(name) {}
VRMultiGrid::~VRMultiGrid() {}

VRMultiGridPtr VRMultiGrid::create(string name) { return VRMultiGridPtr( new VRMultiGrid(name) ); }
VRMultiGridPtr VRMultiGrid::ptr() { return static_pointer_cast<VRMultiGrid>(shared_from_this()); }

void VRMultiGrid::clear() {
    grids.clear();
    outerGrid = -1;
}

void VRMultiGrid::addGrid(Vec4d rect, Vec2d res) {
    cout << "addGrid " << rect << " " << res << endl;
    Grid g;
    g.rect = rect;
    g.res = res;
    grids.push_back(g);
}

bool VRMultiGrid::compute(VRGeometryPtr geo) {
    if (checkForOverlap()) {
        cout << "VRMultiGrid::compute failed, grid overlap detected!" << endl;
        return false;
    }

    getOuterGrid();
    computeTree();
    computeGeo(geo);
    return true;
}

bool VRMultiGrid::checkForOverlap() {
    for (auto& g1 : grids) {
        for (auto& g2 : grids) {
            if (isInside(g1, Vec3d(g2.rect[0],0,g2.rect[2]))) {
                if (!isInside(g1, Vec3d(g2.rect[1],0,g2.rect[3]))) return true;
            } else {
                if (isInside(g1, Vec3d(g2.rect[0],0,g2.rect[3]))) return true;
                if (isInside(g1, Vec3d(g2.rect[1],0,g2.rect[2]))) return true;
                if (isInside(g1, Vec3d(g2.rect[1],0,g2.rect[3]))) return true;
            }
        }
    }

    return false;
}

void VRMultiGrid::getOuterGrid() {
    double xMin = DBL_MAX;

    for (size_t i=0; i<grids.size(); i++) {
        auto& g = grids[i];
        if (g.rect[0] < xMin) {
            outerGrid = i;
            xMin = g.rect[0];
        }
    }
}

bool VRMultiGrid::isInside(Grid& g1, Grid& g2) {
    if (g2.rect[0] < g1.rect[0] || g2.rect[1] > g1.rect[1]) return false;
    if (g2.rect[2] < g1.rect[2] || g2.rect[3] > g1.rect[3]) return false;
    return true;
}

void VRMultiGrid::computeTree() {
    for (size_t i=0; i<grids.size(); i++) { // get parent grid
        auto& g1 = grids[i];
        for (size_t j=0; j<grids.size(); j++) {
            if (i == j) continue;
            auto& g2 = grids[j];
            if (isInside(g2, g1)) {
                if (g1.parent < 0) g1.parent = j;
                else {
                    auto& p = grids[g1.parent];
                    if (isInside(p, g2)) g1.parent = j;
                }
            }
        }
    }

    for (size_t i=0; i<grids.size(); i++) { // set child grids
        auto& g = grids[i];
        if (g.parent < 0) continue;
        auto& p = grids[g.parent];
        p.children.push_back(i);
    }
}

void VRMultiGrid::computeGeo(VRGeometryPtr geo) {
    if (outerGrid == -1) return;
    VRGeoData data;
    computeGridGeo(outerGrid, data);
    data.apply(geo ? geo : ptr());
}

bool VRMultiGrid::isInside(Grid& grid, const Vec3d& p, double e) {
    if (p[0] < grid.rect[0]-e || p[0] > grid.rect[1]+e) return false;
    if (p[2] < grid.rect[2]-e || p[2] > grid.rect[3]+e) return false;
    return true;
}

int VRMultiGrid::isInChild(Grid& grid, const Vec3d& p, double e) {
    for (auto& c : grid.children) {
        if (isInside(grids[c], p, e)) return c;
    }
    return -1;
}

void VRMultiGrid::computeGridGeo(int gridID, VRGeoData& data) {
    auto& grid = grids[gridID];
    for (auto& cID : grid.children) computeGridGeo(cID, data);

    grid.Nx = round((grid.rect[1] - grid.rect[0]) / grid.res[0]);
    grid.Ny = round((grid.rect[3] - grid.rect[2]) / grid.res[1]);
    double dx = (grid.rect[1] - grid.rect[0]) / grid.Nx;
    double dy = (grid.rect[3] - grid.rect[2]) / grid.Ny;

    Vec3d n(0,1,0);
    Vec3d p(grid.rect[0], 0, grid.rect[2]);
    int k = data.size();

    vector<Vec2i> lastRow; // (vID, cID)
    vector<Vec2i> currentRow;

    for (int j=0; j<=grid.Ny; j++) {
        lastRow = currentRow;
        currentRow.clear();

        for (int i=0; i<=grid.Nx; i++) {
            int cID = isInChild(grid, p, -e);
            if (cID != -1) {
                currentRow.push_back(Vec2i(-1, cID));
                if (i > 0 && j > 0) { // check for frame
                    int vUp = lastRow[i][0];
                    int vLe = currentRow[i-1][0];
                    if (vLe != -1 && vUp != 0) grid.frames[cID].push_back(lastRow[i-1][0]); // frame corner U L
                    if (vUp != -1) grid.frames[cID].push_back(vUp); // frame U
                    if (vLe != -1) grid.frames[cID].push_back(vLe); // frame L
                }
            } else {
                int vID = data.pushVert(p,n);
                currentRow.push_back(Vec2i(vID, -1));
                if (i > 0 && j > 0) { // check for quad and then frame
                    int vID1 = currentRow[i-1][0];
                    int vID2 = currentRow[i][0];
                    int vID3 = lastRow[i][0];
                    int vID4 = lastRow[i-1][0];

                    if (vID1 >= 0 && vID2 >= 0 && vID3 >= 0 && vID4 >= 0) {
                        data.pushQuad(vID1, vID2, vID3, vID4);
                    }

                    // frame
                    int vUpLe = lastRow[i-1][0];
                    int vUp = lastRow[i][0];
                    int vLe = currentRow[i-1][0];
                    int cID = currentRow[i-1][1];
                    if (vLe == -1 && vUpLe != -1) grid.frames[cID].push_back(vUp); // frame corner U R
                    if (vLe == -1) grid.frames[cID].push_back(vID); // frame R
                    cID = lastRow[i][1];
                    if (vUpLe != -1 && vUp == -1) grid.frames[cID].push_back(vLe); // frame corner B L
                    if (vUp == -1) grid.frames[cID].push_back(vID); // frame B
                    cID = lastRow[i-1][1];
                    if (vUpLe == -1 && vUp != -1 && vLe != -1) grid.frames[cID].push_back(vID); // frame corner B R
                }

                if (j == 0 || j == grid.Ny || i == 0 || i == grid.Nx)
                    grid.border.push_back(vID);
            }
            k++;
            p[0] += dx;
        }
        p[2] += dy;
        p[0] = grid.rect[0];
    }

    for (auto& f : grid.frames) { // fill frame child gap
        auto& frame = f.second;
        auto& border = grids[f.first];

        ; // TODO
    }

    cout << "Grid " << gridID << " geo, border: " << toString(grid.border) << endl;
    cout << "           frames: " << grid.frames.size() << endl;
    for (auto& f : grid.frames)
    cout << "           child " << f.first << " frame: " << toString(f.second) << endl;
}




