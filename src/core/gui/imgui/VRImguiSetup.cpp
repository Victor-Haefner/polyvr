#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/imWidgets/VRImguiInput.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager"),
        tree("setup"),
        viewPosition("viewPos", "Area"),
        viewSize("viewSize", "Size"),
        eyeSeparation("eyeSep", "Eye separation [m]", "0.06"),
        viewProjUser("viewProjUser", "User"),
        viewProjCenter("viewProjCenter", "Center"),
        viewProjNormal("viewProjNormal", "Normal"),
        viewProjUp("viewProjUp", "Up"),
        viewProjSize("viewProjSize", "Size"),
        viewProjShear("viewProjShear", "Shear"),
        viewProjWarp("viewProjWarp", "Warp"),
        viewMirrorPos("viewMPos", "Position"),
        viewMirrorNorm("viewMNorm", "Normal"),
        NxNy("NxNy", "Nx Ny"),
        windowMSAA("winMSAA", "MSAA"),
        windowSize("winSize", "Size"),
        windowMouse("winMouse", "Mouse"),
        windowMultitouch("winMultitouch", "Multitouch"),
        windowKeyboard("winKeyboard", "Keyboard"),
        slaveConnectionType("slaveConnType", "Connection Type"),
        slaveSystemScreens("slaveSystemScreens", ""),
        winConnectionType("winConnType", "Connection Type"),
        artOffset("artOffset", "Offset"),
        artAxis("artAxis", "Axis") {

    windowMSAA.setList({"none", "x2", "x4", "x8", "x16"});
    slaveSystemScreens.setList({":0.0", ":0.1", ":1.0", ":1.1"});

    //vector<string> ctypes = {"Multicast", "SockPipeline", "StreamSock"};
    vector<string> ctypes = {"Multicast", "StreamSock"};
    slaveConnectionType.setList(ctypes);
    winConnectionType.setList(ctypes);

    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("updateSetupsList", [&](OSG::VRGuiSignals::Options o){ updateSetupsList(o["setups"]); return true; } );
    mgr->addCallback("updateViewTrackersList", [&](OSG::VRGuiSignals::Options o){ updateViewTrackersList(o["trackers"]); return true; } );
    mgr->addCallback("setCurrentSetup", [&](OSG::VRGuiSignals::Options o){ current_setup = toInt(o["setup"]); return true; } );
    mgr->addCallback("on_setup_tree_clear", [&](OSG::VRGuiSignals::Options o){ tree.clear(); return true; } );
    mgr->addCallback("on_setup_tree_append", [&](OSG::VRGuiSignals::Options o) { treeAppend(o["ID"], o["label"], o["type"], o["parent"]); return true; } );
    mgr->addCallback("on_setup_tree_expand", [&](OSG::VRGuiSignals::Options o) { tree.expandAll(); return true; } );

    mgr->addCallback("on_setup_select_clear", [&](OSG::VRGuiSignals::Options o) { hideAll(); return true; } );
    mgr->addCallback("on_setup_select_view", [&](OSG::VRGuiSignals::Options o) { selectView(o); return true; } );
    mgr->addCallback("on_setup_select_window", [&](OSG::VRGuiSignals::Options o) { selectWindow(o); return true; } );
    mgr->addCallback("on_setup_select_multiwindow", [&](OSG::VRGuiSignals::Options o) { selectMultiWindow(o); return true; } );
    mgr->addCallback("on_setup_select_node", [&](OSG::VRGuiSignals::Options o) { selectNode(o); return true; } );
    mgr->addCallback("on_setup_select_slave", [&](OSG::VRGuiSignals::Options o) { selectSlave(o); return true; } );
    mgr->addCallback("on_setup_select_art_device", [&](OSG::VRGuiSignals::Options o) { selectARTDevice(o); return true; } );
    mgr->addCallback("on_setup_select_art", [&](OSG::VRGuiSignals::Options o) { selectART(o); return true; } );

    mgr->addCallback("updateMouseList", [&](OSG::VRGuiSignals::Options o) { windowMouse.setList(o["list"]); return true; } );
    mgr->addCallback("updateMTouchList", [&](OSG::VRGuiSignals::Options o) { windowMultitouch.setList(o["list"]); return true; } );
    mgr->addCallback("updateKeyboardList", [&](OSG::VRGuiSignals::Options o) { windowKeyboard.setList(o["list"]); return true; } );
    mgr->addCallback("updateDisplayList", [&](OSG::VRGuiSignals::Options o) { slaveSystemScreens.setList(o["list"]); return true; } );

    mgr->addCallback("state_multiwindow_updated", [&](OSG::VRGuiSignals::Options o) { setWindowState(o["window"], o["state"]); return true; } );

    tree.setNodeFlags( ImGuiTreeNodeFlags_DefaultOpen );
}

