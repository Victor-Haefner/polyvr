#include "OSMMap.h"
#include "core/utils/toString.h"
#include "core/math/boundingbox.h"
#include "core/utils/VRTimer.h"
#include "core/utils/xml.h"

#ifndef WITHOUT_GDAL

#ifndef _WIN32
#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>
#include <gdal/gdal_version.h>
#include <gdal/ogrsf_frmts.h>
#else
#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_version.h>
#include <ogrsf_frmts.h>
#endif

// define needed before including proj_api.h !!
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#include <proj_api.h>
#endif

#include <iostream>
#include <fstream>
#include <cmath>

/* FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features
*/

/* TESTING GDAL
	filepath = 'test.json'
	VR.map = VR.OSMMap()
	VR.map.readGEOJSON(filepath)
*/

using namespace OSG;


OSG_BEGIN_NAMESPACE;

class OSMSAXHandlerCP : public XMLStreamHandler {
    private:
        long numerator;
        long numeratorWritten;
        long nodeCounter;
        long wayCounter;
        long relCounter;
        long nodeCounterRaw;
        long wayCounterRaw;
        long relCounterRaw;
        int counter;
        int currentDepth; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
        int currentType; //error = -1, node = 0, way = 1, relation = 2
        int lastDepth;
        bool lvl1Open;
        bool lvl1Closer;
        bool lvl1Closed;
        string currentID ="";
            //currentElementPtr;
        string newfilepath = "";
        string buffer = "";
        vector<string> element;

        //map<string, string> whitelist;
        vector<pair<string, string>> whitelist;
        map<string, string> miscAtts;
        map<string, string> tagsInfo;
        vector<string> refsForWays;
        vector<string> nodesForRelations;
        vector<string> waysForRelations;
        map<string, bool> neededNodes;
        bool writeToFile = true;
        bool sndPass = false;

        void writeLine(string line);
        void writeLineToFile(string line);
        void writeElementToFile();
        void handleElement();

        void init();

    public:
        OSMSAXHandlerCP();
        void startDocument();
        void startElement(const string& name, const map<string, string>& attributes);
        void endElement(const string& name);
        void endDocument();

        long getNumerator();
        long getNumeratorWritten();
        long getNodeCounter();
        long getWayCounter();
        long getRelationCounter();
        long getNodeRawCounter();
        long getWayRawCounter();
        long getRelationRawCounter();
        void setWhitelist(vector<pair<string, string>> whl);
        void setWriteToFile(bool in);
        void setPass(bool in);
        void setNewPath(string in);
        map<string,bool> getNeededNodes();
};

class OSMSAXHandlerBM : public XMLStreamHandler {
    private:
        long numerator;
        long numeratorWritten;
        long nodeCounter;
        long wayCounter;
        long relCounter;
        int counter;
        int currentDepth; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
        int currentType; //error = -1, node = 0, way = 1, relation = 2
        int lastDepth;
        bool lvl1Open;
        bool lvl1Closer;
        bool lvl1Closed;
        string currentID ="";
            //currentElementPtr;
        vector<string> element;

        map<string, string> miscAtts;
        map<string, string> tagsInfo;
        vector<string> refsForWays;
        vector<string> nodesForRelations;
        vector<string> waysForRelations;

        map<string, OSMNodePtr> nodes;
        map<string, OSMWayPtr> ways;
        map<string, OSMRelationPtr> relations;

        void handleElement();

        void init();

    public:
        OSMSAXHandlerBM();
        void startDocument();
        void startElement(const string& name, const map<string, string>& attributes);
        void endElement(const string& name);
        void endDocument();

        map<string, OSMNodePtr> getNodes();
        map<string, OSMWayPtr> getWays();
        map<string, OSMRelationPtr> getRelations();

        long getNumerator();
        long getNodeCounter();
        long getWayCounter();
        long getRelationCounter();
};
OSG_END_NAMESPACE;

OSMSAXHandlerCP::OSMSAXHandlerCP() {
}

//CP = Copy & Paste, used for filtering OSM
void OSMSAXHandlerCP::init(){
    numerator = 0;
    numeratorWritten = 0;
    nodeCounter = 0;
    wayCounter = 0;
    relCounter = 0;
    nodeCounterRaw = 0;
    wayCounterRaw = 0;
    relCounterRaw = 0;
    counter = 0;
    currentDepth = -1; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
    currentType = -1; //error = -1, node = 0, way = 1, relation = 2
    lastDepth = -1;

    lvl1Open = false;
    lvl1Closer = false;
    lvl1Closed = true;

    buffer = "";

    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
}

void OSMSAXHandlerCP::handleElement(){
    ///IMPLEMENT WHITELIST/BLACKLIST HERE
    bool force = false; //whitelist

    //checks taglist for wanted tags
    auto checkTag = [&](string key, string val) {
        if ( force ) return true;
        if ( tagsInfo.count(key) ) {
            if ( tagsInfo[key] == val ) return true;
            if ( val.length()<1 ) return true;
        }
        return false;
    };

    auto checkWhiteList = [&](){
        if ( force ) return true;
        bool res = false;
        for (auto each : whitelist) {
            res = checkTag(each.first, each.second);
            if (res) return true;
        }
        return res;
    };

    //bounds
    if ( currentType == -2 ) {
        force = true;
    }

    //nodes
    if ( currentType == 0 ) {
        nodeCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) neededNodes[currentID] = true;
            force = true;
        } else {
            if (neededNodes.count(currentID)) force = true;
        }
        if ( force ) nodeCounter++;
    }

    //ways
    if ( currentType == 1 ) {
        wayCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) {
                for (auto each: refsForWays) {
                    neededNodes[each] = true;
                }
            }
        } else {
            force = true;
        }
        if ( force ) wayCounter++;
    }

    //relations
    if ( currentType == 2 ) {
        relCounterRaw++;
        if ( !sndPass ) {
            force = checkWhiteList();
            if ( force ) {
                for (auto each: nodesForRelations) {
                    neededNodes[each] = true;
                }
            }
        } else {
            force = true;
        }
        if ( force ) relCounter++;
    }

    if ( force ) writeElementToFile();
    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
    numerator++;
    std::cout << "\r" << std::setw (40) << numerator << " elements";
}

void OSMSAXHandlerCP::writeLine(string line) {
    element.push_back(line);
}

void OSMSAXHandlerCP::writeLineToFile(string line) {
    if (writeToFile) {
        std::ofstream ofs;
        //XMLCh *xmlLine = XMLString::transcode();
        ofs.open (newfilepath, std::ofstream::out | std::ofstream::app);
        ofs << line << std::endl;
        //XMLString::release(&xmlLine);
        ofs.close();
    }
}

