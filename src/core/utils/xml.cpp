#include "xml.h"
#include "toString.h"

#include <iostream>
#include <libxml/tree.h>


XMLElement::XMLElement(_xmlNode* node) : node(node) {}
XMLElement::~XMLElement() {}

XMLElementPtr XMLElement::create(_xmlNode* node) { return XMLElementPtr( new XMLElement(node) ); }

void XMLElement::print() {
    if (!node) cout << "invalid xml node!";
    else {
        cout << "xml node: '" << getName() << "', ns: '" << getNameSpace() << "', data: " << getText();
        for (auto a : getAttributes()) cout << ", " << a.first << ":" << a.second;
    }
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
    if (!node) return res;
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

string getXMLError() {
    auto error = xmlGetLastError();
    if (!error || error->code == XML_ERR_OK) return ""; // No error

    string str;

    if (error->file && *error->file != '\0') { str += "File "; str += error->file; }

    if (error->line > 0) {
        str += (str.empty() ? "Line " : ", line ") + toString(error->line);
        if (error->int2 > 0) str += ", column " + toString(error->int2);
    }

    const bool two_lines = !str.empty();
    if (two_lines) str += ' ';

    switch (error->level) {
        case XML_ERR_WARNING:
            str += "(warning):";
            break;
        case XML_ERR_ERROR:
            str += "(error):";
            break;
        case XML_ERR_FATAL:
            str += "(fatal):";
            break;
        default:
            str += "():";
            break;
    }

    str += two_lines ? '\n' : ' ';
    if (error->message && *error->message != '\0') str += error->message;
    else str += "Error code " + toString(error->code);
    if (*str.rbegin() != '\n') str += '\n';
    return str;
}

void XML::write(string path) {
    auto r = xmlDocGetRootElement(doc);
    if (!r) { cout << "XML::write failed, no root: " << path << endl; return; }
    xmlKeepBlanksDefault(0);
    xmlIndentTreeOutput = 1;
    xmlResetLastError();
    const int result = xmlSaveFormatFileEnc(path.c_str(), doc, "UTF-8", 1);
    if (result == -1) cout << "XML::write error: " << getXMLError() << endl;
}

string XML::toString() {
    xmlKeepBlanksDefault(0);
    xmlBuffer* buffer = xmlBufferCreate();
    xmlOutputBuffer* outputBuffer = xmlOutputBufferCreateBuffer( buffer, NULL );
    xmlSaveFormatFileTo(outputBuffer, doc, "UTF-8", 1);
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
    xmlDocSetRootElement(doc, rnode);
    return root;
}

void XML::printTree(XMLElementPtr e, string D) {
    cout << D << e->getName() << endl;
    for (auto c : e->getChildren()) printTree(c, D + " ");
}




