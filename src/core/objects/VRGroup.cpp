#include "VRGroup.h"
#include "core/utils/toString.h"
#include <libxml++/nodes/element.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

map<string, vector<VRGroup*>* > VRGroup::groups = map<string, vector<VRGroup*>* >();
map<string, VRObject* > VRGroup::templates = map<string, VRObject* >();

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

VRObject* VRGroup::copy(vector<VRObject*> children) {
    VRGroup* g = new VRGroup(getName());
    g->setActive(getActive());
    g->setGroup(getGroup());
    return g;
}

/** Returns the group **/
void VRGroup::setGroup(string g) {
    if (group == g) return;

    // remove from old group!!
    if (groups.count(group) == 1) {
        vector<VRGroup*>* v = groups[group];
        v->erase(std::remove(v->begin(), v->end(), this), v->end());
    }

    // add to new group
    group = g;
    if (groups.count(g) == 0) groups[g] = new vector<VRGroup*>();
    groups[g]->push_back(this);
}

string VRGroup::getGroup() { return group; }

void VRGroup::sync() {
    if (templates.count(group) == 0) return;

    clearChildren();

    for (uint i=0; i<templates[group]->getChildrenCount(); i++)
        addChild(templates[group]->getChild(i)->duplicate());
}

void VRGroup::apply() {
    if (templates.count(group) == 1) delete templates[group];

    templates[group] = new VRObject("Group_anchor");
    for (uint i=0; i<getChildrenCount(); i++)
        templates[group]->addChild(getChild(i)->duplicate());

    for (uint i=0; i< groups[group]->size(); i++) {
        VRGroup* g = groups[group]->at(i);
        if (g->getActive()) g->sync();
    }
}

vector<string> VRGroup::getGroups() {
    vector<string> v;
    map<string, vector<VRGroup*>* >::iterator itr;
    for (itr = groups.begin(); itr != groups.end(); itr++) v.push_back(itr->first);
    return v;
}

vector<VRGroup*>* VRGroup::getGroupObjects() {
    if (groups.count(group) == 0) return 0;
    else return groups[group];
}

bool VRGroup::getActive() { return active; }
void VRGroup::setActive(bool b) { active = b; }

void VRGroup::clearGroups() {
    groups.clear();
    map<string, VRObject* >::iterator itr;
    for (itr = templates.begin(); itr != templates.end(); itr++) delete itr->second;
    templates.clear();
}

OSG_END_NAMESPACE;
