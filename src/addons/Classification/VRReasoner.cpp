#include "VRReasoner.h"
#include "VROntology.h"

#include <iostream>
#include <sstream>
#include <list>

using namespace std;

VRReasoner::VRReasoner() {;}

VRReasoner* VRReasoner::get() {
    static VRReasoner* r = new VRReasoner();
    return r;
}

string VRReasoner::Result::toString() {
    //if (o) return o->toString();
    return "";
}

VRReasoner::Job::Job( string s ) { content = s; }

vector<string> VRReasoner::split(string s, string d) {
    vector<string> res;
    size_t pos = 0;
    int dN = d.length();

    string t;
    while ((pos = s.find(d)) != string::npos) {
        t = s.substr(0, pos);
        s.erase(0, pos + dN);
        res.push_back(t);
    }

    return res;
}

vector<string> VRReasoner::split(string s, char d) {
    vector<string> res;
    string t;
    stringstream ss(s);
    while (getline(ss, t, d)) res.push_back(t);
    return res;
}

bool VRReasoner::startswith(string s, string subs) {
    return s.compare(0, subs.size(), subs);
}

VRReasoner::Result VRReasoner::process(string query) {
    cout << "VRReasoner query: " << query << endl;

    list<Job> jobs;
    map<string, VROntologyInstance*> vars;

    vector<string> parts = split(query, ':');
    string q = parts[0];
    parts = split(parts[1], ';');
    for (auto p : parts) jobs.push_back(Job(p));

    int i=0;
    for(; jobs.size() > 0 && i < 100; i++) {
        Job j = jobs.back(); jobs.pop_back();

        if (startswith(j.content,"is")) {
            string params = split(j.content, '(')[1];
            params = split(j.content, ')')[0];
            auto param = split(params, ',');
            string v = param[0];
            if (vars.count(v) == 0) { jobs.push_front(j); continue; }
            continue;
        }

        if (startswith(j.content,"has")) {
            string params = split(j.content, '(')[1];
            params = split(j.content, ')')[0];
            auto param = split(params, ',');
            string v1 = param[0];
            string v2 = param[1];
            if (vars.count(v1) == 0) { jobs.push_front(j); continue; }
            if (vars.count(v2) == 0) { jobs.push_front(j); continue; }
            continue;
        }

        ;
    }

    cout << " done after " << i << " jobs\n";

    Result res;
    return res;
}
