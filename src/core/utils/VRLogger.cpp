#include "VRLogger.h"
#include "core/gui/VRGuiManager.h"

map<string, bool> VRLog::tags;

void VRLog::print(string tag, string s) {
    if (!tags[tag]) return;

    OSG::VRGuiManager::get()->printToConsole("Console", s);
}

void VRLog::setTag(string tag, bool b) { tags[tag] = b; }
bool VRLog::tag(string tag) { return tags[tag]; }

void VRLog::log(string tag, string s) { print(tag, "Log: "+s); }
void VRLog::wrn(string tag, string s) { print(tag, "Warning: "+s); }
void VRLog::err(string tag, string s) { print(tag, "Error: "+s); }
