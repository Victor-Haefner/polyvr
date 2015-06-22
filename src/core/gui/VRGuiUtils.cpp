#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"

#include <iostream>

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/table.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/window.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/notebook.h>
#include <gtkmm/expander.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/colorselection.h>
#include <gtkmm/paned.h>

using namespace std;

Glib::RefPtr<Gtk::Builder> VRGuiBuilder(bool standalone) {
	static bool init = false;
	static Glib::RefPtr<Gtk::Builder> b;
	if (init) return b;
	init = true;

	string path = "ressources/gui/VRDirector.glade";
	if (standalone) path = "ressources/gui/VRDirector_min.glade";

	if (!VRGuiFile::exists(path)) cerr << "FATAL ERROR: " << path << " not found\n";
	else cout << " found glade file: " << path << endl;

	try { b = Gtk::Builder::create_from_file(path); }
	catch (Gtk::BuilderError& e) { cerr << "FATAL ERROR: " << e.what() << endl; }
    return b;
}

void setComboboxCallback(string b, void (* fkt)(GtkComboBox*, gpointer)) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(b, cb);
    g_signal_connect (cb->gobj(), "changed", G_CALLBACK(fkt), NULL);
}

void setLabel(string l, string txt) {
    Glib::RefPtr<Gtk::Label> label = Glib::RefPtr<Gtk::Label>::cast_static(VRGuiBuilder()->get_object(l));
    label->set_text(txt.c_str());
}

void setTextEntry(string entry, string text) {
    Gtk::Entry* ent;
    VRGuiBuilder()->get_widget(entry, ent);
    ent->set_text( text );
}

string getTextEntry(string entry) {
    Gtk::Entry* ent;
    VRGuiBuilder()->get_widget(entry, ent);
    return ent->get_text();
}

void setCheckButton(string cb, bool b) {
    Gtk::CheckButton* cbut;
    VRGuiBuilder()->get_widget(cb, cbut);
    cbut->set_active(b);
}

void setRadioButton(string cb, bool b) {
    Gtk::RadioButton* cbut;
    VRGuiBuilder()->get_widget(cb, cbut);
    cbut->set_active(b);
}

void setTableSensitivity(string table, bool b) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(table, tab);
    tab->set_sensitive(b);
}

void setComboboxSensitivity(string cb, bool b) {
    Gtk::ComboBox* tab;
    VRGuiBuilder()->get_widget(cb, tab);
    tab->set_sensitive(b);
}

void setNotebookSensitivity(string nb, bool b) {
    Gtk::Notebook* tab;
    VRGuiBuilder()->get_widget(nb, tab);
    tab->set_sensitive(b);
}

void setVPanedSensitivity(string vp, bool b) {
    Gtk::VPaned* tab;
    VRGuiBuilder()->get_widget(vp, tab);
    tab->set_sensitive(b);
}

void setToolButtonSensitivity(string toolbutton, bool b) {
    Gtk::ToolButton* tb;
    VRGuiBuilder()->get_widget(toolbutton, tb);
    tb->set_sensitive(b);
}

void setExpanderSensitivity(string exp, bool b) {
    Gtk::Expander* e;
    VRGuiBuilder()->get_widget(exp, e);
    if (!b) e->hide();
    else e->show();
}

void setCombobox(string n, int i) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(n, cb);
    cb->set_active(i);
}

void setCheckButtonCallback(string cb, void (* fkt)(GtkToggleButton*, gpointer) ) {
    Gtk::CheckButton* cbut;
    VRGuiBuilder()->get_widget(cb, cbut);
    g_signal_connect (cbut->gobj(), "toggled", G_CALLBACK(fkt), NULL);
}

void setCheckButtonCallback(string cb, sigc::slot<void> sig ) {
    Gtk::CheckButton* cbut;
    VRGuiBuilder()->get_widget(cb, cbut);
    cbut->signal_toggled().connect(sig);
}

void setRadioButtonCallback(string cb, sigc::slot<void> sig ) {
    Gtk::RadioButton* rbut;
    VRGuiBuilder()->get_widget(cb, rbut);
    rbut->signal_toggled().connect(sig);
}

bool getCheckButtonState(string b) {
    Gtk::CheckButton* tbut;
    VRGuiBuilder()->get_widget(b, tbut);
    return tbut->get_active();
}

bool getRadioButtonState(string b) {
    Gtk::RadioButton* tbut;
    VRGuiBuilder()->get_widget(b, tbut);
    return tbut->get_active();
}

bool getToggleButtonState(string b) {
    Gtk::ToggleToolButton* tbut;
    VRGuiBuilder()->get_widget(b, tbut);
    return tbut->get_active();
}

void setComboboxCallback(string b, sigc::slot<void> sig) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(b, cb);
    cb->signal_changed().connect(sig);
}