void ImSetupManager::selectART(map<string,string> o) {
    hideAll();
    showART = true;

    artPort = toInt(o["port"]);
    artActive = toBool(o["active"]);
    artOffset.set3( o["offset"] );
    artAxis.set3( o["axis"] );
}

void ImSetupManager::selectARTDevice(map<string,string> o) {
    hideAll();
    showART = true;
    showARTDevice = true;

    selected = o["name"];
    artID = o["ID"];
}

void ImSetupManager::selectView(OSG::VRGuiSignals::Options o) {
    hideAll();
    showViewport = true;

    selected = o["name"];
    viewPosition.set4( o["position"] );
    viewSize.set2( o["size"]);
    viewStereo = toBool(o["stereo"]);
    viewEyesInverted = toBool(o["eyesInverted"]);
    viewActiveStereo = toBool(o["activeStereo"]);
    viewProjection = toBool(o["projection"]);
    viewMirror = toBool(o["mirror"]);
    eyeSeparation.value = o["eyeSeparation"];
    string beacon = o["userBeacon"];
    current_view_user = 0;
    for (int i=0; i<view_users.size(); i++) {
        if (view_users[i] == beacon) { current_view_user = i; break; }
    }

    viewProjUser.set3( o["projUser"]);
    viewProjCenter.set3( o["projCenter"]);
    viewProjNormal.set3( o["projNormal"]);
    viewProjUp.set3( o["projUp"]);
    viewProjSize.set3( o["projSize"]);
    viewProjShear.set2( o["projShear"]);
    viewProjWarp.set2( o["projWarp"]);
    viewMirrorPos.set3( o["mirrorPos"]);
    viewMirrorNorm.set3( o["mirrorNorm"]);
}

void ImSetupManager::selectWindow(OSG::VRGuiSignals::Options o) {
    hideAll();
    showWindow = true;

    selected = o["name"];
    windowActive = toBool(o["active"]);
    windowMSAA.set(o["msaa"]);
    windowMouse.set(o["mouse"]);
    windowMultitouch.set(o["multitouch"]);
    windowKeyboard.set(o["keyboard"]);
    windowTitle = o["title"];
    windowIcon = o["icon"];
    int sW = toInt(o["sizeW"]);
    int sH = toInt(o["sizeH"]);
    windowSize.set2(sW, sH);
}

void ImSetupManager::selectMultiWindow(OSG::VRGuiSignals::Options o) {
    hideAll();
    showWindow = true;
    showRemoteWindow = true;

    selected = o["name"];
    remoteWinState = o["state"];
    winConnectionType.set(o["connType"]);
    Nx = toInt(o["nx"]);
    Ny = toInt(o["ny"]);
    NxNy.set2(Nx, Ny);
    toValue(o["serverIDs"], serverIDs);
}

void ImSetupManager::setWindowState(string window, string state) {
    if (selected == window) remoteWinState = state;
}

void ImSetupManager::selectNode(OSG::VRGuiSignals::Options o) {
    hideAll();
    showNode = true;

    nodeAddress = o["address"];
    nodeUser = o["user"];
    nodeSlave = o["slave"];
    nodeStatus = o["nodeStatus"];
    nodeSshStatus = o["sshStatus"];
    nodeSshKeyStatus = o["sshKeyStatus"];
    nodePathStatus = o["pathStatus"];
}

