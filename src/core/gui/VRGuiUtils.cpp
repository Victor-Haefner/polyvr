#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRScene.h"
#include "core/setup/devices/VRDevice.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/xml.h"

#include <OpenSG/OSGImage.h>

#include <iostream>
#include <functional>

#include <gtk/gtkwidget.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhscale.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkradiotoolbutton.h>
#include <gtk/gtktoolbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderercombo.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbuilder.h>

using namespace std;
namespace PL = std::placeholders;

VRGuiBuilder::VRGuiBuilder() {}
VRGuiBuilder::~VRGuiBuilder() {}

void VRGuiBuilder::read(string path) {
    //XML xml;
    //xml.read(path);

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, path.c_str(), NULL);
}

GtkWidget* VRGuiBuilder::get_widget(string name) {
    return (GtkWidget*)gtk_builder_get_object(builder, name.c_str());
    //if (widgets.count(name)) return widgets[name];
    return 0;
}

GtkObject* VRGuiBuilder::get_object(string name) {
    return (GtkObject*)gtk_builder_get_object(builder, name.c_str());
    //if (objects.count(name)) return objects[name];
    return 0;
}

VRGuiBuilder* getGUIBuilder(bool standalone) {
	static VRGuiBuilder* b = 0;
	if (b) return b;
	b = new VRGuiBuilder();

	string path = "ressources/gui/VRDirector.glade";
	if (standalone) path = "ressources/gui/VRDirector_min.glade";
	if (!VRGuiFile::exists(path)) cerr << "FATAL ERROR: " << path << " not found\n";
	else {
        cout << " found glade file: " << path << endl;
        b->read(path);
	}
    return b;
}

void setLabel(string l, string txt) {
    GtkLabel* label = (GtkLabel*)getGUIBuilder()->get_object(l);
    gtk_label_set_text(label, txt.c_str());
}

void setTextEntry(string entry, string text) {
    GtkEntry* ent = (GtkEntry*)getGUIBuilder()->get_widget(entry);
    gtk_entry_set_text(ent, text.c_str());
}

string getTextEntry(string entry) {
    GtkEntry* ent = (GtkEntry*)getGUIBuilder()->get_widget(entry);
    auto t = gtk_entry_get_text(ent);
    return t?t:"";
}

void setToggleButton(string cb, bool b) {
    GtkToggleButton* tb = (GtkToggleButton*)getGUIBuilder()->get_widget(cb);
    gtk_toggle_button_set_active(tb, b);
}

void setWidgetSensitivity(string table, bool b) {
    GtkWidget* w = getGUIBuilder()->get_widget(table);
    gtk_widget_set_sensitive(w, b);
}

void setWidgetVisibility(string table, bool b) {
    GtkWidget* w = getGUIBuilder()->get_widget(table);
    if (b) gtk_widget_show(w);
    else   gtk_widget_hide(w);
}

void setCombobox(string n, int i) {
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(n);
    gtk_combo_box_set_active(cb, i);
}

template<typename T>
void setupCallback(string w, T fkt, string event, bool after = false) {
    GtkWidget* W = getGUIBuilder()->get_widget(w);
    connect_signal(W, fkt, event, after);
}

void setButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "clicked"); }
void setToggleButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "clicked"); }
void setToolButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "clicked"); }
void setCheckButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "toggled"); }
void setRadioToolButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "toggled"); }
void setRadioButtonCallback(string b, function<void()> sig ) { setupCallback(b, sig, "toggled"); }
void setComboboxCallback(string b, function<void()> sig) { setupCallback(b, sig, "changed"); }

void setSliderCallback(string b, function<bool(int,double)> sig) { setupCallback(b, sig, "change_value"); }
void setTreeviewSelectCallback(string b, function<void(void)> sig) { setupCallback(b, sig, "cursor_changed"); }
void setCellRendererCallback(string b, function<void(gchar*, gchar*)> sig, bool after) { setupCallback(b, sig, "edited"); }
void setNoteBookCallback(string b, function<void(GtkNotebookPage*, guint)> sig) { setupCallback(b, sig, "switch-page", true); }