void OSMSAXHandlerCP::writeElementToFile() {
    for (auto each : element) {
        writeLineToFile(each);
    }
    numeratorWritten++;
}

long OSMSAXHandlerCP::getNumerator() { return numerator; }
long OSMSAXHandlerCP::getNumeratorWritten() { return numeratorWritten; }
long OSMSAXHandlerCP::getNodeCounter() { return nodeCounter; }
long OSMSAXHandlerCP::getNodeRawCounter() { return nodeCounterRaw; }
long OSMSAXHandlerCP::getWayCounter() { return wayCounter; }
long OSMSAXHandlerCP::getWayRawCounter() { return wayCounterRaw; }
long OSMSAXHandlerCP::getRelationCounter() { return relCounter; }
long OSMSAXHandlerCP::getRelationRawCounter() { return relCounterRaw; }
map<string,bool> OSMSAXHandlerCP::getNeededNodes() { return neededNodes; }
void OSMSAXHandlerCP::setWhitelist(vector<pair<string, string>> whl) { whitelist = whl; }
void OSMSAXHandlerCP::setWriteToFile(bool in) { writeToFile = in; }
void OSMSAXHandlerCP::setPass(bool in) { sndPass = in; }
void OSMSAXHandlerCP::setNewPath(string in) { newfilepath = in; }

void OSMSAXHandlerCP::startDocument(){
    init();
    //cout << "OSMSAXHandlerCP::startDocument" << endl;
    if ( writeToFile ) std::ofstream file { newfilepath.c_str() };
    writeLineToFile("<?xml version='1.0' encoding='UTF-8'?>");
}

void OSMSAXHandlerCP::endDocument(){
    writeLineToFile("</osm>");
    //cout << "OSMSAXHandlerCP::endDocument with " << numerator << " elements" << endl;
}

void OSMSAXHandlerCP::startElement(const string& name, const map<string, string>& attributes) {
    currentDepth++;
    counter = 0;

    auto convertString = [&](string str) {
        string res = "";
        for (size_t i = 0; i < str.length(); i++) {
            if ( str.at(i) == '"' ) res+= "&quot;"; else
            if ( str.at(i) == '\'' ) res+= "&apos;"; else
            if ( str.at(i) == '<' ) res+= "&lt;"; else
            if ( str.at(i) == '>' ) res+= "&gt;"; else
            if ( str.at(i) == '&' ) res+= "&amp;"; else
            res += str.at(i);
        }
        return res;
    };

    auto readAttributes = [&]() {
        string res = " ";
        string tmpKey = ""; //Tag
        string tmpVal = ""; //Tag
        string tmpType = ""; //Relation
        string tmpRef = ""; //Relation
        string tmpRole = ""; //Relation
        for (auto attr : attributes) {
            string key = attr.first;
            string val = attr.second;
            string keyXML = convertString(key);
            string valXML = convertString(val);
            res += keyXML+"='"+valXML+"' ";

            if (key == "id") {
                currentID = val;
                lvl1Open = true;
            }
            if ( currentDepth == 1 ) miscAtts[key] = val;
            if ( name == "tag" ) {
                if ( key == "k" ) { tmpKey = val; }
                if ( key == "v" ) { tmpVal = val; }
            }
            if ( name == "member" ) {
                if ( key == "type" ) { tmpType = val; }
                if ( key == "ref" ) { tmpRef = val; }
                if ( key == "role" ) { tmpRole = val; }
            }
            if ( name == "nd" ) { refsForWays.push_back(val); }
            counter++;
        }
        res.pop_back(); // remove trailing blank
        if ( name == "tag" ) tagsInfo[tmpKey] = tmpVal;
        if ( name == "member" ) {
            if (tmpType == "node") nodesForRelations.push_back(tmpRef);
        }
        return res;
    };

    string atts = readAttributes();
    if ( currentDepth > 0 && currentDepth > lastDepth && lastDepth > 0 ) {
        buffer += ">";
        writeLine(buffer);
    } else
    if ( currentDepth == 1 && currentDepth == lastDepth && !lvl1Closer ) {
        buffer += " />";
        writeLine(buffer);
        handleElement(); ///END OF ELEMENT
    } else
    if ( currentDepth == 1 && currentDepth > lastDepth && lastDepth == 0 ) {
        if ( currentType == -2 ) {
            buffer += " />";
            writeLine(buffer);
            handleElement(); ///END OF ELEMENT
        }
    }


    if ( currentDepth == 0 ) {
        if ( name == "osm" )currentType = -3; //OSM
        buffer = "<";
        buffer += name;
        buffer += readAttributes();
        buffer += ">";
        writeLineToFile(buffer);
        miscAtts.clear();
        tagsInfo.clear();
        element.clear();
    }
    if ( currentDepth == 1 ) {
        if ( counter>0 ) {
            buffer = "  <";
            buffer += name;
            buffer+= atts;
            if ( name == "bounds" ) currentType = -2; else //nodes
            if ( name == "node" ) currentType = 0; else //nodes
            if ( name == "way" ) currentType = 1; else //ways
            if ( name == "relation" ) currentType = 2; else //relations
            currentType = -1; //unknown
        }
    } else
    if ( currentDepth == 2 ) {
        buffer = "    <";
        buffer += name;
        buffer+= readAttributes();
    } else
    if ( currentDepth > 2 ) {
        //cout << "OSMSAXHandlerCP::startElement unknown found: " << msgAsString << endl;
        //cout << "   at level: " << currentDepth << endl;
    }
    lastDepth = currentDepth;
}

void OSMSAXHandlerCP::endElement(const string& name) {
    if ( currentDepth == 0 ) {
        //cout << buffer << endl;
    }
    if ( currentDepth == 1 ) {
        if ( currentDepth < lastDepth  ) {
            buffer = "  </";
            buffer += name;
            buffer +=">";
            lvl1Closer = true;
            writeLine(buffer);
            handleElement(); ///END OF ELEMENT
        }
        if ( currentDepth == lastDepth ) {
            lvl1Closer = false;
        }
        if ( lastDepth == 0 && currentType == 1 ){
            lvl1Closer = false;
        }
    } else
    if ( currentDepth == 2 ) {
        buffer += " />";
        writeLine(buffer);
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandlerCP::startElement unknown found: " << name << endl;
        cout << "   at level: " << currentDepth << endl;
    }

    lastDepth = currentDepth;
    currentDepth--;
}


