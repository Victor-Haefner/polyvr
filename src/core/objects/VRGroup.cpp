#include "VRGroup.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, vector<VRGroupWeakPtr> > VRGroup::groups = map<string, vector<VRGroupWeakPtr> >();
map<string, VRObjectWeakPtr > VRGroup::templates = map<string, VRObjectWeakPtr >();

void VRGroup::setup() {
    setGroup( group );
    //setActive( active );
}

VRGroup::VRGroup(string name) : VRObject(name) {
    type = "Group";

    store("group", &group);
    store("active", &active);
}

VRGroup::~VRGroup() {;}

VRGroupPtr VRGroup::create(string name) { return shared_ptr<VRGroup>(new VRGroup(name) ); }
VRGroupPtr VRGroup::ptr() { return static_pointer_cast<VRGroup>( shared_from_this() ); }

VRObjectPtr VRGroup::copy(vector<VRObjectPtr> children) {
    VRGroupPtr g = VRGroup::create(getName());
    g->setActive(getActive());
    g->setGroup(getGroup());
    return g;
}

void VRGroup::setGroup(string g) {
    if (group == g) return;

    // remove from old group!!
    if (groups.count(group) == 1) {
        for (auto gr : groups[group]) {
            if (auto sp = gr.lock()) if (sp == ptr()) gr.reset();
        }
    }

    // add to new group
    group = g;
    if (groups.count(g) == 0) groups[g] = vector<VRGroupWeakPtr>();
    groups[g].push_back( ptr() );
}

string VRGroup::getGroup() { return group; }

void VRGroup::sync() {
    if (templates.count(group) == 0) return;
    auto tmp = templates[group].lock();
    if (tmp == 0) return;

    clearChildren();

    for (uint i=0; i<tmp->getChildrenCount(); i++)
        addChild(tmp->getChild(i)->duplicate());
}

void VRGroup::apply() {
    templates[group] = VRObject::create("Group_anchor");
    auto tmp = templates[group].lock();
    if (tmp == 0) return;

    for (uint i=0; i<getChildrenCount(); i++)
        tmp->addChild(getChild(i)->duplicate());

    for (uint i=0; i< groups[group].size(); i++) {
        VRGroupPtr g = groups[group][i].lock();
        if (g == 0) continue;
        if (g->getActive()) g->sync();
    }
}

vector<string> VRGroup::getGroups() {
    vector<string> v;
    for (auto g : groups) v.push_back(g.first);
    return v;
}

vector<VRGroupWeakPtr> VRGroup::getGroupObjects() {
    if (groups.count(group) == 0) return vector<VRGroupWeakPtr>();
    else return groups[group];
}

bool VRGroup::getActive() { return active; }
void VRGroup::setActive(bool b) { active = b; }

void VRGroup::clearGroups() {
    groups.clear();
    templates.clear();
}

OSG_END_NAMESPACE;
