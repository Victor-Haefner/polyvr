#include "xml.h"

#include <iostream>
#include <libxml/tree.h>


XMLElement::XMLElement(_xmlNode* node) : node(node) {}
XMLElement::~XMLElement() {}

XMLElementPtr XMLElement::create(_xmlNode* node) { return XMLElementPtr( new XMLElement(node) ); }

void XMLElement::print() {
    cout << "xml node: '" << getName() << "', ns: '" << getNameSpace() << "', data: " << getText();
    for (auto a : getAttributes()) cout << ", " << a.first << ":" << a.second;
    cout << endl;
}

string XMLElement::getName() {
    if (!node || ! node->name) return "";
    return string((const char*)node->name);
}

string XMLElement::getNameSpace() {
    if (!node || ! node->ns) return "";
    return string((const char*)node->ns);
}

string XMLElement::getText() {
    if (!hasText()) return "";
    auto txt = xmlNodeGetContent( node->children );
    auto res = string((const char*)txt);
    xmlFree(txt);
    return res;
}

bool XMLElement::hasText() {
    if (!node || !node->children) return false;
    auto txt = xmlNodeGetContent( node->children );
    if (!txt) return false;
    return true;
}

string XMLElement::getAttribute(string name) {
    auto a = xmlGetProp(node, (xmlChar*)name.c_str());
    string v = a ? string((const char*)a) : "";
    if (a) xmlFree(a);
    return v;
}

bool XMLElement::hasAttribute(string name) {
    auto a = xmlHasProp(node, (xmlChar*)name.c_str());
    return a != 0;
}

map<string,string> XMLElement::getAttributes() {
    map<string,string> res;
    xmlAttr* attribute = node->properties;
    while(attribute) {
        xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
        string ans((const char*)attribute->name);
        string vas((const char*)value);
        res[ans] = vas;
        xmlFree(value);
        attribute = attribute->next;
    }
    return res;
}

void XMLElement::setAttribute(string name, string value) {
    xmlSetProp(node, (xmlChar*)name.c_str(), (xmlChar*)value.c_str());
}

_xmlNode* getNextNode(_xmlNode* cur) {
    while ( cur && xmlIsBlankNode(cur) ) cur = cur->next;
    return cur;
}

vector<XMLElementPtr> XMLElement::getChildren(string name) {
    vector<XMLElementPtr> res;
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        if (cnode->type != XML_ELEMENT_NODE) continue;
        if (name != "" && name != string((const char*)cnode->name)) continue;
        res.push_back(XMLElement::create(cnode));
        cnode = getNextNode( cnode->next );
    }
    return res;
}

XMLElementPtr XMLElement::getChild(string name) {
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        if (cnode->type != XML_ELEMENT_NODE) continue;
        if (name == string((const char*)cnode->name)) return XMLElement::create(cnode);
        cnode = getNextNode( cnode->next );
    }
    return 0;
}

XMLElementPtr XMLElement::getChild(int i) {
    int k = 0;
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        if (cnode->type != XML_ELEMENT_NODE) continue;
        if (k == i) return XMLElement::create(cnode);
        cnode = getNextNode( cnode->next );
        k++;
    }
    return 0;
}

XMLElementPtr XMLElement::addChild(string name) {
    auto child = xmlNewNode(NULL, (xmlChar*)name.c_str());
    xmlAddChild(node, child);
    return XMLElement::create(child);
}

void XMLElement::setText(string text) {
    auto child = xmlNewText((xmlChar*)text.c_str());
    xmlAddChild(node, child);
}

void XMLElement::importNode(XMLElementPtr e, bool recursive, XML& xml) {
    if (!node) return;
    auto inode = xmlDocCopyNode(e->node, xml.doc, recursive);
    xmlAddChild(node, inode);
}




XML::XML() {}
XML::~XML() {
    if (doc) xmlFreeDoc(doc);
}

XMLPtr XML::create() { return XMLPtr( new XML() ); }

void XML::read(string path, bool validate) {
    if (doc) xmlFreeDoc(doc);
    // parser.set_validate(false); // TODO!
    doc = xmlParseFile(path.c_str());
    xmlNodePtr xmlRoot = xmlDocGetRootElement(doc);
    root = XMLElement::create(xmlRoot);
}

void XML::parse(string data, bool validate) {
    if (doc) xmlFreeDoc(doc);
    // parser.set_validate(false); // TODO!
    doc = xmlParseMemory(data.c_str(), data.size());
    xmlNodePtr xmlRoot = xmlDocGetRootElement(doc);
    root = XMLElement::create(xmlRoot);
}

void XML::write(string path) {
    //xmlSaveFormatFile(path.c_str(), doc, 1);
    xmlKeepBlanksDefault(0);
    xmlSaveFormatFileEnc(path.c_str(), doc, "ISO-8859-1", 1);
}

string XML::toString() {
    xmlKeepBlanksDefault(0);
    xmlBuffer* buffer = xmlBufferCreate();
    xmlOutputBuffer* outputBuffer = xmlOutputBufferCreateBuffer( buffer, NULL );
    xmlSaveFormatFileTo(outputBuffer, doc, "ISO-8859-1", 1);
    string str( (char*) buffer->content, buffer->use );
    xmlBufferFree( buffer );
    return str;
}

XMLElementPtr XML::getRoot() { return root; }

XMLElementPtr XML::newRoot(string name, string ns_uri, string ns_prefix) {
    if (doc) xmlFreeDoc(doc);
    doc = xmlNewDoc(NULL);
    auto ns = xmlNewNs(NULL, (xmlChar*)ns_uri.c_str(), (xmlChar*)ns_prefix.c_str());
    auto rnode = xmlNewNode(ns, (xmlChar*)name.c_str());
    root = XMLElement::create( rnode );
    return root;
}

void XML::printTree(XMLElementPtr e, string D) {
    cout << D << e->getName() << endl;
    for (auto c : e->getChildren()) printTree(c, D + " ");
}