OSMSAXHandlerBM::OSMSAXHandlerBM() {
}

//BM = Build Map, used for reading and making the file structures of OSM
void OSMSAXHandlerBM::init(){
    numerator = 0;
    nodeCounter = 0;
    wayCounter = 0;
    relCounter = 0;
    counter = 0;
    currentDepth = -1; //Depth: 0 = OSM/XML, 1 = Node/Ways/Relations, 2 = Tags/Refs
    currentType = -1; //error = -1, node = 0, way = 1, relation = 2
    lastDepth = -1;

    lvl1Open = false;
    lvl1Closer = false;
    lvl1Closed = true;

    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
}

void OSMSAXHandlerBM::handleElement(){
    //bounds
    if ( currentType == -2 ) {}

    //nodes
    if ( currentType == 0 ) {
        double lat = atof(miscAtts["lat"].c_str());
        double lon = atof(miscAtts["lon"].c_str());
        OSMNodePtr node = OSMNodePtr( new OSMNode(currentID, lat, lon) );
        node->tags = tagsInfo;
        nodes[node->id] = node;
        nodeCounter++;
    }

    //ways
    if ( currentType == 1 ) {
        OSMWayPtr way = OSMWayPtr( new OSMWay(currentID) );
        way->nodes = refsForWays;
        way->tags = tagsInfo;
        ways[way->id] = way;
        wayCounter++;
    }

    //relations
    if ( currentType == 2 ) {
        OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(currentID) );
        rel->nodes = nodesForRelations;
        rel->ways = waysForRelations;
        rel->tags = tagsInfo;
        relations[rel->id] = rel;
        relCounter++;
    }
    miscAtts.clear();
    tagsInfo.clear();
    refsForWays.clear();
    nodesForRelations.clear();
    waysForRelations.clear();

    element.clear();
    numerator++;
    std::cout << "\r" << std::setw (40) << numerator << " elements";
}

map<string, OSMNodePtr> OSMSAXHandlerBM::getNodes() { return nodes; }
map<string, OSMWayPtr> OSMSAXHandlerBM::getWays()  { return ways; }
map<string, OSMRelationPtr> OSMSAXHandlerBM::getRelations() { return relations; }

long OSMSAXHandlerBM::getNumerator() { return numerator; }
long OSMSAXHandlerBM::getNodeCounter() { return nodeCounter; }
long OSMSAXHandlerBM::getWayCounter() { return wayCounter; }
long OSMSAXHandlerBM::getRelationCounter() { return relCounter; }

void OSMSAXHandlerBM::startDocument(){
    init();
    //cout << "OSMSAXHandlerBM::startDocument" << endl;
}

void OSMSAXHandlerBM::endDocument(){
    //cout << "OSMSAXHandlerBM::endDocument with " << numerator << " elements" << endl;
}

void OSMSAXHandlerBM::startElement(const string& name, const map<string, string>& attributes) {
    currentDepth++;
    counter = 0;

    auto readAttributes = [&]() {
        string tmpKey = ""; //Tag
        string tmpVal = ""; //Tag
        string tmpType = ""; //Relation
        string tmpRef = ""; //Relation
        string tmpRole = ""; //Relation
        for (auto attr : attributes) {
            string key = attr.first;
            string val = attr.second;

            if (key == "id") {
                currentID = val;
                lvl1Open = true;
            }
            if ( currentDepth == 1 ) miscAtts[key] = val;
            if ( name == "tag" ) {
                if ( key == "k" ) { tmpKey = val; }
                if ( key == "v" ) { tmpVal = val; }
            }
            if ( name == "member" ) {
                if ( key == "type" ) { tmpType = val; }
                if ( key == "ref" ) { tmpRef = val; }
                if ( key == "role" ) { tmpRole = val; }
            }
            if ( name == "nd" ) { refsForWays.push_back(val); }
            counter++;
        }
        if ( name == "tag" ) tagsInfo[tmpKey] = tmpVal;
        if ( name == "member" ) {
            if (tmpType == "node") nodesForRelations.push_back(tmpRef);
            if (tmpType == "way") waysForRelations.push_back(tmpRef);
        }
        return;
    };

    readAttributes();
    if ( currentDepth > 0 && currentDepth > lastDepth && lastDepth > 0 ) {
    } else {
        if ( currentDepth == 1 && currentDepth == lastDepth && !lvl1Closer ) {
            handleElement(); ///END OF ELEMENT
        }
    }

    if ( currentDepth == 0 ) {
        currentType = -3; //OSM
    }
    if ( currentDepth == 1 ) {
        if ( counter>0 ) {
            if ( name == "bounds" ) currentType = -2; else //nodes
            if ( name == "node" ) currentType = 0; else //nodes
            if ( name == "way" ) currentType = 1; else //ways
            if ( name == "relation" ) currentType = 2; else //relations
            currentType = -1; //unknown
        }
    } else
    if ( currentDepth == 2 ) {
    } else
    if ( currentDepth > 2 ) {
        //cout << "OSMSAXHandlerBM::startElement unknown found: " << msgAsString << endl;
        //cout << "   at level: " << currentDepth << endl;
    }
    lastDepth = currentDepth;
}

void OSMSAXHandlerBM::endElement(const string& name) {
    if ( currentDepth == 0 ) {
        //cout << buffer << endl;
    }
    if ( currentDepth == 1 ) {
        if ( currentDepth < lastDepth  ) {
            lvl1Closer = true;
            handleElement(); ///END OF ELEMENT
        }
        if ( currentDepth == lastDepth ) {
            lvl1Closer = false;
        }
    } else
    if ( currentDepth == 2 ) {
    } else
    if ( currentDepth > 2 ) {
        cout << "OSMSAXHandlerBM::startElement unknown found: " << name << endl;
        cout << "   at level: " << currentDepth << endl;
    }

    lastDepth = currentDepth;
    currentDepth--;
}


OSMBase::OSMBase(string id) : id(id) {}
OSMNode::OSMNode(string id, double lat, double lon) : OSMBase(id), lat(lat), lon(lon) {}
OSMWay::OSMWay(string id) : OSMBase(id) {}
OSMRelation::OSMRelation(string id) : OSMBase(id) {}

OSMBase::OSMBase(XMLElementPtr el) {
    id = el->getAttribute("id");
    for(auto e : el->getChildren()) { // read node tags
        if (e->getName() == "tag") tags[e->getAttribute("k")] = e->getAttribute("v");
    }
}

string OSMBase::toString() {
    string res = "tags:";
    for (auto t : tags) res += " " + t.first + ":" + t.second;
    return res;
}