void setColorChooser(string drawable, function<void(GdkEventButton*)> sig) {
    GtkDrawingArea* darea = (GtkDrawingArea*)getGUIBuilder()->get_object(drawable);
    gtk_widget_add_events((GtkWidget*)darea, (GdkEventMask)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events((GtkWidget*)darea, (GdkEventMask)GDK_BUTTON_RELEASE_MASK);
    connect_signal((GtkWidget*)darea, sig, "button_release_event");
}

bool entryFocusProxy(GdkEventFocus* e, function<void()> sig) {
    sig();
    return true;
}

void setEntryCallback(string b, function<void()> sig, bool onEveryChange, bool onFocusOut, bool onActivate) {
    if (onEveryChange) setupCallback(b, sig, "changed");
    else {
        if (onActivate) setupCallback(b, sig, "activate");
        if (onFocusOut) {
            function<bool(GdkEventFocus*)> sig2 = bind(entryFocusProxy, placeholders::_1, sig);
            setupCallback(b, sig2, "focus_out_event");
        }
    }
}

bool getCheckButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)getGUIBuilder()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getRadioButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)getGUIBuilder()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getRadioToolButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)getGUIBuilder()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getToggleButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)getGUIBuilder()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getToggleToolButtonState(string b) {
    GtkToggleToolButton* tbut = (GtkToggleToolButton*)getGUIBuilder()->get_widget(b);
    return gtk_toggle_tool_button_get_active(tbut);
}

string getTreeviewCell(string treeview, GtkTreeIter iter, int column) {
    GtkTreeView* tree_view = (GtkTreeView*)getGUIBuilder()->get_object(treeview);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    gchar* n;
    gtk_tree_model_get(model, &iter, column, &n, -1);
    string name=n?n:"";
    g_free(n);
    return name;
}

string getTreeviewSelected(string treeview, int column) {
    GtkTreeView* tree_view = (GtkTreeView*)getGUIBuilder()->get_object(treeview);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        return getTreeviewCell(treeview, iter, column);
    }
    return "";
}

bool hasTreeviewSelection(string treeview) {
    GtkTreeView* tree_view = (GtkTreeView*)getGUIBuilder()->get_object(treeview);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    return gtk_tree_selection_get_selected(sel, &model, &iter);
}

void setTreeViewSelectedText(string tv, int column, string data) {
    GtkTreeView* tree_view = (GtkTreeView*)getGUIBuilder()->get_object(tv);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gtk_tree_model_get(model, &iter, column, data.c_str(), -1);
    }
}

void setButtonText(string b, string txt ) {
    GtkButton* bu = (GtkButton*)getGUIBuilder()->get_widget(b);
    gtk_button_set_label(bu, txt.c_str());
}

float getSliderState(string s) {
    GtkHScale* hs = (GtkHScale*)getGUIBuilder()->get_widget(s);
    return gtk_adjustment_get_value((GtkAdjustment*)hs);
}

bool keySignalProxy(GdkEventKey* e, string k, function<void(void)> sig ) {
    if (gdk_keyval_name(e->keyval) == k) { sig(); return true; }
    return false;
}

void setComboboxLastActive(string n) { // TODO: google how to get N rows!
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(n);
    GtkTreeModel* model = gtk_combo_box_get_model(cb);
    int i = gtk_tree_model_iter_n_children(model, 0);
    gtk_combo_box_set_active(cb, i-1);
}

void eraseComboboxActive(string n) {
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(n);
    GtkTreeModel* model = gtk_combo_box_get_model(cb);
    GtkTreeIter i;
    bool b = gtk_combo_box_get_active_iter(cb, &i);
    if (b) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &i);
        setComboboxLastActive(n);
    }
}

