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

VRReasoner::Result VRReasoner::process(string query, VROntology* onto) {
    if (onto == 0) return Result();
    cout << "VRReasoner query: " << query << endl;

    list<Job> jobs;
    map<string, vector<VROntologyInstance*> > vars;

    vector<string> parts = split(query, ':');
    string q = parts[0];
    parts = split(parts[1], ';');
    for (auto p : parts) jobs.push_back(Job(p));

    int i=0;
    for(; jobs.size() > 0 && i < 20; i++) {
        Job j = jobs.back(); jobs.pop_back();

        string fkt = split(j.content, '(')[0];
        string params = split(j.content, '(')[1];
        params = split(params, ')')[0];
        auto var = split(params, ',');
        vector< vector<string> > vpath;
        for (int i=0; i<var.size(); i++) {
            vpath.push_back(split(var[i], '/'));
            var[i] = vpath[i][0];
        }

        cout << "process " << fkt;
        for (auto vi : var) cout << " " << vi;
        cout << endl;

        // TODO: introduce requirements rules for the existence of some individuals
        // TODO: introduce multiple passes? (for example a first one to gather variables)
        // TODO: implement getAtPath!!!

        if (fkt == "is") {
            if (vars.count(var[0]) == 0) { jobs.push_front(j); continue; }
            for (auto vi : vars[var[0]]) {
                string val1 = vi->getAtPath(vpath[0]);
                string val2 = var[1];
                if (vars.count(var[1])) {
                    for (auto vj : vars[var[1]]) {
                        val2 = vj->getAtPath(vpath[1]);
                        if (val1 == val2) { cout << "   success"; continue; }
                    }
                }
                if (val1 != val2) { jobs.push_front(j); continue; }
                else { cout << "   success"; continue; }
            }
            continue;
        }

        if (fkt == "has") {
            if (vars.count(var[0]) == 0) { jobs.push_front(j); continue; }
            if (vars.count(var[1]) == 0) { jobs.push_front(j); continue; }
            continue;
        }

        cout << " search concept " << fkt << endl;
        auto cl = onto->getConcept(fkt);
        if (cl == 0) { jobs.push_front(j); continue; }

        auto cl_insts = onto->getInstances(fkt);
        //if (cl_insts.size() == 0) { jobs.push_front(j); continue; }
        if (cl_insts.size() == 0) {
            cout << "  no instance found, create anonymous concept " << fkt << " labeled " << var[0] << endl;
            cl_insts.push_back( onto->addInstance(var[0], fkt) );
        }
        vars[var[0]] = cl_insts;

        cout << " found concept " << fkt << " and " << cl_insts.size() << " instances: ";
        for (auto i : cl_insts) cout << " " << i->name;
        cout << endl;
    }

    if (jobs.size() == 0) cout << " done after " << i << " jobs\n";
    else {
        cout << " break after " << i << " jobs\n";
        for (auto j : jobs) cout << "  pending job: " << j.content << endl;
    }

    Result res;
    return res;
}