string OSMNode::toString() {
    string res = OSMBase::toString();
    res += " N" + ::toString(lat) + " E" + ::toString(lon);
    return res;
}

Vec2d OSMNode::getPosition() { return Vec2d(lat, lon); }

Vec3d OSMNode::getPosition3() { return Vec3d(lat, lon, elevation); }

string OSMWay::toString() {
    string res = OSMBase::toString() + " nodes:";
    for (auto n : nodes) res += " " + n;
    return res;
}

VRPolygon OSMWay::getPolygon() { return polygon; }

vector<string> OSMWay::getNodes(float downSampleRate) {
    if (downSampleRate > 0.999) return nodes;

    vector<string> res;
    res.push_back(nodes[0]); // force first node!

    for (size_t i=0; i<nodes.size(); i++) {
        float k = i*downSampleRate;
        int N = res.size();
        if (N < k) res.push_back(nodes[i]);
    }

    res.push_back(nodes[nodes.size()-1]); // force last node!
    return res;
}

string OSMRelation::toString() {
    string res = OSMBase::toString();
    return res;
}

vector<string> OSMRelation::getNodes() { return nodes; }
vector<string> OSMRelation::getWays() { return ways; }
vector<string> OSMRelation::getRelations() { return relations; }

bool OSMBase::hasTag(const string& t) {
    return tags.count(t) > 0;
}

map<string, string> OSMBase::getTags() { return tags; }
string OSMBase::getID() { return id; }

OSMNode::OSMNode(XMLElementPtr el) : OSMBase(el) {
    toValue(el->getAttribute("lat"), lat);
    toValue(el->getAttribute("lon"), lon);
}

OSMWay::OSMWay(XMLElementPtr el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(auto e : el->getChildren()) {
        if (e->getName() == "tag") continue;
        if (e->getName() == "nd") {
            string nID = e->getAttribute("ref");
            if (invalidIDs.count(nID)) continue;
            nodes.push_back(nID);
            continue;
        }
        cout << " OSMWay::OSMWay, unhandled element: " << e->getName() << endl;
    }
}

OSMRelation::OSMRelation(XMLElementPtr el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(auto e : el->getChildren()) {
        if (e->getName() == "tag") continue;
        if (e->getName() == "member") {
            string type = e->getAttribute("type");
            string eID = e->getAttribute("ref");
            if (invalidIDs.count(eID)) continue;
            if (type == "way") ways.push_back(eID);
            if (type == "node") nodes.push_back(eID);
            continue;
        }
        cout << " OSMRelation::OSMRelation, unhandled element: " << e->getName() << endl;
    }
}

void OSMBase::writeTo(XMLElementPtr e) {
    e->setAttribute("id", ::toString(id));
    e->setAttribute("visible", "true");
    e->setAttribute("version", "1"); // TODO
    e->setAttribute("timestamp", "2019-09-20T17:59:17Z"); // TODO
    //e->setAttribute("changeset", ""); // TODO

    for (auto tag : tags) {
        auto et = e->addChild("tag");
        et->setAttribute("k", tag.first);
        et->setAttribute("v", tag.second);
    }
}

void OSMNode::writeTo(XMLElementPtr e) {
    OSMBase::writeTo(e);
    e->setAttribute("lat", ::toString(lat));
    e->setAttribute("lon", ::toString(lon));
}

void OSMWay::writeTo(XMLElementPtr e) {
    OSMBase::writeTo(e);

    for (auto node : nodes) {
        auto em = e->addChild("nd");
        em->setAttribute("ref", node);
    }
}

void OSMRelation::writeTo(XMLElementPtr e) {
    OSMBase::writeTo(e);

    for (auto node : nodes) {
        auto em = e->addChild("member");
        em->setAttribute("type", "node");
        em->setAttribute("ref", node);
    }

    for (auto way : ways) {
        auto em = e->addChild("member");
        em->setAttribute("type", "way");
        em->setAttribute("ref", way);
    }
}