void ImSetupManager::selectSlave(OSG::VRGuiSignals::Options o) {
    hideAll();
    showSlave = true;

    slaveConnetionID = o["connectionID"];
    slaveMulticast = o["multicast"];
    slaveStatus = o["status"];
    slaveFullscreen = toBool(o["fullscreen"]);
    slaveActiveStereo = toBool(o["activeStereo"]);
    slaveAutostart = toBool(o["autostart"]);
    slaveDisplay = o["display"];
    slaveConnectionType.set(o["connectionType"]);
    slavePort = o["port"];
    slaveDelay = o["startupDelay"];
    slaveGeometry = o["geometry"];
}

void ImSetupManager::hideAll() {
    selected = "";
    showDisplay = false;
    showWindow = false;
    showEditorWindow = false;
    showLocalWindow = false;
    showRemoteWindow = false;
    showViewport = false;
    showVRPN = false;
    showVRPNTracker = false;
    showART = false;
    showARTDevice = false;
    showDevice = false;
    showMultitouch = false;
    showLeap = false;
    showHaptics = false;
    showNode = false;
    showSlave = false;
    showScript = false;
}

void ImSetupManager::treeAppend(string ID, string label, string type, string parent) {
    cout << " treeAppend " << ID << ", " << label << ", " << type << ", " << parent << endl;
    auto node = tree.add(ID, label, IM_TV_NODE_EDITABLE, parent);
    if (label == "Network") node->setMenu( {{"Add Node", "onSetupMenuAddNode"}} );
    if (label == "Displays") node->setMenu( {{"Add Window", "onSetupMenuAddWindow"}} );
    if (type == "node") node->setMenu( {{"Add Slave", "onSetupMenuAddSlave"}, {"Delete", "onSetupMenuDelete"}} );
    if (type == "slave") node->setMenu( {{"Delete", "onSetupMenuDelete"}} );
    if (type == "window") node->setMenu( {{"Add View", "onSetupMenuAddView"}, {"Delete", "onSetupMenuDelete"}} );
}

void ImSetupManager::updateSetupsList(string s) {
    toValue(s, setups);
}

void ImSetupManager::updateViewTrackersList(string s) {
    toValue(s, view_users);
}