string getComboboxText(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(cbn);
    char* n = gtk_combo_box_get_active_text(cb);
    return n?n:"";
}

int getComboboxI(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(cbn);
    return gtk_combo_box_get_active(cb);
}

GtkTreeIter getComboboxIter(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)getGUIBuilder()->get_widget(cbn);
    GtkTreeIter i;
    gtk_combo_box_get_active_iter(cb, &i);
    return i;
}

void notifyUser(string msg1, string msg2) {
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    GtkMessageDialog* dialog = (GtkMessageDialog*)gtk_message_dialog_new(0, flags, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", msg1.c_str());
    gtk_message_dialog_format_secondary_text(dialog, "%s", msg2.c_str());
    gtk_window_set_deletable((GtkWindow*)dialog, false);
    gtk_dialog_run((GtkDialog*)dialog);
    gtk_widget_destroy ((GtkWidget*)dialog);
}

bool askUser(string msg1, string msg2) {
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    GtkMessageDialog* dialog = (GtkMessageDialog*)gtk_message_dialog_new(0, flags, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "%s", msg1.c_str());
    gtk_message_dialog_format_secondary_text(dialog, "%s", msg2.c_str());
    gtk_window_set_deletable((GtkWindow*)dialog, false);

    bool res = false;
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK) res = true;
    gtk_widget_destroy ((GtkWidget*)dialog);
    return res;
}

string askUserInput(string msg) {
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    GtkMessageDialog* dialog = (GtkMessageDialog*)gtk_message_dialog_new(0, flags, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "%s", msg.c_str());
    gtk_window_set_deletable((GtkWindow*)dialog, false);

    GtkEntry* entry = (GtkEntry*)gtk_entry_new();
    GtkVBox* vb = (GtkVBox*)gtk_dialog_get_content_area((GtkDialog*)dialog);
    gtk_box_pack_end((GtkBox*)vb, (GtkWidget*)entry, false, false, 0);
    gtk_widget_show((GtkWidget*)entry);

    string res = "";
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK) res = gtk_entry_get_text(entry);
    gtk_widget_destroy ((GtkWidget*)dialog);
    return res;
}

string askUserPass(string msg) {
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    GtkMessageDialog* dialog = (GtkMessageDialog*)gtk_message_dialog_new(0, flags, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL, "%s", msg.c_str());
    gtk_window_set_deletable((GtkWindow*)dialog, false);

    GtkEntry* entry = (GtkEntry*)gtk_entry_new();
    GtkVBox* vb = (GtkVBox*)gtk_dialog_get_content_area((GtkDialog*)dialog);
    gtk_box_pack_end((GtkBox*)vb, (GtkWidget*)entry, false, false, 0);
    gtk_entry_set_visibility(entry, false);
    gtk_widget_show((GtkWidget*)entry);

    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK) return gtk_entry_get_text(entry);
    return "";
}

OSG::Color4f chooseColor(string drawable, OSG::Color4f current) {
    GtkDrawingArea* darea = (GtkDrawingArea*)getGUIBuilder()->get_object(drawable);
    GtkColorSelectionDialog* cdiag = (GtkColorSelectionDialog*)gtk_color_selection_dialog_new("");
    gtk_window_set_deletable((GtkWindow*)cdiag, false);

    GdkColor c;
    c.pixel = 0;
    c.red = current[0]*65535;
    c.green = current[1]*65535;
    c.blue = current[2]*65535;

    GtkColorSelection* csel = (GtkColorSelection*)gtk_color_selection_dialog_get_color_selection(cdiag);
    gtk_color_selection_set_has_opacity_control(csel, true);
    gtk_color_selection_set_current_color(csel, &c);
    gtk_color_selection_set_current_alpha(csel, current[3]*65535);


    float alpha = 0;
    if (gtk_dialog_run((GtkDialog*)cdiag) == GTK_RESPONSE_OK) {
        gtk_color_selection_get_current_color(csel, &c);
        alpha = gtk_color_selection_get_current_alpha(csel)/65535.0;
    }
    gtk_widget_destroy((GtkWidget*)cdiag);

    gtk_widget_modify_bg((GtkWidget*)darea, GTK_STATE_NORMAL, &c);  // TODO: blend with pattern to show alpha channel

    OSG::Color4f col;
    col[0] = c.red/65535.0;
    col[1] = c.green/65535.0;
    col[2] = c.blue/65535.0;
    col[3] = alpha;
    return col;
}