OSMMap::OSMMap(string filepath) {
    auto getFileSize = [&](string filename) {
        struct stat stat_buf;
        int rc = stat(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : -1;
    };
    if ( getFileSize(filepath) < 10000000 ) readFile(filepath); //10Mb
    else readFileStreaming(filepath);
}

OSMMap::OSMMap() {
}

OSMMapPtr OSMMap::create() {
    return OSMMapPtr( new OSMMap() );
}

OSMMapPtr OSMMap::create(string filepath) {
    return OSMMapPtr( new OSMMap(filepath) );
}

void OSMMap::clear() {
    bounds->clear();
    ways.clear();
    nodes.clear();
}

bool OSMMap::isValid(XMLElementPtr e) {
    if (e->hasAttribute("action")) {
        if (e->getAttribute("action") == "delete") {
            invalidElements[e->getAttribute("id")] = true;
            return false;
        }
    }
    return true;
};

void OSMMap::readFile(string path) {
    filepath = path;
    VRTimer t; t.start();
    bounds = Boundingbox::create();

    XML xml;
    xml.read(filepath);

    auto data = xml.getRoot()->getChildren();
    for (auto element : data) {
        if (!isValid(element)) continue;
        if (element->getName() == "node") { readNode(element); continue; }
        if (element->getName() == "bounds") { readBounds(element); continue; }
        //cout << " OSMMap::readFile, unhandled element: " << element->getName() << endl;
    }
    for (auto element : data) {
        if (!isValid(element)) continue;
        if (element->getName() == "way") { readWay(element, invalidElements); continue; }
        //cout << " OSMMap::readFile, unhandled element: " << element->getName() << endl;
    }
    for (auto element : data) {
        if (!isValid(element)) continue;
        if (element->getName() == "relation") { readRelation(element, invalidElements); continue; }
        //cout << " OSMMap::readFile, unhandled element: " << element->getName() << endl;
    }

    for (auto way : ways) {
        for (auto nID : way.second->nodes) {
            auto n = getNode(nID);
            if (!n) { /*cout << " Error in OSMMap::readFile: no node with ID " << nID << endl;*/ continue; }
            way.second->polygon.addPoint(Vec2d(n->lon, n->lat));
            n->ways.push_back(way.second->id);
        }
    }

    mapType = "OSM";
    auto t2 = t.stop()/1000.0;
    cout << "OSMMap::readFile path " << path << endl;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
    cout << "  secs needed: " << t2 << endl;
}
void OSMMap::checkGDAL(){
#ifdef WITHOUT_GDAL
    cout << "OSMMap - WITHOUT_GDAL" << endl;
#endif // WITHOUT_GDAL
}
void OSMMap::readGEOJSON(string path) {
    checkGDAL();
#ifndef WITHOUT_GDAL
    auto coordsFromString = [&](string inB) {
        //string has format: "lon lat"
        int at1 = inB.find_first_of(" ");
        double lon = toFloat( inB.substr(0, at1).c_str() );
        double lat = toFloat( inB.substr(at1+1, inB.length()-(at1+1)).c_str() );
        return Vec2d(lat,lon);
    };

    auto latlonFromPointString = [&](string in){
        int at0 = in.find_first_of("(")+1;
        return coordsFromString( in.substr(at0, (in.length()-1)-at0) );
    };

    auto multicoordsFromString = [&](string inB){
        //string has format: "lon lat,lon lat,lon lat"
        vector<Vec2d> res;
        bool ende = false;
        int at1 = 0;
        int at2 = 1;
        while (!ende){
            at2 = inB.find_first_of(',', at1);
            if (at2 > 0) {
                res.push_back( coordsFromString( inB.substr(at1, at2-at1) ) );
                at1 = at2+1;
            }
            else {
                res.push_back( coordsFromString( inB.substr(at1, inB.length()-at1) ) );
                ende = true;
            }
        }
        return res;
    };

    auto multipolyFromString = [&](string in) {
        vector<vector<Vec2d>> res;
        int at1 = in.find_first_of("(((") + 3;
        //int at2 = 1;
        int at3 = in.find_first_of(")))");
        int at4 = in.find_first_of(")");
        if (at3 != at4) {
            //TODO
        } else {
            res.push_back( multicoordsFromString( in.substr(at1, at3-at1) ) );
        }
        return res;
    };

    cout << "OSMMap::readGEOJSON path " << path << endl;
    cout << "  GDAL Version Nr:" << GDAL_VERSION_NUM << endl;

    int nodeID = -1;
    int wayID = -1;
    int layercount = -1;
    VRTimer t; t.start();
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpenEx( path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL  );
    //GDALDataset* poDS = (GDALDataset *) GDALOpen( path.c_str(), GA_ReadOnly );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }
    // general information
    printf( "  Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    layercount = poDS->GetLayerCount();

    for (int i = 0; i < layercount; i++) {
        OGRLayer  *poLayer = poDS->GetLayer(i);
        OGRFeature *poFeature;
        poLayer->ResetReading();
        int featureCounter = 0;
        while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
            OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
            map<string, string> tags;
            for( int iField = 0; iField < poFDefn->GetFieldCount(); iField++ ) {
                OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
                tags[poFieldDefn->GetNameRef()] = poFeature->GetFieldAsString(iField);
                //cout << poFieldDefn->GetNameRef() << "-" << poFeature->GetFieldAsString(iField) << endl;
            }

            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
                //OGRPoint *poPoint = poGeometry->toPoint();
                //OGRPoint *poPoint = (OGRPoint *) poGeometry;
                char *wkt_tmp = nullptr;
                poGeometry->exportToWkt(&wkt_tmp);
                Vec2d latlon = latlonFromPointString(wkt_tmp);
                nodeID++;
                string strNID = to_string(nodeID);
                OSMNodePtr node = OSMNodePtr( new OSMNode(strNID, latlon[0], latlon[1] ) );
                node->tags = tags;
                nodes[node->id] = node;
                //bounds->update(Vec3d(latlon[1],latlon[0],0));
            }
            else if ( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPolygon ) {
                //OGRMultiPolygon* poMPoly = (OGRMultiPolygon *) poGeometry;
                char *wkt_tmp = nullptr;
                poGeometry->exportToWkt(&wkt_tmp);
                vector<vector<Vec2d>> multiPoly = multipolyFromString(wkt_tmp);
                vector<string> refsForWays;
                for (auto eachPoly : multiPoly) {
                    refsForWays.clear();
                    for (auto eachPoint : eachPoly) {
                        nodeID++;
                        string strNID = to_string(nodeID);
                        OSMNodePtr node = OSMNodePtr( new OSMNode(strNID, eachPoint[0], eachPoint[1] ) );
                        refsForWays.push_back(strNID);
                        nodes[node->id] = node;
                        //bounds->update(Vec3d(eachPoint[1],eachPoint[0],0));
                    }
                    wayID++;
                    string strWID = to_string(wayID);
                    OSMWayPtr way = OSMWayPtr( new OSMWay(strWID) );
                    way->nodes = refsForWays;
                    way->tags = tags;
                    ways[way->id] = way;
                }
            }
            else {
                printf( "no point or multipolygon geometry\n" );
            }
            featureCounter++;
        }
        //OGRFeature::DestroyFeature( poFeature );
    }
    mapType = "geoJSON";
    GDALClose(poDS);
#endif // GDAL_VERSION_NUM
    auto t2 = t.stop()/1000.0;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
    cout << "  secs needed: " << t2 << endl;
#endif // WITHOUT_GDAL
}

void OSMMap::readSHAPE(string path) {
    checkGDAL();
#ifndef WITHOUT_GDAL
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
    VRTimer t; t.start();
    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpenEx( path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL  );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }
    // general information
    cout << "OSMMap::readSHAPE path " << path << endl;
    printf( "  Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    //int nodeID = -1;
    //int wayID = -1;

    GDALClose(poDS);
    auto t2 = t.stop()/1000.0;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
    cout << "  secs needed: " << t2 << endl;
#endif // GDAL_VERSION_NUM
#endif // WITHOUT_GDAL
}

Vec2d OSMMap::convertCoords(double northing, double easting, int EPSG_Code_in, int EPSG_Code_out) {
    checkGDAL();
#ifndef WITHOUT_GDAL
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
    GDALAllRegister();
    OGRSpatialReference source, target;

    source.importFromEPSG(EPSG_Code_in);
    target.importFromEPSG(EPSG_Code_out); //4326 is lat lon

    OGRPoint p;
    p.setX(easting);
    p.setY(northing);
    p.assignSpatialReference(&source);

    p.transformTo(&target);

    //cout << " new " << p.getX() << " | " << p.getY();
    return Vec2d(p.getY(), p.getX());
#endif // GDAL_VERSION_NUM
#endif // WITHOUT_GDAL
#ifdef WITHOUT_GDAL
    return Vec2d(0.0,0.0);
#endif // WITHOUT_GDAL
}

