#include "VRImguiSetup.h"

#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/imgui/imWidgets/VRImguiInput.h"

ImSetupManager::ImSetupManager() : ImWidget("SetupManager"), tree("setup") {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("updateSetupsList", [&](OSG::VRGuiSignals::Options o){ updateSetupsList(o["setups"]); return true; } );
    mgr->addCallback("setCurrentSetup", [&](OSG::VRGuiSignals::Options o){ current_setup = toInt(o["setup"]); return true; } );
}

void ImSetupManager::updateSetupsList(string s) {
    toValue(s, setups);
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


    auto region1 = ImGui::GetContentRegionAvail();
    auto region2 = ImGui::GetContentRegionAvail();
    region1.x *= 0.5;
    region2.x *= 0.5;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;


    ImGui::BeginChild("setupTree", region1, false, flags);
    tree.render();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("setupProps", region2, false, flags);
        if (0) {
            ImGui::Text(("Displays: " + selected).c_str());
            ImGui::Indent(10);
            // Global Offset Vec3
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Window: " + selected).c_str());
            ImGui::Indent(10);
            // bool active
            // X Display dropdown, probably deprecated
            // fullscreen checkbutton
            // Vec3 Position
            // Vec2 Resolution
            // Combo Mouse device (None / mouse)
            // Combo MSAA ( x0 x2 x4 x8 x16 )
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Editor Window: " + selected).c_str());
            ImGui::Indent(10);
            // nothing yet
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Local Window: " + selected).c_str());
            ImGui::Indent(10);
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Remote Window: " + selected).c_str());
            ImGui::Indent(10);
            // State: CONN_STATE   button connect
            // Connection type: radiobuttons Multicast SockPipeline StreamSock
            // Nx: entry 1   Ny: entry 1
            // Listview Server List
            //  i j CONN_ID_STR
            //  ...
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Viewport: " + selected).c_str());
            ImGui::Indent(10);
            // Area
            //  Vec2 x
            //  Vec2 y
            // Vec2 Size (resolution)
            // checkbutton statistics
            // checkbutton stereo
            // Eye Separation: entry 0.06 [m]
            // checkbutton invert
            // checkbutton active stereo
            // checkbutton projection
            // checkbutton user  combo trackers
            // Vec3 center
            // Vec3 user
            // Vec3 normal
            // Vec3 up
            // Vec2 size
            // Vec2 shear
            // Vec2 warp
            // checkbutton mirror:
            // Vec3 mirror origin
            // Vec3 mirror normal
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("VRPN: " + selected).c_str());
            ImGui::Indent(10);
            // checkbutton active    Port: entry PORT
            // checkbutton test server
            // checkbutton verbose
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("VRPN Tracker: " + selected).c_str());
            ImGui::Indent(10);
            // address: entry ADDR
            // IF selected_type == "vrpn_device"
            //   translation: Vec3 TRANS_AXIS
            //   rotation: Vec3 TROT_AXIS
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("ART: " + selected).c_str());
            ImGui::Indent(10);
            // checkbutton active    Port: entry PORT
            // Vec3 Offset
            // Vec3 Axis
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("ART Device: " + selected).c_str());
            ImGui::Indent(10);
            // ID: ID
            ImGui::Unindent(10);
        }

        if (0) {
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

        if (0) {
            ImGui::Text(("Multitouch Device: " + selected).c_str());
            ImGui::Indent(10);
            // device: combo devicelist
            ImGui::Unindent(10);
        }

        if (0) {
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

        if (0) {
            ImGui::Text(("Haptic Device: " + selected).c_str());
            ImGui::Indent(10);
            // IP: entry IP
            // combo type
            // deamon state: STATE
            // device state: STATE
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Network Node: " + selected).c_str());
            ImGui::Indent(10);
            // address: entry ADDR   STATE
            // ssh user: entry USER   STATE
            // button distribute key   STATE
            // button stop slaves
            // root path: entry PATH   STATE
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Network Slave: " + selected).c_str());
            ImGui::Indent(10);
            // connection identifier:  CONN_ID_STR
            // checkbutton autostart     button start    STATE
            // checkbutton active stereo     checkbutton fullscreen      port: entry PORT
            // Connection type: radiobuttons Multicast SockPipeline StreamSock
            // local display: entry DISPLAY    startup delay: entry DELAY
            // geometry ('512x512+0+0'): entry 512x512+0+0
            ImGui::Unindent(10);
        }

        if (0) {
            ImGui::Text(("Script: " + selected).c_str());
            ImGui::Indent(10);
            // nothing, TODO or deprecate?
            ImGui::Unindent(10);
        }

    ImGui::EndChild();
}