void setColorChooserColor(string drawable, OSG::Color3f col) {
    GdkColor c;
    c.pixel = 0;
    c.red = col[0]*65535;
    c.green = col[1]*65535;
    c.blue = col[2]*65535;

    GtkDrawingArea* darea = (GtkDrawingArea*)getGUIBuilder()->get_object(drawable);
    gtk_widget_modify_bg((GtkWidget*)darea, GTK_STATE_NORMAL, &c);
}

void setCellRendererCombo(string treeviewcolumn, string combolist, int col, function<void(const char*, GtkTreeIter*)> fkt) {
    GtkListStore* combo_list = (GtkListStore*)getGUIBuilder()->get_object(combolist);
    GtkCellRendererCombo* renderer = (GtkCellRendererCombo*)gtk_cell_renderer_combo_new();

    g_object_set(renderer, "has_entry", false, NULL);
    g_object_set(renderer, "model", combo_list, NULL);
    g_object_set(renderer, "text_column", 0, NULL);
    g_object_set(renderer, "editable", true, NULL);

    GtkTreeViewColumn* column = (GtkTreeViewColumn*)getGUIBuilder()->get_object(treeviewcolumn);
    gtk_tree_view_column_pack_start(column, (GtkCellRenderer*)renderer, true);
    gtk_tree_view_column_add_attribute(column, (GtkCellRenderer*)renderer, "text", col);
    connect_signal((GtkWidget*)renderer, fkt, "changed");
}

void clearContainer(GtkWidget* container) {
    GList* iter = 0;
    GList* children = gtk_container_get_children(GTK_CONTAINER(container));
    for(iter = children; iter != NULL; iter = g_list_next(iter)) gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);
}

void setNotebookPage(string nb, int p) {
    GtkNotebook* nbk = (GtkNotebook*)getGUIBuilder()->get_object(nb);
    gtk_notebook_set_current_page(nbk, p);
}

OSG::VRTexturePtr takeSnapshot() {
    GtkDrawingArea* drawArea = (GtkDrawingArea*)getGUIBuilder()->get_widget("glarea");
    GdkDrawable* src = gtk_widget_get_window((GtkWidget*)drawArea); // 24 bits per pixel ( src->get_depth() )
    int w, h;
    gdk_drawable_get_size(src, &w, &h);
    w -= w%4; h -= h%4;

    GdkColormap* cm = gdk_drawable_get_colormap(src);
    GdkImage* img = gdk_drawable_get_image(src, 0, 0, w, h);
    GdkPixbuf* pxb = gdk_pixbuf_get_from_image(NULL, img, cm, 0,0,0,0,w,h);

    OSG::ImageMTRecPtr res = OSG::Image::create();
    //Image::set(pixFormat, width, height, depth, mipmapcount, framecount, framedelay, data, type, aloc, sidecount);
    res->set(OSG::Image::OSG_RGB_PF, w, h, 1, 0, 1, 0, (const unsigned char*)gdk_pixbuf_get_pixels(pxb), OSG::Image::OSG_UINT8_IMAGEDATA, true, 1);
    return OSG::VRTexture::create(res);
}