void ImSetupManager::begin() {
    vector<const char*> tmpSetups(setups.size(), 0);
    for (int i=0; i<setups.size(); i++) tmpSetups[i] = setups[i].c_str();
    ImGui::Text("Setup:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    if (ImGui::Combo("##Setups", &current_setup, &tmpSetups[0], tmpSetups.size())) {
        uiSignal("setup_switch_setup", {{"setup",setups[current_setup]}});
    }

    ImGui::SameLine(); if (ImGui::Button("New")) uiSignal("setup_new");
    ImGui::SameLine(); if (ImGui::Button("Delete")) uiSignal("setup_delete");
    ImGui::SameLine(); if (ImGui::Button("Save")) uiSignal("setup_save");
    ImGui::SameLine(); if (ImGui::Button("Save as..")) uiSignal("setup_saveas");

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
    float fs = io.FontGlobalScale;
    float width = 120*fs; // TODO: add a width compute to TreeView

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetContentRegionAvail().y;
    float w1 = min(float(w*0.5), width);
    float w2 = w - w1;

    ImGui::BeginChild("setupTree", ImVec2(w1, h), true, flags);
    tree.render();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("setupProps", ImVec2(w2, h), false, flags);
        if (showDisplay) {
            ImGui::Text(("Displays: " + selected).c_str());
            ImGui::Indent(10);
            // Global Offset Vec3
            ImGui::Unindent(10);
        }

        if (showWindow) {
            ImGui::Text(("Window: " + selected).c_str());
            ImGui::Indent(10);
            if (ImGui::Checkbox("active##winActive", &windowActive)) uiSignal("setup_set_win_active", {{"active", toString(windowActive)}});
            if (windowSize.render(220) && windowSize.vX > 0 && windowSize.vY > 0) windowSize.signal("win_set_res");
            if (windowMSAA.render(100)) windowMSAA.signal("setup_switch_win_msaa");
            if (windowMouse.render(200)) windowMouse.signal("setup_switch_win_mouse");
            if (windowMultitouch.render(200)) windowMultitouch.signal("setup_switch_win_mtouch");
            if (windowKeyboard.render(200)) windowKeyboard.signal("setup_switch_win_keyb");
            ImInput wTitleEntry("##winTitle", "Title", windowTitle, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput wIconEntry("##winIcon", "Icon", windowIcon, ImGuiInputTextFlags_EnterReturnsTrue);
            if (wTitleEntry.render(240)) uiSignal("win_set_title", {{"title", wTitleEntry.value}});
            if (wIconEntry.render(240)) uiSignal("win_set_icon", {{"icon", wIconEntry.value}});
            ImGui::Unindent(10);
        }

        if (showEditorWindow) {
            ImGui::Separator();
            ImGui::Text(("Editor Window: " + selected).c_str());
            ImGui::Indent(10);
            // nothing yet
            ImGui::Unindent(10);
        }

        if (showLocalWindow) {
            ImGui::Separator();
            ImGui::Text(("Local Window: " + selected).c_str());
            ImGui::Indent(10);
            ImGui::Unindent(10);
        }

        if (showRemoteWindow) {
            ImGui::Separator();
            ImGui::Text(("Remote Window: " + selected).c_str());
            ImGui::Indent(10);
            ImGui::Text("State:");
            ImGui::SameLine();
            ImGui::Text(remoteWinState.c_str());
            ImGui::SameLine();
            if (ImGui::Button("connect##win")) uiSignal("win_click_connect", {{}});

            if (winConnectionType.render(200)) winConnectionType.signal("setup_switch_win_conn_type");

            if (NxNy.render(160) && NxNy.vX > 0 && NxNy.vY > 0) NxNy.signal("win_set_NxNy");
            ImGui::Indent(10);
            int k = 0;
            for (int x=0; x<Nx; x++) {
                for (int y=0; y<Ny; y++) {
                    string sID = "";
                    if (k < serverIDs.size()) sID = serverIDs[k];
                    string xy = toString(x) + " " + toString(y);
                    ImGui::Text(xy.c_str());
                    ImInput entry("##nxy_"+xy, "", sID, ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::SameLine();
                    if (entry.render(240)) uiSignal("win_set_serverID", {{"x", toString(x)}, {"y", toString(y)}, {"sID", entry.value}});
                    k++;
                }
            }
            ImGui::Unindent(10);
            ImGui::Unindent(10);
        }

        if (showViewport) {
            ImGui::Text(("Viewport: " + selected).c_str());
            ImGui::Indent(10);

            int w3 = w2-20;
            if (viewPosition.render(w2-10)) viewPosition.signal("setup_set_view_position");
            if (viewSize.render(w2-10)) viewSize.signal("setup_set_view_size");

            if (ImGui::Checkbox("Stereo", &viewStereo)) uiSignal("setup_set_view_stereo", {{"active", toString(viewStereo)}});

            if (viewStereo) {
                ImGui::Indent(10);
                if (eyeSeparation.render(50)) uiSignal("setup_set_view_eye_separation", {{"value", eyeSeparation.value}});
                if (ImGui::Checkbox("Invert eyes", &viewEyesInverted)) uiSignal("setup_set_view_invert_eyes", {{"active", toString(viewEyesInverted)}});
                if (ImGui::Checkbox("Active stereo", &viewActiveStereo)) uiSignal("setup_set_view_active_stereo", {{"active", toString(viewActiveStereo)}});
                ImGui::Unindent(10);
            }

            if (ImGui::Checkbox("Projection", &viewProjection)) uiSignal("setup_set_view_projection", {{"active", toString(viewProjection)}});
            if (viewProjection) {
                ImGui::Indent(10);
                vector<const char*> tmpViewUsers(view_users.size(), 0);
                for (int i=0; i<view_users.size(); i++) tmpViewUsers[i] = view_users[i].c_str();
                ImGui::Text("Tracker:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(150);
                if (ImGui::Combo("##ViewTracker", &current_view_user, &tmpViewUsers[0], tmpViewUsers.size())) {
                    uiSignal("setup_switch_view_user", {{"tracker",view_users[current_view_user]}});
                }

                if (viewProjCenter.render(w3)) viewProjCenter.signal("setup_set_view_proj_center");
                if (viewProjUser.render(w3)) viewProjUser.signal("setup_set_view_proj_user");
                if (viewProjNormal.render(w3)) viewProjNormal.signal("setup_set_view_proj_normal");
                if (viewProjUp.render(w3)) viewProjUp.signal("setup_set_view_proj_up");
                if (viewProjSize.render(w3)) viewProjSize.signal("setup_set_view_proj_size");
                if (viewProjShear.render(w3)) viewProjShear.signal("setup_set_view_proj_shear");
                if (viewProjWarp.render(w3)) viewProjWarp.signal("setup_set_view_proj_warp");
                ImGui::Unindent(10);
            }

            if (ImGui::Checkbox("Mirror", &viewMirror)) uiSignal("setup_set_view_mirror", {{"active", toString(viewMirror)}});
            if (viewMirror) {
                ImGui::Indent(10);
                if (viewMirrorPos.render(w3)) viewMirrorPos.signal("setup_set_view_mirror_position");
                if (viewMirrorNorm.render(w3)) viewMirrorNorm.signal("setup_set_view_mirror_normal");
                ImGui::Unindent(10);
            }
            ImGui::Unindent(10);
        }

        if (showVRPN) {
            ImGui::Text(("VRPN: " + selected).c_str());
            ImGui::Indent(10);
            ImGui::Checkbox("active##VRPN", &vrpnActive);
            ImGui::SameLine();
            //ImInput("##vrpnPort", "Port:")
            ImGui::Checkbox("test server##VRPN", &vrpnTestServer);
            ImGui::Checkbox("verbose##VRPN", &vrpnVerbose);
            ImGui::Unindent(10);
        }

        if (showVRPNTracker) {
            ImGui::Text(("VRPN Tracker: " + selected).c_str());
            ImGui::Indent(10);
            // address: entry ADDR
            // IF selected_type == "vrpn_device"
            //   translation: Vec3 TRANS_AXIS
            //   rotation: Vec3 TROT_AXIS
            ImGui::Unindent(10);
        }

        if (showART) {
            int w3 = w2-20;
            ImGui::Text("ART System");
            ImGui::Indent(10);
            if (ImGui::Checkbox("active##ART", &artActive)) uiSignal("setup_set_art_active", {{"active", toString(artActive)}});

            ImInput entry("##artPort", "port", toString(artPort), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if (entry.render(80)) uiSignal("setup_set_art_port", {{"port", toString(artPort)}});
            if (artOffset.render(w3)) artOffset.signal("setup_set_art_offset");
            if (artAxis.render(w3)) artAxis.signal("setup_set_art_axis");
            ImGui::Unindent(10);
        }

        if (showARTDevice) {
            ImGui::Text(("ART Device: " + selected).c_str());
            ImGui::Indent(10);
            ImGui::Text(artID.c_str());
            ImGui::Unindent(10);
        }

        if (showDevice) {
            ImGui::Text(("Device: " + selected).c_str());
            ImGui::Indent(10);
            // name: DEVICENAME
            // type: Combo devicetype
            // Intersection: INT_OBJ
            // int. point: INT_PNT
            // int. texel: INT_UV
            // checkbutton show intersection point
            ImGui::Unindent(10);
        }

        if (showMultitouch) {
            ImGui::Text(("Multitouch Device: " + selected).c_str());
            ImGui::Indent(10);
            // device: combo devicelist
            ImGui::Unindent(10);
        }

        if (showLeap) {
            ImGui::Text(("Leap Device: " + selected).c_str());
            ImGui::Indent(10);
            // address: entry ADDR
            // status: STATUS
            // serial: SERIAL
            // button start calibration    button stop calibration
            // transformation:
            //   position: Vec3 pos
            //   direction: Vec3 dir
            //   up-vector: Vec3 up
            ImGui::Unindent(10);
        }

        if (showHaptics) {
            ImGui::Text(("Haptic Device: " + selected).c_str());
            ImGui::Indent(10);
            // IP: entry IP
            // combo type
            // deamon state: STATE
            // device state: STATE
            ImGui::Unindent(10);
        }

        if (showNode) {
            ImInput nAddrEntry("##nodeAddr", "address:", nodeAddress, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput nUserEntry("##sshUsr", "ssh user:", nodeUser, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput nPathEntry("##pvrPath", "root path:", nodeSlave, ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::Text(("Network Node: " + selected).c_str());
            ImGui::Indent(10);
            if (nAddrEntry.render(240)) uiSignal("node_set_address", {{"address", nAddrEntry.value}});
            ImGui::SameLine();
            ImGui::Text(nodeStatus.c_str());

            if (nUserEntry.render(240)) uiSignal("node_set_user", {{"user", nUserEntry.value}});
            ImGui::SameLine();
            ImGui::Text(nodeSshStatus.c_str());

            if (ImGui::Button("distribute key##node")) uiSignal("node_clicked_distribkey", {{}});
            ImGui::SameLine();
            ImGui::Text(nodeSshKeyStatus.c_str());

            if (ImGui::Button("stop slaves##node")) uiSignal("node_clicked_stopslaves", {{}});

            if (nPathEntry.render(240)) uiSignal("node_set_path", {{"path", nPathEntry.value}});
            ImGui::SameLine();
            ImGui::Text(nodePathStatus.c_str());
            ImGui::Unindent(10);
        }

        if (showSlave) {
            ImInput sPortEntry("##slavePort", "port:", slavePort, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput sScreenEntry("##screenDisplay", "local display:", slaveDisplay, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput sDelayEntry("##startupDelay", "startup delay:", slaveDelay, ImGuiInputTextFlags_EnterReturnsTrue);
            ImInput sGeometryEntry("##winGeometry", "geometry ('512x512+0+0'):", slaveGeometry, ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::Text(("Network Slave, connection ID: " + slaveConnetionID).c_str());
            ImGui::Indent(10);
            // connection identifier:  CONN_ID_STR
            if (ImGui::Checkbox("autostart##slave", &slaveAutostart)) uiSignal("slave_toggle_autostart", {{"state",toString(slaveAutostart)}});
            ImGui::SameLine();
            if (ImGui::Button("start##slave")) uiSignal("slave_clicked_start", {{}});
            ImGui::SameLine();
            ImGui::Text(slaveStatus.c_str());

            if (ImGui::Checkbox("active stereo##slave", &slaveActiveStereo)) uiSignal("slave_toggle_activestereo", {{"state",toString(slaveActiveStereo)}});
            if (ImGui::Checkbox("fullscreen##slave", &slaveFullscreen)) uiSignal("slave_toggle_fullscreen", {{"state",toString(slaveFullscreen)}});
            if (sPortEntry.render(240)) uiSignal("slave_set_port", {{"port", sPortEntry.value}});
            if (slaveConnectionType.render(200)) slaveConnectionType.signal("setup_switch_slave_conn_type");
            if (sScreenEntry.render(150)) uiSignal("slave_set_screen", {{"screen", sScreenEntry.value}});
            ImGui::SameLine();
            if (slaveSystemScreens.render(100)) uiSignal("slave_set_screen", {{"screen", slaveSystemScreens.get()}});
            if (sDelayEntry.render(240)) uiSignal("slave_set_delay", {{"delay", sDelayEntry.value}});
            ImGui::SameLine();
            ImGui::Text("seconds");
            if (sGeometryEntry.render(240)) uiSignal("slave_set_geometry", {{"geometry", sGeometryEntry.value}});
            ImGui::Unindent(10);
        }

        if (showScript) {
            ImGui::Text(("Script: " + selected).c_str());
            ImGui::Indent(10);
            // nothing, TODO or deprecate?
            ImGui::Unindent(10);
        }

    ImGui::EndChild();
}
