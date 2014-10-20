#include "VRNumberingEngine.h"

#include "core/objects/material/VRMaterial.h"
#include "core/tools/VRText.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;
using namespace std;

VRNumberingEngine::VRNumberingEngine() : VRGeometry("NumbEng") {
    OSG::GeoPnt3fPropertyRecPtr pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRefPtr inds = OSG::GeoUInt32Property::create();

    this->pos = pos;
    this->norms = norms;

    setType(GL_POINTS);
    setPositions(pos);
    setNormals(norms);
    setIndices(inds);

    mat = new VRMaterial("NumbEngMat");
    setMaterial(mat);

    group g;
    groups.push_back(g);
}

bool VRNumberingEngine::checkUIn(int grp) {
    if (grp < 0 or grp > (int)groups.size()) return true;
    return false;
}

bool VRNumberingEngine::checkUIn(int i, int grp) {
    if (grp < 0 or grp > (int)groups.size()) return true;
    if (i < 0 or i > (int)pos->size()) return true;
    return false;
}

void VRNumberingEngine::add(Vec3f p, int N, float f, int d, int grp) {
    if (checkUIn(grp)) return;

    int s = pos->size();
    mesh->getLengths()->setValue(N+s, 0);

    group g = groups[grp];
    for (int i=0; i<N; i++) {
        pos->addValue(p);
        norms->addValue(Vec3f(0,grp,0));
        mesh->getIndices()->addValue(i+s);
    }
}

void VRNumberingEngine::set(int i, Vec3f p, float f, int d, int grp) {
    if (checkUIn(i,grp)) return;
    pos->setValue(p, i);
    norms->setValue(Vec3f(0,grp,0), i);
}

void VRNumberingEngine::setPrePost(int grp, string pre, string post) {
    if (checkUIn(grp)) return;
    groups[grp].pre = pre;
    groups[grp].post = post;
}

int VRNumberingEngine::addPrePost(string pre, string post) {
    group g;
    g.pre = pre; g.post = post;
    groups.push_back(g);
    return groups.size()-1;
}

void VRNumberingEngine::updateTexture() {
    string txt = "0123456789";
    for (auto g : groups) {
        txt += "\n"+g.pre+"\n"+g.post;
    }

    ImageRecPtr img = VRText::get()->create(txt, "SANS 20", 20, Color4f(0,0,0,255), Color4f(0,0,0,0) );
    mat->setTexture(img);
}