void saveSnapshot(string path) {
    if (!exists(getFolderName(path))) return;
    GtkDrawingArea* drawArea = (GtkDrawingArea*)getGUIBuilder()->get_widget("glarea");
    GdkDrawable* src = gtk_widget_get_window((GtkWidget*)drawArea);
    int w, h;
    gdk_drawable_get_size(src, &w, &h);
    int smin = min(w, h);
    int u = max(0.0, w*0.5 - smin*0.5);
    int v = max(0.0, h*0.5 - smin*0.5);

    GdkColormap* cm = gdk_drawable_get_colormap(src);
    GdkImage* img = gdk_drawable_get_image(src, 0, 0, w, h);
    GdkPixbuf* pxb = gdk_pixbuf_get_from_image(NULL, img, cm, u, v,0,0,smin, smin);
    pxb = gdk_pixbuf_scale_simple(pxb, 128, 128, GDK_INTERP_HYPER);
    gdk_pixbuf_save(pxb, path.c_str(), "png", 0, 0, NULL);
}

void saveScene(string path, bool saveas, string encryptionKey) {
    auto scene = OSG::VRScene::getCurrent();
    if (scene == 0) return;
    if (scene->getFlag("write_protected") && !saveas) return;
    scene->setFlag("write_protected", false);
    if (path == "") path = scene->getPath();
    OSG::VRSceneLoader::get()->saveScene(path, 0, encryptionKey);
    saveSnapshot( scene->getIcon() );
    OSG::VRGuiSignals::get()->getSignal("onSaveScene")->triggerPtr<OSG::VRDevice>();
}

int getListStorePos(string ls, string s) {
    GtkListStore* store = (GtkListStore*)getGUIBuilder()->get_object(ls);
    int N = gtk_tree_model_iter_n_children( (GtkTreeModel*)store, NULL );
    for (int i=0; i<N; i++) {
        string si = toString(i);
        GtkTreeIter iter;
        gchar* item = NULL;
        gtk_tree_model_get_iter_from_string((GtkTreeModel*)store, &iter, si.c_str());
        gtk_tree_model_get((GtkTreeModel*)store, &iter, 0, &item, -1);
        string Item = string(item);
        g_free(item);
        if (!item) continue;
        if (Item == s) return i;
    }
    return -1;
}

void fillStringListstore(string ls, vector<string> list) {
    GtkListStore* store = (GtkListStore*)getGUIBuilder()->get_object(ls);
    if (!store) { cout << "ERROR: liststore " << ls << " not found!" << endl; return; }
    gtk_list_store_clear(store);
    for (unsigned int i=0; i<list.size(); i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, list[i].c_str(), -1);
    }
}

void showDialog(string d) {
    GtkWidget* dialog = getGUIBuilder()->get_widget(d);
    //gtk_dialog_run(dialog);
    gtk_widget_show(dialog);
}

void hideDialog(string d) {
    GtkWidget* dialog = getGUIBuilder()->get_widget(d);
    gtk_widget_hide(dialog);
}

void setTooltip(string widget, string tp) {
    GtkWidget* w = getGUIBuilder()->get_widget(widget);
    gtk_widget_set_tooltip_text(w, tp.c_str());
}

GtkImage* loadGTKIcon(GtkImage* img, string path, int w, int h) {
    if ( !exists( path ) ) {
        cout << "Warning (loadGTKIcon): " << path << " not found!" << endl;
        return img;
    }
    if (img == 0) img = (GtkImage*)gtk_image_new();
    gtk_image_set_from_file(img, path.c_str());
    gtk_widget_set_size_request((GtkWidget*)img, w, h);
    return img;
}

bool on_close_frame_clicked(GdkEvent* event, GtkWidget* diag, bool hide) {
    if (hide) gtk_widget_hide(diag);
    return true;
}

void disableDestroyDiag(string diag, bool hide) {
    auto widget = getGUIBuilder()->get_widget(diag);
    connect_signal<bool, GdkEvent*>(widget, bind(on_close_frame_clicked, PL::_1, widget, hide), "delete-event");
}