void setTreeviewSelectCallback(string treeview, sigc::slot<void> sig) {
    Gtk::TreeView* tv;
    VRGuiBuilder()->get_widget(treeview, tv);
    tv->signal_cursor_changed().connect(sig);
}

Gtk::TreeModel::iterator getTreeviewSelected(string treeview) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object(treeview));
    return tree_view->get_selection()->get_selected();
}

void selectTreestoreRow(string treeview, Gtk::TreeModel::iterator itr) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object(treeview));
    Gtk::TreeModel::Path path(itr);
    tree_view->set_cursor( path );
    tree_view->grab_focus();
}

void focusTreeView(string treeview) {
    Glib::RefPtr<Gtk::TreeView> tree_view  = Glib::RefPtr<Gtk::TreeView>::cast_static(VRGuiBuilder()->get_object(treeview));
    tree_view->grab_focus();
}

void setEntryCallback(string e, void (* fkt)(GtkEntry*, gpointer)) {
    Gtk::Entry* en;
    VRGuiBuilder()->get_widget(e, en);
    g_signal_connect (en->gobj(), "activate", G_CALLBACK(fkt), NULL);
    g_signal_connect (en->gobj(), "focus-out-event", G_CALLBACK(fkt), NULL);
}

void setEntryCallback(string e, sigc::slot<void> sig) {
    Gtk::Entry* en;
    VRGuiBuilder()->get_widget(e, en);
    en->signal_activate().connect(sig);
}

void setEntryCallback(string e, sigc::slot<bool,GdkEventFocus*> sig) {
    Gtk::Entry* en;
    VRGuiBuilder()->get_widget(e, en);
    en->signal_focus_out_event().connect(sig);
}

void setEntrySensitivity(string e, bool b) {
    Gtk::Entry* en;
    VRGuiBuilder()->get_widget(e, en);
    en->set_sensitive(b);
}

void setButtonCallback(string b, void (* fkt)(GtkButton*, gpointer), gpointer data) {
    Gtk::Button* bu;
    VRGuiBuilder()->get_widget(b, bu);
    g_signal_connect (bu->gobj(), "clicked", G_CALLBACK(fkt), data);
}

void setButtonCallback(string b, sigc::slot<void> sig ) {
    Gtk::Button* bu;
    VRGuiBuilder()->get_widget(b, bu);
    bu->signal_clicked().connect(sig);
}

void setButtonText(string b, string txt ) {
    Gtk::Button* bu;
    VRGuiBuilder()->get_widget(b, bu);
    bu->set_label(txt);
}

bool keySignalProxy(GdkEventKey* e, string k, sigc::slot<void> sig ) {
    if (gdk_keyval_name(e->keyval) == k) {
        sig();
        return true;
    }
    return false;
}

void setToolButtonCallback(string b, void (* fkt)(GtkButton*, gpointer)) {
    Gtk::ToolButton* bu;
    VRGuiBuilder()->get_widget(b, bu);
    g_signal_connect (bu->gobj(), "clicked", G_CALLBACK(fkt), NULL);
}

void setToolButtonCallback(string b, sigc::slot<void> sig ) {
    Gtk::ToolButton* bu;
    VRGuiBuilder()->get_widget(b, bu);
    bu->signal_clicked().connect(sig);
}

void setToggleButtonCallback(string b, sigc::slot<void> sig ) {
    Gtk::ToggleButton* bu;
    VRGuiBuilder()->get_widget(b, bu);
    bu->signal_clicked().connect(sig);
}

void setComboboxLastActive(string n) { // TODO: google how to get N rows!
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(n, cb);
    Glib::RefPtr<Gtk::TreeModel> model = cb->get_model();
    cb->set_active(model->children().size() - 1);
}

void eraseComboboxActive(string n) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(n, cb);
    //Glib::RefPtr<Gtk::TreeModel> model = cb->get_model()
    GtkTreeModel* model = cb->get_model()->gobj();
    //model->erase(cb->get_active());

    gtk_list_store_remove(GTK_LIST_STORE(model), cb->get_active().gobj());
    cb->set_active(cb->get_model()->children().size() - 1);
}

string getComboboxText(string cbn) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(cbn, cb);
    char* name = gtk_combo_box_get_active_text(cb->gobj());
    if (name == 0) return "";
    return string (name);
}

int getComboboxI(string cbn) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(cbn, cb);
    int i = gtk_combo_box_get_active(cb->gobj());
    return i;
}

Gtk::TreeModel::Row getComboboxRow(string cbn) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(cbn.c_str(), cb);
    return *cb->get_active();
}

Gtk::TreeModel::iterator getComboboxIter(string cbn) {
    Gtk::ComboBox* cb;
    VRGuiBuilder()->get_widget(cbn.c_str(), cb);
    return cb->get_active();
}

