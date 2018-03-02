
#include "VRGuiNet.h"
#include "VRGuiUtils.h"
#include "core/networking/VRSocket.h"
#include "core/utils/toString.h"
#include <gtkmm/liststore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/combobox.h>
#include <gtkmm/builder.h>
#include <gtkmm/cellrenderercombo.h>
#include "core/scene/VRScene.h"

//* DEPRECATED!! *//

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiNet_SocketCols : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiNet_SocketCols() { add(type); add(port); add(ip); add(callback); add(signal); add(name); add(obj); add(sens); }

        Gtk::TreeModelColumn<Glib::ustring> type;
        Gtk::TreeModelColumn<uint>          port;
        Gtk::TreeModelColumn<Glib::ustring> ip;
        Gtk::TreeModelColumn<Glib::ustring> callback;
        Gtk::TreeModelColumn<Glib::ustring> signal;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<gpointer>      obj;
        Gtk::TreeModelColumn<bool>      sens;
};

class VRGuiNet_TypeCols : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiNet_TypeCols() { add(type); }

        Gtk::TreeModelColumn<Glib::ustring> type;
};

// --------------------------
// ---------Callbacks--------
// --------------------------

void VRGuiNet_updateList() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    // update script list
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("Sockets"));
    store->clear();
    for (auto s : scene->getSockets()) {
        VRSocketPtr socket = s.second;
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set(store->gobj(), row.gobj(), 0, socket->getType().c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 1, socket->getPort(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 2, socket->getIP().c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 3, socket->getCallback().c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 4, socket->getSignal()->getName().c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 5, socket->getName().c_str(), -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 6, socket, -1);
        gtk_list_store_set(store->gobj(), row.gobj(), 7, socket->isClient(), -1);
    }
}

void VRGuiNet::on_new_clicked() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;
    VRSocketPtr socket = scene->getSocket(5000);
    if (socket == 0) return;

    socket->setType("tcpip send");
    socket->setIP("192.168.0.100");

    VRGuiNet_updateList();
}

void VRGuiNet::on_del_clicked() {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    VRGuiNet_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);

    string msg1 = "Delete socket " + name;
    if (!askUser(msg1, "Are you sure you want to delete this socket?")) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->remSocket(name);

    Glib::RefPtr<Gtk::ListStore> list_store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("Sockets"));
    list_store->erase(iter);

    Gtk::ToolButton* b;
    VRGuiBuilder()->get_widget("toolbutton15", b);
    b->set_sensitive(false);
}

void VRGuiNet_on_treeview_select(GtkTreeView* tv, gpointer user_data) {
    Gtk::ToolButton* b;
    VRGuiBuilder()->get_widget("toolbutton15", b);
    b->set_sensitive(true);
}


void VRGuiNet_on_name_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_name, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // get selected socket
    VRGuiNet_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    string name = row.get_value(cols.name);
    row[cols.name] = new_name;

    // update key in map
    auto scene = VRScene::getCurrent();
    if (scene) scene->changeSocketName(name, new_name);
}

void VRGuiNet_on_argtype_edited(GtkCellRendererCombo* crc, gchar *path_string, GtkTreeIter *new_iter, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new type
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("socket_type_list"));
    gchar *t;
    gtk_tree_model_get((GtkTreeModel*)combo_list->gobj(), new_iter, 0, &t, -1);
    string type = string(t);
    Gtk::TreeModel::Row row = *iter;
    VRGuiNet_SocketCols cols;
    row[cols.type] = type;

    VRSocket* socket = (VRSocket*)row.get_value(cols.obj);
    socket->setType(type);
    row[cols.sens] = socket->isClient();
}

void VRGuiNet_on_argport_edited(GtkCellRendererText *cell, gchar *path_string, gchar *port, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiNet_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.port] = toInt(port);

    VRSocket* socket = (VRSocket*)row.get_value(cols.obj);
    socket->setPort(toInt(port));
}

void VRGuiNet_on_argip_edited(GtkCellRendererText *cell, gchar *path_string, gchar *ip, gpointer d) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    Gtk::TreeModel::iterator iter = tree_view->get_selection()->get_selected();
    if(!iter) return;

    // set the cell with new name
    VRGuiNet_SocketCols cols;
    Gtk::TreeModel::Row row = *iter;
    row[cols.ip] = ip;

    VRSocket* socket = (VRSocket*)row.get_value(cols.obj);
    socket->setIP(ip);
}


void VRGuiNet_on_notebook_switched(GtkNotebook* notebook, GtkNotebookPage* page, guint pageN, gpointer data) {
    if (pageN == 4) VRGuiNet_updateList();
}


// --------------------------
// ---------Main-------------
// --------------------------

VRGuiNet::VRGuiNet() {
    setToolButtonCallback("toolbutton14", sigc::mem_fun(*this, &VRGuiNet::on_new_clicked));
    setToolButtonCallback("toolbutton15", sigc::mem_fun(*this, &VRGuiNet::on_del_clicked));

    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object("treeview9"));
    g_signal_connect (tree_view->gobj(), "cursor-changed", G_CALLBACK (VRGuiNet_on_treeview_select), NULL);

    setCellRendererCallback("cellrenderertext22", VRGuiNet_on_name_edited);
    setCellRendererCallback("cellrenderertext18", VRGuiNet_on_argport_edited);
    setCellRendererCallback("cellrenderertext20", VRGuiNet_on_argip_edited);

    VRGuiNet_SocketCols cols;
    setCellRendererCombo("treeviewcolumn18", "socket_type_list", cols.type, VRGuiNet_on_argtype_edited);

    // fill combolist
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("socket_type_list"));
    VRGuiNet_TypeCols cols2;
    Gtk::ListStore::Row row;
    row = *combo_list->append(); row[cols2.type] = "tcpip send";
    row = *combo_list->append(); row[cols2.type] = "tcpip receive";
    row = *combo_list->append(); row[cols2.type] = "http post";
    row = *combo_list->append(); row[cols2.type] = "http get";

    setNoteBookCallback("notebook3", VRGuiNet_on_notebook_switched);

    setToolButtonSensitivity("toolbutton15", false);
}

void VRGuiNet::updateList() {
    VRGuiNet_updateList();
}

OSG_END_NAMESPACE;
