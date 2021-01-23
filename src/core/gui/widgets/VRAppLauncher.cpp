#include <gtk/gtk.h>
#include "VRAppLauncher.h"
#include "../VRDemos.h"
#include "../VRGuiUtils.h"

#include "core/utils/system/VRSystem.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/gui/VRGuiSignals.h"
#include "core/gui/VRGuiContextMenu.h"

#include <iostream>

using namespace OSG;

VRAppLauncher::VRAppLauncher(VRAppPanelPtr s) : section(s) {}
VRAppLauncher::~VRAppLauncher() {}

VRAppLauncherPtr VRAppLauncher::create(VRAppPanelPtr s) { return VRAppLauncherPtr( new VRAppLauncher(s) ); }

void VRAppLauncher::show() { gtk_widget_show((GtkWidget*)widget); }
void VRAppLauncher::hide() { gtk_widget_hide((GtkWidget*)widget); }

void VRAppLauncher::updatePixmap() {
    if (imgScene == 0) return;
    if ( !exists( pxm_path ) ) return;
    try {
        loadGTKIcon(imgScene, pxm_path, 100, 60);
    } catch (...) { cout << "Warning: Caught exception in VRAppManager::updatePixmap, ignoring.."; }
}

void VRAppLauncher::setState(int state) {
    bool running = false;
    bool sensitive = true;
    const char* stock_id = "media-playback-start";

    if (state == 0) {} // default state, launcher is ready to start an app

    if (state == 1) sensitive = false; // launcher is disabled

    if (state == 2) { // launcher is ready to stop application
        running = true;
        stock_id = "media-playback-stop";
    }

    this->running = running;
    if (widget) gtk_widget_set_sensitive((GtkWidget*)widget, sensitive);
    if (imgPlay) gtk_image_set_from_icon_name(imgPlay, stock_id, GTK_ICON_SIZE_BUTTON);
}

void VRAppLauncher::toggle_lock() {
    write_protected = !write_protected;

    if (write_protected) {
        gtk_container_remove((GtkContainer*)butLock, (GtkWidget*)imgUnlock);
        gtk_container_add((GtkContainer*)butLock, (GtkWidget*)imgLock);
    } else {
        gtk_container_remove((GtkContainer*)butLock, (GtkWidget*)imgLock);
        gtk_container_add((GtkContainer*)butLock, (GtkWidget*)imgUnlock);
    }

    gtk_widget_show_all((GtkWidget*)butLock);

    auto scene = VRScene::getCurrent();
    if (scene) scene->setFlag("write_protected", write_protected);
}

void VRAppLauncher::setup(VRGuiContextMenu* menu, VRAppManager* mgr) {
    g_object_set(gtk_settings_get_default(), "gtk-button-images", FALSE, NULL);

    string rpath = VRSceneManager::get()->getOriginalWorkdir();

    // prep icons
    imgPlay = (GtkImage*)gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
    imgOpts = loadGTKIcon(0, rpath+"/ressources/gui/opts20.png", 20, 20);
    imgScene = loadGTKIcon(0, rpath+"/ressources/gui/default_scene.png", 100, 60);
    imgLock = loadGTKIcon(0, rpath+"/ressources/gui/lock20.png", 20, 20);
    imgUnlock = loadGTKIcon(0, rpath+"/ressources/gui/unlock20.png", 20, 20);
    g_object_ref(imgLock); // increase ref count
    g_object_ref(imgUnlock); // increase ref count

    // prep other widgets
    widget = (GtkFrame*)gtk_frame_new("");
    g_object_ref(widget); // increase ref count
    GtkEventBox* ebox = (GtkEventBox*)gtk_event_box_new();
    auto hb  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    auto vb  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    auto vb2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    label = (GtkLabel*)gtk_label_new(path.c_str());
    timestamp = (GtkLabel*)gtk_label_new(lastStarted.c_str());
    butPlay = (GtkButton*)gtk_button_new();
    butOpts = (GtkButton*)gtk_button_new();
    butLock = (GtkButton*)gtk_button_new();
    gtk_widget_set_halign((GtkWidget*)label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign((GtkWidget*)label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign((GtkWidget*)timestamp, GTK_ALIGN_CENTER);
    gtk_widget_set_valign((GtkWidget*)timestamp, GTK_ALIGN_CENTER);
    gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_START);
    gtk_label_set_max_width_chars(label, 20);
    gtk_widget_set_tooltip_text((GtkWidget*)butPlay, "Play/Stop");
    gtk_widget_set_tooltip_text((GtkWidget*)butOpts, "Options");
    gtk_widget_set_tooltip_text((GtkWidget*)butLock, "Write protection");
    gtk_widget_set_tooltip_text((GtkWidget*)label, path.c_str());

    // build widget
    gtk_box_pack_start((GtkBox*)vb2, (GtkWidget*)butOpts, false, false, 0);
    gtk_box_pack_end((GtkBox*)vb2, (GtkWidget*)butLock, false, false, 0);
    gtk_box_pack_start((GtkBox*)hb, (GtkWidget*)imgScene, false, false, 3);
    gtk_box_pack_end((GtkBox*)hb, (GtkWidget*)butPlay, false, false, 3);
    gtk_box_pack_end((GtkBox*)hb, (GtkWidget*)vb2, false, false, 3);
    gtk_box_pack_start((GtkBox*)vb, (GtkWidget*)label, false, false, 3);
    gtk_box_pack_start((GtkBox*)vb, (GtkWidget*)hb, false, false, 3);
    if (lastStarted != "") gtk_box_pack_start((GtkBox*)vb, (GtkWidget*)timestamp, false, false, 2);
    gtk_container_add((GtkContainer*)widget, (GtkWidget*)ebox);
    gtk_container_add((GtkContainer*)ebox, (GtkWidget*)vb);
    gtk_container_add((GtkContainer*)butPlay, (GtkWidget*)imgPlay);
    gtk_container_add((GtkContainer*)butOpts, (GtkWidget*)imgOpts);
    if (write_protected) gtk_container_add((GtkContainer*)butLock, (GtkWidget*)imgLock);
    else gtk_container_add((GtkContainer*)butLock, (GtkWidget*)imgUnlock);

    updatePixmap();

    // events
    uPixmap = VRDeviceCb::create("GUI_addDemoEntry", bind(&VRAppLauncher::updatePixmap, this) );
    VRGuiSignals::get()->getSignal("onSaveScene")->add( uPixmap );

    menu->connectWidget("DemoMenu", (GtkWidget*)ebox);
    //ebox->signal_event().connect( sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*mgr, &VRAppManager::on_any_event), e) );
    function<void(GdkEvent*)> f = bind(&VRAppManager::on_any_event, mgr, placeholders::_1, shared_from_this());
    connect_signal((GtkWidget*)ebox, f, "event" );

    function<void()> f1 = bind(&VRAppManager::toggleDemo, mgr, shared_from_this());
    function<void()> f2 = bind(&VRAppManager::on_menu_advanced, mgr, shared_from_this());
    function<void()> f3 = bind(&VRAppManager::on_lock_toggle, mgr, shared_from_this());
    connect_signal((GtkWidget*)butPlay, f1, "clicked" );
    connect_signal((GtkWidget*)butOpts, f2, "clicked" );
    connect_signal((GtkWidget*)butLock, f3, "clicked" );
    gtk_widget_show_all((GtkWidget*)widget);
}