bool askUser(string msg1, string msg2) {
    Gtk::MessageDialog dialog(msg1, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);
    dialog.set_secondary_text(msg2);
    dialog.set_deletable(false);

    if (dialog.run() == Gtk::RESPONSE_OK) return true;
    return false;
}

OSG::Color4f chooseColor(string drawable, OSG::Color4f current) {
    Glib::RefPtr<Gtk::DrawingArea> darea = Glib::RefPtr<Gtk::DrawingArea>::cast_static(VRGuiBuilder()->get_object(drawable.c_str()));
    Gtk::ColorSelectionDialog cdiag;
    cdiag.set_deletable(false);
    cdiag.get_color_selection()->set_has_opacity_control(true);

    Gdk::Color c("#FFFFFF");
    c.set_rgb_p(current[0], current[1], current[2]);
    cdiag.get_color_selection()->set_current_color(c);
    cdiag.get_color_selection()->set_current_alpha(current[3]*65535);

    float alpha = 0;
    if (cdiag.run() == Gtk::RESPONSE_OK) {
        c = cdiag.get_color_selection()->get_current_color();
        alpha = cdiag.get_color_selection()->get_current_alpha()/65535.0;
    }

    darea->modify_bg(Gtk::STATE_NORMAL, c); // TODO: blend with pattern to show alpha channel

    OSG::Color4f col;
    col[0] = c.get_red_p();
    col[1] = c.get_green_p();
    col[2] = c.get_blue_p();
    col[3] = alpha;
    return col;
}

void setColorChooserColor(string drawable, OSG::Color3f col) {
    Gdk::Color c;
    c.set_rgb_p(col[0], col[1], col[2]);

    Glib::RefPtr<Gtk::DrawingArea> darea;
    darea = Glib::RefPtr<Gtk::DrawingArea>::cast_static(VRGuiBuilder()->get_object(drawable.c_str()));
    darea->modify_bg(Gtk::STATE_NORMAL, c);
}


void setColorChooser(string drawable, sigc::slot<bool, GdkEventButton*> sig) {
    Glib::RefPtr<Gtk::DrawingArea> darea = Glib::RefPtr<Gtk::DrawingArea>::cast_static(VRGuiBuilder()->get_object(drawable.c_str()));
    darea->add_events((Gdk::EventMask)GDK_BUTTON_PRESS_MASK);
    darea->add_events((Gdk::EventMask)GDK_BUTTON_RELEASE_MASK);
    darea->signal_button_release_event().connect( sig );
}

void setCellRendererCallback(string renderer, void (* fkt)(GtkCellRendererText*, gchar*, gchar*, gpointer)) {
    Glib::RefPtr<Gtk::CellRendererText> crt;
    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object(renderer.c_str()));
    g_signal_connect (crt->gobj(), "edited", G_CALLBACK (fkt), NULL);
}

void setCellRendererCallback(string renderer, sigc::slot<void,const Glib::ustring&,const Glib::ustring& > sig, bool after) {
    Glib::RefPtr<Gtk::CellRendererText> crt;
    crt = Glib::RefPtr<Gtk::CellRendererText>::cast_static(VRGuiBuilder()->get_object(renderer.c_str()));
    crt->signal_edited().connect( sig, after);
}

void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, void (* fkt)(GtkCellRendererCombo*, gchar*, GtkTreeIter*, gpointer)) {
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object(combolist.c_str()));
    Gtk::CellRendererCombo* renderer = new Gtk::CellRendererCombo();
    renderer->property_has_entry() = false;
    renderer->property_model() = combo_list;
    renderer->property_text_column() = 0;
    renderer->property_editable() = true;

    Glib::RefPtr<Gtk::TreeView::Column> column = Glib::RefPtr<Gtk::TreeView::Column>::cast_static(VRGuiBuilder()->get_object(treeviewcolumn.c_str()));
    column->pack_start(*renderer);

    column->add_attribute(renderer->property_text(), col);
    g_signal_connect (renderer->gobj(), "changed", G_CALLBACK (fkt), NULL);
}

void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, sigc::slot<void, const Glib::ustring&, const Gtk::TreeModel::iterator& > sig) {
    Glib::RefPtr<Gtk::ListStore> combo_list = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object(combolist.c_str()));
    Gtk::CellRendererCombo* renderer = new Gtk::CellRendererCombo();
    renderer->property_has_entry() = false;
    renderer->property_model() = combo_list;
    renderer->property_text_column() = 0;
    renderer->property_editable() = true;

    Glib::RefPtr<Gtk::TreeView::Column> column = Glib::RefPtr<Gtk::TreeView::Column>::cast_static(VRGuiBuilder()->get_object(treeviewcolumn.c_str()));
    column->pack_start(*renderer);

    column->add_attribute(renderer->property_text(), col);
    renderer->signal_changed().connect(sig);
}