void OSMMap::readGML(string path, int EPSG_Code) {
    checkGDAL();
#ifndef WITHOUT_GDAL
    cout << "OSMMap::readGML path " << path << endl;
    cout << "  GDAL Version Nr:" << GDAL_VERSION_NUM << endl;

    auto coordsFromString = [&](string inB) {
        //string has format: "x y z"
        int at1 = inB.find_first_of(" ");
        int at2 = inB.find_first_of(" ",at1+1);
        string rechtswert = inB.substr(0, at1).c_str();
        string hochwert = inB.substr(at1+1, at2-(at1+1)).c_str();
        double elevation = toFloat( inB.substr(at2+1, inB.length()-(at2+1)).c_str() );
        return Vec3d( atof(hochwert.c_str()), atof(rechtswert.c_str()), elevation);
    };

    auto multicoordsFromString = [&](string inB){
        //string has format: "x y z,x y z,x y z"
        vector<Vec3d> res;
        bool ende = false;
        int at1 = 0;
        int at2 = 1;
        while (!ende){
            at2 = inB.find_first_of(',', at1);
            if (at2 > 0) {
                res.push_back( coordsFromString( inB.substr(at1, at2-at1) ) );
                at1 = at2+1;
            }
            else {
                res.push_back( coordsFromString( inB.substr(at1, inB.length()-at1) ) );
                ende = true;
            }
        }
        return res;
    };

    auto multipolyFromString = [&](string in) {
        vector<vector<Vec3d>> res;
        int at1 = in.find_first_of("(((") + 3;
        //int at2 = 1;
        int at3 = in.find_first_of(")))");
        int at4 = in.find_first_of(")");
        if (at3 != at4) {
            //TODO
        } else {
            res.push_back( multicoordsFromString( in.substr(at1, at3-at1) ) );
        }
        return res;
    };

    int nodeID = -1;
    //int wayID = -1;
    int layercount = -1;
    VRTimer t; t.start();

// GDALOpenEx is available since GDAL 2.0
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
    string workingpath = "GMLAS:"+path;

    GDALAllRegister();
    GDALDataset* poDS = (GDALDataset *) GDALOpenEx( workingpath.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL  );
    //GDALDataset* poDS = (GDALDataset *) GDALOpenEx( path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL  );
    if( poDS == NULL ) { printf( "Open failed.\n" ); return; }
    // general information
    printf( "  Driver: %s/%s\n", poDS->GetDriver()->GetDescription(), poDS->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

    layercount = poDS->GetLayerCount();
    int featureCounter = 0;
    map<string,string> pkIDs;
    map<string,map<string,string>> pkIDtoTags;
    for (int i = 0; i < layercount; i++) {
        bool nlnG = true;
        OGRLayer  *poLayer = poDS->GetLayer(i);
        OGRFeature *poFeature;
        poLayer->ResetReading();
        int localCounter = 0;
        while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
            localCounter++;
            OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
            map<string, string> tags;
            //cout << " --- " << endl;
            for( int iField = 0; iField < poFDefn->GetFieldCount(); iField++ ) {
                OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );
                tags[poFieldDefn->GetNameRef()] = poFeature->GetFieldAsString(iField);
                //if (poFieldDefn->GetNameRef() == "ogr_pkid") pkIDs[poFeature->GetFieldAsString(iField)] = poLayer->GetName();
                //cout << poFieldDefn->GetNameRef() << "-" << poFeature->GetFieldAsString(iField) << endl;
            }

            for (auto each:tags) {
                if (each.first == "ogr_pkid") {
                    pkIDtoTags[each.second] = tags;
                }
            }

            OGRGeometry *poGeometry;
            poGeometry = poFeature->GetGeometryRef();
            if( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint ) {
                cout << " polygon" << endl;
            }
            else if ( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPolygon ) {
                //cout << " multipolygon" << endl;
                //OGRMultiPolygon* poMPoly = (OGRMultiPolygon *) poGeometry;
                char *wkt_tmp = nullptr;
                poGeometry->exportToWkt(&wkt_tmp);
                vector<vector<Vec3d>> multiPoly = multipolyFromString(wkt_tmp);
                vector<string> refsForWays;
                string wID = "";

                for (auto each:tags) {
                    if (each.first == "ogr_pkid") {
                        pkIDs[each.second] = poLayer->GetName();
                        wID = each.second;
                    }
                }

                for (auto eachPoly : multiPoly) {
                    refsForWays.clear();
                    for (auto eachPoint : eachPoly) {
                        nodeID++;
                        string strNID = to_string(nodeID);
                        Vec2d latlon = convertCoords(eachPoint[0], eachPoint[1], EPSG_Code, EPSG_LATLON);
                        OSMNodePtr node = OSMNodePtr( new OSMNode(strNID, latlon[0], latlon[1] ) );
                        refsForWays.push_back(strNID);
                        nodes[node->id] = node;
                        node->elevation = eachPoint[2];
                        //bounds->update(Vec3d(eachPoint[1],eachPoint[0],0));
                        //cout << eachPoint << " ";
                    }
                    //wayID++;
                    //string strWID = to_string(wayID);
                    OSMWayPtr way = OSMWayPtr( new OSMWay(wID) );
                    way->nodes = refsForWays;
                    way->tags = tags;
                    ways[way->id] = way;
                    nlnG = false;
                }
                //cout << endl;
                //cout << i <<  " F: "<< featureCounter << endl;
            }
            else {
                //printf( "no point or multipolygon geometry\n" );
                cout << poLayer->GetName() << " = ";
                for (auto each:tags) {
                    cout << each.first << ":" << each.second << " ";
                    if (each.first == "ogr_pkid") pkIDs[each.second] = poLayer->GetName();
                }
                cout << endl;

                ///TODO: implement everything else as relations
                if ( tags.count("texcoordlist_texturecoordinates") ){

                }
                if ( tags.count("") ){

                }
                if ( tags.count("") ){

                }
                if ( tags.count("") ){

                }
                /*Relations:
                Building - a gathering of polygons
                Building bounded by - gathering of polygons
                Textures - materials and links to material as gathering for polygons (ways)
                TextureCoordinates - meta data of polygons (ways)
                */
            }
            featureCounter++;
        }

        if (nlnG && localCounter>0) {
            //cout << poLayer->GetName() << endl;
            cout << "------new Layer------" << poLayer->GetName() << endl;
        }
        //OGRFeature::DestroyFeature( poFeature );
    }
    for (auto each : pkIDs) {
        if (each.second == "texcoordlist") {}
        if (each.second == "texcoordlist_texturecoordinates") { cout << "COORDINATES---" << each.first << " type: " << each.second << endl; }
        if (each.second == "building_name") {}
        if (each.second == "x3dmaterial") { cout << "MATERIAL---" << each.first << " type: " << each.second << endl; }
        if (each.second == "parameterizedtexture") { cout << "URI HERE---" << each.first << " type: " << each.second << endl; }
        if (each.second == "parameterizedtexture_target") {}
        //cout << each.first << " type: " << each.second << endl;
    }

    for (auto outer : pkIDtoTags) {
        if (outer.second.count("imageuri")) {
            cout << "found imageuri: " << outer.second["imageuri"] << endl;
            if (outer.second.count("imageuri")) {
                //cout << "" << endl;
            }
        }

        if (pkIDs[outer.first] == "parameterizedtexture_target") {
            //cout << "found everything" << endl;
            cout << " TCL " << outer.second["_textureparameterization_texcoordlist_pkid"] << " -- ";
            cout << " Par " << outer.second["parent_ogr_pkid"] << " -- ";
            cout << " uri " << outer.second["uri"];
        }
        for (auto inner : outer.second) {

            /*for (auto each : inner) {
                if (each.first == "target uri") { }
                if (each.first == "") { }
            }*/
        }
    }

    mapType = "GML";
    GDALClose(poDS);
    cout << "Layers: " << layercount << " Features: " << featureCounter << endl;
