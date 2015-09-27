#include "VRGroup.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, vector<VRGroupPtr>* > VRGroup::groups = map<string, vector<VRGroupPtr>* >();
map<string, VRObjectPtr > VRGroup::templates = map<string, VRObjectPtr >();

void VRGroup::saveContent(xmlpp::Element* e) {
    VRObject::saveContent(e);
    e->set_attribute("group", group);
    e->set_attribute("active", toString(active));
}

void VRGroup::loadContent(xmlpp::Element* e) {
    VRObject::loadContent(e);
    setGroup( e->get_attribute("group")->get_value().c_str() );
    //setActive( e->get_attribute("active")->get_value().c_str() );
}

VRGroup::VRGroup(string name) : VRObject(name) {
    type = "Group";
    active = true;
    group = "";
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

/** Returns the group **/
void VRGroup::setGroup(string g) {
    if (group == g) return;

    // remove from old group!!
    if (groups.count(group) == 1) {
        vector<VRGroupPtr>* v = groups[group];
        v->erase(std::remove(v->begin(), v->end(), ptr()), v->end());
    }

    // add to new group
    group = g;
    if (groups.count(g) == 0) groups[g] = new vector<VRGroupPtr>();
    groups[g]->push_back(ptr());
}

string VRGroup::getGroup() { return group; }

void VRGroup::sync() {
    if (templates.count(group) == 0) return;

    clearChildren();

    for (uint i=0; i<templates[group]->getChildrenCount(); i++)
        addChild(templates[group]->getChild(i)->duplicate());
}

void VRGroup::apply() {
    templates[group] = VRObject::create("Group_anchor");
    for (uint i=0; i<getChildrenCount(); i++)
        templates[group]->addChild(getChild(i)->duplicate());

    for (uint i=0; i< groups[group]->size(); i++) {
        VRGroupPtr g = groups[group]->at(i);
        if (g->getActive()) g->sync();
    }
}

vector<string> VRGroup::getGroups() {
    vector<string> v;
    map<string, vector<VRGroupPtr>* >::iterator itr;
    for (itr = groups.begin(); itr != groups.end(); itr++) v.push_back(itr->first);
    return v;
}

vector<VRGroupPtr>* VRGroup::getGroupObjects() {
    if (groups.count(group) == 0) return 0;
    else return groups[group];
}

bool VRGroup::getActive() { return active; }
void VRGroup::setActive(bool b) { active = b; }

void VRGroup::clearGroups() {
    groups.clear();
    templates.clear();
}

OSG_END_NAMESPACE;