void setNoteBookCallback(string nb,  void (* fkt)(GtkNotebook*, GtkNotebookPage*, guint, gpointer), gpointer ptr) {
    Glib::RefPtr<Gtk::Notebook> nbk;
    nbk = Glib::RefPtr<Gtk::Notebook>::cast_static(VRGuiBuilder()->get_object(nb.c_str()));
    g_signal_connect_after(nbk->gobj(), "switch-page", G_CALLBACK (fkt), ptr);
}

void setNotebookPage(string nb, int p) {
    Glib::RefPtr<Gtk::Notebook> nbk;
    nbk = Glib::RefPtr<Gtk::Notebook>::cast_static(VRGuiBuilder()->get_object(nb.c_str()));
    nbk->set_current_page(p);
}

class LStore_ModelColumns : public Gtk::TreeModelColumnRecord {
    public:
        LStore_ModelColumns() { add(content); }

        Gtk::TreeModelColumn<Glib::ustring> content;
};

OSG::ImageRecPtr takeSnapshot() {
    Gtk::DrawingArea* drawArea = 0;
    VRGuiBuilder()->get_widget("glarea", drawArea);
    Glib::RefPtr<Gdk::Drawable> src = drawArea->get_window(); // 24 bits per pixel ( src->get_depth() )
    int w = drawArea->get_width();
    int h = drawArea->get_height();
    w -= w%4; h -= h%4;
    //cout << "PIC FORMAT " << w << " " << h << " " << src->get_depth() << endl;
    Glib::RefPtr<Gdk::Image> img = Glib::wrap( gdk_drawable_get_image(src->gobj(), 0, 0, w, h) );
    Glib::RefPtr<Gdk::Pixbuf> pxb = Glib::wrap( gdk_pixbuf_get_from_image(NULL, img->gobj(), src->get_colormap()->gobj(), 0,0,0,0,w,h) );

    OSG::ImageRecPtr res = OSG::Image::create();
    //Image::set(pixFormat, width, height, depth, mipmapcount, framecount, framedelay, data, type, aloc, sidecount);
    res->set(OSG::Image::OSG_RGB_PF, w, h, 1, 0, 1, 0, (const unsigned char*)pxb->get_pixels(), OSG::Image::OSG_UINT8_IMAGEDATA, true, 1);
    return res;
}

void saveSnapshot(string path) {
    Gtk::DrawingArea* drawArea = 0;
    VRGuiBuilder()->get_widget("glarea", drawArea);
    Glib::RefPtr<Gdk::Drawable> src = drawArea->get_window();
    int smin = min(drawArea->get_width(), drawArea->get_height());
    int u = max(0.0, drawArea->get_width()*0.5 - smin*0.5);
    int v = max(0.0, drawArea->get_height()*0.5 - smin*0.5);
    Glib::RefPtr<Gdk::Pixbuf> pxb = Gdk::Pixbuf::create( src, u, v, smin, smin);
    pxb = pxb->scale_simple(128, 128, Gdk::INTERP_HYPER);
    pxb->save(path, "png");
}

void saveScene(string path) {
    OSG::VRScene* scene = OSG::VRSceneManager::getCurrent();
    if (scene == 0) return;
    //if (path == "") path = scene->getPath();
    path = scene->getFile();

    if (scene->getFlag("write_protected")) return;

    OSG::VRSceneLoader::get()->saveScene(path);
    //string ipath = scene->getWorkdir() + '/'+  scene->getIcon();
    string ipath = scene->getIcon();
    saveSnapshot(ipath);

    OSG::VRGuiSignals::get()->getSignal("onSaveScene")->trigger<OSG::VRDevice>();
}

int getListStorePos(string ls, string s) {
    Gtk::TreeModel::iterator iter;
    Gtk::TreeModel::Row row;
    LStore_ModelColumns mcols;
    Glib::RefPtr<Gtk::ListStore> store  = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object(ls.c_str()));
    int N = gtk_tree_model_iter_n_children( (GtkTreeModel*) store->gobj(), NULL );
    for (int i=0; i<N; i++) {
        stringstream ss; ss << i;
        iter = store->get_iter(ss.str());
        if (!iter) continue;

        row = *iter;
        string c = row.get_value(mcols.content);
        if (c == s) return i;
    }
    return -1;
}

void fillStringListstore(string ls, vector<string> list) {
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object(ls.c_str()));
    store->clear();
    for (unsigned int i=0; i<list.size(); i++) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, list[i].c_str(), -1);
    }
}

void showDialog(string d) {
    Gtk::Window* dialog;
    VRGuiBuilder()->get_widget(d, dialog);
    dialog->show();
}

void hideDialog(string d) {
    Gtk::Window* dialog;
    VRGuiBuilder()->get_widget(d, dialog);
    dialog->hide();
}