#endif // GDAL_VERSION_NUM
    auto t2 = t.stop()/1000.0;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
    cout << "  secs needed: " << t2 << endl;
#endif // WITHOUT_GDAL
}

void OSMMap::writeFile(string path) {
    XML xml;

    auto root = xml.newRoot("osm", "", "");
    root->setAttribute("version", "0.6");
    root->setAttribute("upload", "false");
    root->setAttribute("generator", "PolyVR");

    writeBounds(root);
    for (auto node : nodes) if (node.second) node.second->writeTo( root->addChild("node") );
    for (auto way : ways) if (way.second) way.second->writeTo( root->addChild("way") );
    for (auto rel : relations) if (rel.second) rel.second->writeTo( root->addChild("relation") );
    xml.write(path);
}

int OSMMap::readFileStreaming(string path) {
    filepath = path;
    VRTimer t; t.start();

    XML xml;
    OSMSAXHandlerBM* docHandler = new OSMSAXHandlerBM();

    cout << "OSMMap::readFileStreaming - " << filepath << endl;
    std::cout << std::setw (40) << "0 elements";
    xml.stream(path, docHandler);
    nodes = docHandler->getNodes();
    ways = docHandler->getWays();
    relations = docHandler->getRelations();
    mapType = "OSM";
    cout << "\r";
    cout << "OSMMap::readFileStreaming - elements read: " << docHandler->getNumerator() << endl;
    cout << "OSMMap::readFileStreaming - nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;

    delete docHandler;
    auto t2 = t.stop()/1000.0;
    cout << "OSMMap::readFileStreaming - secs needed: " << t2 << endl;
    return 0;
}

int OSMMap::filterFileStreaming(string path, vector<pair<string, string>> whitelist) {
    filepath = path;

    XML xml;

    //DocumentHandler* docHandler = new OSMSAXHandlerCP();
    OSMSAXHandlerCP* docHandler = new OSMSAXHandlerCP();
    docHandler->setWhitelist(whitelist);

    auto genPath = [&](string in){
        string osm = "";
        string res = "";
        for (int i = 0; i < 4; i++){
            osm = filepath.at(filepath.length()-1-i) + osm;
        }

        if (osm != ".osm") {
            res+="newOSM.osm";
            return res;
        }
        for (unsigned int i = 0; i+4 < filepath.length(); i++) {
            res += filepath.at(i);
        }
        res += in;
        res += ".osm";
        return res;
    };

    string tempPath = genPath("_temp");
    VRTimer t; t.start();
    cout << "OSMMap::copyFileStreaming 1st Pass - " << filepath << endl;
    std::cout << std::setw (40) << "0 elements";
    docHandler->setNewPath(tempPath);
    xml.stream(path, docHandler);
    auto t2 = t.stop()/1000;
    cout << "\r";
    cout << "OSMMap::copyFileStreaming 1st Pass - secs needed: " << t2 << endl;
    cout << "OSMMap::copyFileStreaming 1st Pass - elements read: " << docHandler->getNumerator() << ", elements written: " << docHandler->getNumeratorWritten() << endl;
    cout << "OSMMap::copyFileStreaming 1st Pass - read: nodes: " << docHandler->getNodeRawCounter() << ", ways: " << docHandler->getWayRawCounter() <<  ", relations: " << docHandler->getRelationRawCounter() << endl;
    cout << "OSMMap::copyFileStreaming 1st Pass - writ: nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;

    t.start();
    docHandler->setPass(true);
    docHandler->setNewPath(genPath("_filtered"));
    cout << "OSMMap::copyFileStreaming 2nd Pass - " << filepath << endl;
    xml.stream(tempPath, docHandler);
    auto t3 = t.stop()/1000;
    cout << "OSMMap::copyFileStreaming 2nd Pass - secs needed: " << t3 << endl;
    cout << "OSMMap::copyFileStreaming 2nd Pass - elements read: " << docHandler->getNumerator() << ", elements written: " << docHandler->getNumeratorWritten() << endl;
    cout << "OSMMap::copyFileStreaming 2nd Pass - read: nodes: " << docHandler->getNodeRawCounter() << ", ways: " << docHandler->getWayRawCounter() <<  ", relations: " << docHandler->getRelationRawCounter() << endl;
    cout << "OSMMap::copyFileStreaming 2nd Pass - writ: nodes: " << docHandler->getNodeCounter() << ", ways: " << docHandler->getWayCounter() <<  ", relations: " << docHandler->getRelationCounter() << endl;

    delete docHandler;
    return 0;
}

void OSMMap::filterFileStreaming(string path, vector<vector<string>> wl) {
    vector<pair<string, string>> whitelist;
    for (auto each : wl){
        whitelist.push_back(pair<string, string>(each[0],each[1]));
    }
    /*
    vector<pair<string, string>> whitelist;
    whitelist.push_back(pair<string, string>("admin_level","2"));
    whitelist.push_back(pair<string, string>("admin_level","4"));
    whitelist.push_back(pair<string, string>("admin_level","5"));
    whitelist.push_back(pair<string, string>("admin_level","6"));
    whitelist.push_back(pair<string, string>("highway","motorway"));
    whitelist.push_back(pair<string, string>("railway","rail"));
    whitelist.push_back(pair<string, string>("place","city"));*/
    filterFileStreaming(path, whitelist);
}

template <class Key, class Value>
unsigned long mapSize(const map<Key,Value> &map){
    unsigned long size = 0;
    for(auto it = map.begin(); it != map.end(); ++it){
        size += it->first.capacity();
        size += sizeof(it->second);
    }
    return size;
}

template <class Value>
unsigned long vecSize(const std::vector<Value> &vec){
    unsigned long size = 0;
    for(auto it = vec.begin(); it != vec.end(); ++it){
        size += it->capacity();
    }
    return size;
}

double OSMMap::getMemoryConsumption() {
    double res = sizeof(*this);

    res += filepath.capacity();
    res += sizeof(*bounds);
    res += mapSize(ways);
    res += mapSize(nodes);
    res += mapSize(relations);
    res += mapSize(invalidElements);

    for (auto& w : ways) if (w.second) res += mapSize(w.second->tags) + vecSize(w.second->nodes);
    for (auto& n : nodes) if (n.second) res += mapSize(n.second->tags) + vecSize(n.second->ways);
    for (auto& r : relations) if (r.second) res += mapSize(r.second->tags) + vecSize(r.second->ways) + vecSize(r.second->nodes);

    return res/1048576.0;
}

OSMMapPtr OSMMap::loadMap(string filepath) { return OSMMapPtr( new OSMMap(filepath) ); }

map<string, OSMWayPtr> OSMMap::getWays() { return ways; }
map<string, OSMNodePtr> OSMMap::getNodes() { return nodes; }
map<string, OSMRelationPtr> OSMMap::getRelations() { return relations; }
OSMNodePtr OSMMap::getNode(string id) { return nodes[id]; }
OSMWayPtr OSMMap::getWay(string id) { return ways[id]; }
OSMRelationPtr OSMMap::getRelation(string id) { return relations[id]; }
void OSMMap::reload() { clear(); readFile(filepath); }

OSMMapPtr OSMMap::subArea(double latMin, double latMax, double lonMin, double lonMax) {
    auto map = OSMMap::create();

    map->bounds = Boundingbox::create();
    map->bounds->update(Vec3d(lonMin,latMin,0));
    map->bounds->update(Vec3d(lonMax,latMax,0));

    vector<string> validNodes;
    for (auto n : nodes) {
        if (!n.second) continue;
        bool isOutside = bool(n.second->lat < latMin || n.second->lat > latMax || n.second->lon < lonMin || n.second->lon > lonMax);
        if (!isOutside) {
            map->nodes[n.first] = n.second;
            validNodes.push_back(n.first);
        }
    }

    auto isValidNode = [&](string nID) {
        return bool(find(validNodes.begin(), validNodes.end(), nID) != validNodes.end());
    };

    auto isValidWay = [&](OSMWayPtr w) {
        if (!w) return false;
        for (auto n : w->nodes) if (isValidNode(n)) return true;
        return false;
    };

    auto isValidRelation = [&](OSMRelationPtr r) {
        if (!r) return false;
        for (auto n : r->nodes) if (isValidNode(n)) return true;
        for (auto w : r->ways) if (isValidWay( getWay(w) )) return true;
        return false;
    };

    for (auto w : ways) {
        if (isValidWay(w.second)) {
            auto w2 = OSMWayPtr(new OSMWay(w.first));
            *w2 = *w.second;
            w2->nodes.clear();
            for (auto n : w.second->nodes) if(isValidNode(n)) w2->nodes.push_back(n);
            map->ways[w.first] = w2;
        }
    }

    for (auto r : relations) {
        if (isValidRelation(r.second)) {
            auto r2 = OSMRelationPtr(new OSMRelation(r.first));
            *r2 = *r.second;
            r2->nodes.clear();
            for (auto n : r.second->nodes) if(isValidNode(n)) r2->nodes.push_back(n);
            r2->ways.clear();
            for (auto w : r.second->ways) if(isValidWay(getWay(w))) r2->ways.push_back(w);
            map->relations[r.first] = r2;
        }
    }

    map->mapType = mapType;
    return map;
}

vector<OSMWayPtr> OSMMap::splitWay(OSMWayPtr way, int segN) {
    vector<OSMWayPtr> res;
    int segL = way->nodes.size()/segN;

    unsigned int k = 0;
    OSMWayPtr w = 0;
    for (int s=0; s<segN && k<way->nodes.size(); s++) {
        w = OSMWayPtr( new OSMWay(way->id + "_" + toString(s)) );
        w->tags = way->tags;
        ways[w->id] = w;
        for (int i=0; i<segL; i++) {
            if (i == 0 && k != 0) k--;
            w->nodes.push_back( way->nodes[k] );
            k++;
            if (k >= way->nodes.size()) break;
        }
        for (auto n : w->nodes) {
            int N = nodes[n]->ways.size();
            for (int i=0; i<N; i++) {
                if (nodes[n]->ways[i] == way->id) { nodes[n]->ways[i] = w->id; break; }
                else if (i == N-1) { nodes[n]->ways.push_back(w->id); break; }
            }
        }
        res.push_back(w);
    }

    ways.erase(way->id);
    for (; k<way->nodes.size(); k++) w->nodes.push_back( way->nodes[k] );
    return res;
}

void OSMMap::readBounds(XMLElementPtr element) {
    Vec3d min(toFloat( element->getAttribute("minlon") ), toFloat( element->getAttribute("minlat") ), 0 );
    Vec3d max(toFloat( element->getAttribute("maxlon") ), toFloat( element->getAttribute("maxlat") ), 0 );
    bounds->clear();
    bounds->update(min);
    bounds->update(max);
}

void OSMMap::writeBounds(XMLElementPtr parent) {
    auto element = parent->addChild("bounds");
    element->setAttribute("minlon", ::toString( bounds->min()[0]) );
    element->setAttribute("minlat", ::toString( bounds->min()[1]) );
    element->setAttribute("maxlon", ::toString( bounds->max()[0]) );
    element->setAttribute("maxlat", ::toString( bounds->max()[1]) );
}

void OSMMap::readNode(XMLElementPtr element) {
    OSMNodePtr node = OSMNodePtr( new OSMNode(element) );
    nodes[node->id] = node;
}

void OSMMap::readWay(XMLElementPtr element, map<string, bool>& invalidIDs) {
    OSMWayPtr way = OSMWayPtr( new OSMWay(element, invalidIDs) );
    ways[way->id] = way;
}

void OSMMap::readRelation(XMLElementPtr element, map<string, bool>& invalidIDs) {
    OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(element, invalidIDs) );
    relations[rel->id] = rel;
}
