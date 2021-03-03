#include <gtk/gtk.h>

#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
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

using namespace std;
namespace PL = std::placeholders;

string gtk_combo_box_get_active_text(GtkComboBox* b) {
	GtkTreeIter itr;
	gtk_combo_box_get_active_iter(b, &itr);
	GtkTreeModel* m = gtk_combo_box_get_model(b);
	char* str = 0;
	gtk_tree_model_get(m, &itr, 0, &str, -1);
	return str ? string(str) : "";
}

void setLabel(string l, string txt) {
    GtkLabel* label = (GtkLabel*)VRGuiBuilder::get()->get_widget(l);
    gtk_label_set_text(label, txt.c_str());
}

void setTextEntry(string entry, string text) {
    GtkEntry* ent = (GtkEntry*)VRGuiBuilder::get()->get_widget(entry);
    gtk_entry_set_text(ent, text.c_str());
}

string getTextEntry(string entry) {
    GtkEntry* ent = (GtkEntry*)VRGuiBuilder::get()->get_widget(entry);
    auto t = gtk_entry_get_text(ent);
    return t?t:"";
}

void setToggleButton(string cb, bool b) {
    GtkToggleButton* tb = (GtkToggleButton*)VRGuiBuilder::get()->get_widget(cb);
    gtk_toggle_button_set_active(tb, b);
}

void setWidgetSensitivity(string table, bool b) {
    GtkWidget* w = VRGuiBuilder::get()->get_widget(table);
    gtk_widget_set_sensitive(w, b);
}

void setWidgetVisibility(string table, bool b, bool p) {
    GtkWidget* w = VRGuiBuilder::get()->get_widget(table);
    if (p) w = gtk_widget_get_parent(w);
    if (b) gtk_widget_show(w);
    else   gtk_widget_hide(w);
}

void setCombobox(string n, int i) {
    GtkComboBox* cb = GTK_COMBO_BOX(VRGuiBuilder::get()->get_widget(n));
    gtk_combo_box_set_active(cb, i);
}

template<typename T>
void setupCallback(string w, T fkt, string event, bool after = false) {
    GtkWidget* W = VRGuiBuilder::get()->get_widget(w);
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
void setNoteBookCallback(string b, function<void(GtkWidget*, guint)> sig) { setupCallback(b, sig, "switch-page", true); }

void setColorChooser(string drawable, function<void(GdkEventButton*)> sig) {
    GtkDrawingArea* darea = (GtkDrawingArea*)VRGuiBuilder::get()->get_widget(drawable);
    gtk_widget_add_events((GtkWidget*)darea, (GdkEventMask)GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events((GtkWidget*)darea, (GdkEventMask)GDK_BUTTON_RELEASE_MASK);
    connect_signal((GtkWidget*)darea, sig, "button_release_event");
}

bool entryFocusProxy(GdkEventFocus* e, function<void()> sig) {
    sig();
    return true;
}

void setEntryCallback(GtkWidget* e, function<void()> sig, bool onEveryChange, bool onFocusOut, bool onActivate) {
    if (onEveryChange) connect_signal(e, sig, "changed");
    else {
        if (onActivate) connect_signal(e, sig, "activate");
        if (onFocusOut) {
            function<bool(GdkEventFocus*)> sig2 = bind(entryFocusProxy, placeholders::_1, sig);
            connect_signal(e, sig2, "focus_out_event");
        }
    }
}

void setEntryCallback(string b, function<void()> sig, bool onEveryChange, bool onFocusOut, bool onActivate) {
    GtkWidget* e = VRGuiBuilder::get()->get_widget(b);
    setEntryCallback(e, sig, onEveryChange, onFocusOut, onActivate);
}

bool getCheckButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getRadioButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getRadioToolButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getToggleButtonState(string b) {
    GtkToggleButton* tbut = (GtkToggleButton*)VRGuiBuilder::get()->get_widget(b);
    return gtk_toggle_button_get_active(tbut);
}

bool getToggleToolButtonState(string b) {
    GtkToggleToolButton* tbut = (GtkToggleToolButton*)VRGuiBuilder::get()->get_widget(b);
    return gtk_toggle_tool_button_get_active(tbut);
}

string getTreeviewCell(string treeview, GtkTreeIter iter, int column) {
    GtkTreeView* tree_view = (GtkTreeView*)VRGuiBuilder::get()->get_object(treeview);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    gchar* n;
    gtk_tree_model_get(model, &iter, column, &n, -1);
    string name=n?n:"";
    g_free(n);
    return name;
}

string getTreeviewSelected(string treeview, int column) {
    GtkTreeView* tree_view = (GtkTreeView*)VRGuiBuilder::get()->get_object(treeview);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        return getTreeviewCell(treeview, iter, column);
    }
    return "";
}

bool hasTreeviewSelection(string treeview) {
    GtkTreeView* tree_view = (GtkTreeView*)VRGuiBuilder::get()->get_object(treeview);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    return gtk_tree_selection_get_selected(sel, &model, &iter);
}

void setTreeViewSelectedText(string tv, int column, string data) {
    GtkTreeView* tree_view = (GtkTreeView*)VRGuiBuilder::get()->get_object(tv);
    GtkTreeSelection* sel = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gtk_tree_model_get(model, &iter, column, data.c_str(), -1);
    }
}

void setButtonText(string b, string txt ) {
    GtkButton* bu = (GtkButton*)VRGuiBuilder::get()->get_widget(b);
    gtk_button_set_label(bu, txt.c_str());
}

float getSliderState(string s) {
    GtkHScale* hs = (GtkHScale*)VRGuiBuilder::get()->get_widget(s);
    return gtk_adjustment_get_value((GtkAdjustment*)hs);
}

bool keySignalProxy(GdkEventKey* e, string k, function<void(void)> sig ) {
    if (gdk_keyval_name(e->keyval) == k) { sig(); return true; }
    return false;
}

void setComboboxLastActive(string n) { // TODO: google how to get N rows!
    GtkComboBox* cb = (GtkComboBox*)VRGuiBuilder::get()->get_widget(n);
    GtkTreeModel* model = gtk_combo_box_get_model(cb);
    int i = gtk_tree_model_iter_n_children(model, 0);
    gtk_combo_box_set_active(cb, i-1);
}

void eraseComboboxActive(string n) {
    GtkComboBox* cb = (GtkComboBox*)VRGuiBuilder::get()->get_widget(n);
    GtkTreeModel* model = gtk_combo_box_get_model(cb);
    GtkTreeIter i;
    bool b = gtk_combo_box_get_active_iter(cb, &i);
    if (b) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &i);
        setComboboxLastActive(n);
    }
}

string getComboboxPtrText(GtkComboBox* cb) {
    if (!cb) return "";
#if GTK_MAJOR_VERSION == 2
    char* n = gtk_combo_box_get_active_text(cb);
#else
    GtkTreeIter itr;
    bool b = gtk_combo_box_get_active_iter(cb, &itr);
    if (!b) return "";

    char* n = 0;
    GtkTreeModel* model = gtk_combo_box_get_model(cb);
    gtk_tree_model_get(model, &itr, 0, &n, -1); // Gtk-CRITICAL **: 12:00:31.666: gtk_list_store_get_value: assertion 'iter_is_valid (iter, list_store)' failed
#endif
    return n?n:"";
}

string getComboboxText(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)VRGuiBuilder::get()->get_widget(cbn);
    return getComboboxPtrText(cb);
}

int getComboboxI(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)VRGuiBuilder::get()->get_widget(cbn);
    return gtk_combo_box_get_active(cb);
}

GtkTreeIter getComboboxIter(string cbn) {
    GtkComboBox* cb = (GtkComboBox*)VRGuiBuilder::get()->get_widget(cbn);
    GtkTreeIter i;
    gtk_combo_box_get_active_iter(cb, &i);
    return i;
}

GtkMessageDialog* basicDialog(string msg, GtkButtonsType buttons) {
    GtkDialogFlags flags = GTK_DIALOG_MODAL;
    GtkMessageDialog* dialog = (GtkMessageDialog*)gtk_message_dialog_new(0, flags, GTK_MESSAGE_WARNING, buttons, "%s", msg.c_str());
    gtk_window_set_deletable((GtkWindow*)dialog, false);
    return dialog;
}

void notifyUser(string msg1, string msg2) {
    GtkMessageDialog* dialog = basicDialog(msg1, GTK_BUTTONS_OK);
    gtk_message_dialog_format_secondary_text(dialog, "%s", msg2.c_str());
    gtk_dialog_run((GtkDialog*)dialog);
    gtk_widget_destroy ((GtkWidget*)dialog);
}

bool askUser(string msg1, string msg2) {
    GtkMessageDialog* dialog = basicDialog(msg1, GTK_BUTTONS_OK_CANCEL);
    gtk_message_dialog_format_secondary_text(dialog, "%s", msg2.c_str());

    bool res = false;
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK) res = true;
    gtk_widget_destroy ((GtkWidget*)dialog);
    return res;
}

GtkEntry* addDialogEntry(GtkMessageDialog* dialog) {
    GtkEntry* entry = (GtkEntry*)gtk_entry_new();
    GtkVBox* vb = (GtkVBox*)gtk_dialog_get_content_area((GtkDialog*)dialog);
    gtk_box_pack_end((GtkBox*)vb, (GtkWidget*)entry, false, false, 0);
    gtk_widget_show((GtkWidget*)entry);
    return entry;
}

string runEntryDialog(GtkMessageDialog* dialog, GtkEntry* entry) {
    string res = "";
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_OK) res = gtk_entry_get_text(entry);
    gtk_widget_destroy ((GtkWidget*)dialog);
    return res;
}

string askUserInput(string msg) {
    GtkMessageDialog* dialog = basicDialog(msg, GTK_BUTTONS_OK_CANCEL);
    GtkEntry* entry = addDialogEntry(dialog);
    return runEntryDialog(dialog, entry);
}

string askUserPass(string msg) {
    GtkMessageDialog* dialog = basicDialog(msg, GTK_BUTTONS_OK_CANCEL);
    GtkEntry* entry = addDialogEntry(dialog);
    gtk_entry_set_visibility(entry, false);
    return runEntryDialog(dialog, entry);
}

OSG::Color4f chooseColor(string drawable, OSG::Color4f current) {
    GtkDrawingArea* darea = (GtkDrawingArea*)VRGuiBuilder::get()->get_widget(drawable);
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

bool drawBG(GtkWidget *widget, cairo_t *cr, gpointer col) {
    auto context = gtk_widget_get_style_context (widget);
    int width = gtk_widget_get_allocated_width (widget);
    int height = gtk_widget_get_allocated_height (widget);
    gtk_render_background (context, cr, 0, 0, width, height);

    GdkRGBA color;
    gtk_style_context_get_color(context, gtk_style_context_get_state (context), &color);
    gdk_cairo_set_source_rgba(cr, &color);
    cairo_fill(cr);
    return false;
}

void setColorChooserColor(GtkWidget* drawable, OSG::Color3f col) {
    GdkColor c;
    c.pixel = 0;
    c.red = col[0]*65535;
    c.green = col[1]*65535;
    c.blue = col[2]*65535;

    gtk_widget_modify_bg(drawable, GTK_STATE_NORMAL, &c);
    g_signal_connect(G_OBJECT(drawable), "draw", G_CALLBACK(drawBG), NULL);
}

void setColorChooserColor(string drawable, OSG::Color3f col) {
    setColorChooserColor(VRGuiBuilder::get()->get_widget(drawable), col);
}

void setCellRendererCombo(string treeviewcolumn, string combolist, int col, function<void(const char*, GtkTreeIter*)> fkt) {
    GtkListStore* combo_list = (GtkListStore*)VRGuiBuilder::get()->get_object(combolist);
    GtkCellRendererCombo* renderer = (GtkCellRendererCombo*)gtk_cell_renderer_combo_new();

    g_object_set(renderer, "has_entry", false, NULL);
    g_object_set(renderer, "model", combo_list, NULL);
    g_object_set(renderer, "text_column", 0, NULL);
    g_object_set(renderer, "editable", true, NULL);

    GtkTreeViewColumn* column = (GtkTreeViewColumn*)VRGuiBuilder::get()->get_widget(treeviewcolumn);
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
    GtkNotebook* nbk = (GtkNotebook*)VRGuiBuilder::get()->get_widget(nb);
    gtk_notebook_set_current_page(nbk, p);
}

OSG::VRTexturePtr takeSnapshot() {
    GtkWidget* drawArea = VRGuiBuilder::get()->get_widget("glarea");
    GdkWindow* src = gtk_widget_get_window(drawArea); // 24 bits per pixel ( src->get_depth() )

    GtkAllocation a;
    gtk_widget_get_allocation(drawArea, &a);
    int w = a.width;
    int h = a.height;

    w -= w%4; h -= h%4;

    GdkPixbuf* pxb = gdk_pixbuf_get_from_window(src, a.x,a.y,w,h);

    OSG::ImageMTRecPtr res = OSG::Image::create();
    //Image::set(pixFormat, width, height, depth, mipmapcount, framecount, framedelay, data, type, aloc, sidecount);
    res->set(OSG::Image::OSG_RGB_PF, w, h, 1, 0, 1, 0, (const unsigned char*)gdk_pixbuf_get_pixels(pxb), OSG::Image::OSG_UINT8_IMAGEDATA, true, 1);
    //cout << "takeSnapshot1 " << drawArea << " " << VRGuiBuilder::get()->get_widget("hbox1") << " " << VRGuiBuilder::get()->get_widget("vbox5") << endl;
    //cout << "takeSnapshot2 " << src << " " << src2 << " " << src3 << endl;
    return OSG::VRTexture::create(res);
}

void saveSnapshot(string path) {
    cout << "saveSnapshot " << path << endl;
    if (!exists(getFolderName(path))) return;
    GtkWidget* drawArea = VRGuiBuilder::get()->get_widget("glarea");
    GdkWindow* src = gtk_widget_get_window(drawArea);
    int w = gdk_window_get_width(src);
    int h = gdk_window_get_height(src);
    int smin = min(w, h);
    int u = max(0.0, w*0.5 - smin*0.5);
    int v = max(0.0, h*0.5 - smin*0.5);

    GdkPixbuf* pxb = gdk_pixbuf_get_from_window(src, u,v,smin,smin);
    pxb = gdk_pixbuf_scale_simple(pxb, 128, 128, GDK_INTERP_HYPER);
    gdk_pixbuf_save(pxb, path.c_str(), "png", 0, 0, NULL);
    cout << " saveSnapshot done " << path << endl;
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
    GtkListStore* store = (GtkListStore*)VRGuiBuilder::get()->get_object(ls);
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

void gtk_list_store_clear_debug(GtkListStore *list_store) {
	//GtkListStorePrivate *priv;
	GtkTreeIter iter;

	g_return_if_fail(GTK_IS_LIST_STORE(list_store));
	//priv = list_store->priv;

	while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL) > 0) {
		//iter.stamp = priv->stamp;
		//iter.user_data = g_sequence_get_begin_iter(priv->seq);
		auto path = gtk_tree_path_new_from_string("0");
		gtk_tree_model_get_iter(GTK_TREE_MODEL(list_store), &iter, path);

		char* str = 0;
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 0, &str, -1);

		gtk_list_store_remove(list_store, &iter);
	}

	//gtk_list_store_increment_stamp(list_store);
}

void fillStringListstore(string ls, vector<string> list) {
    GtkListStore* store = GTK_LIST_STORE( VRGuiBuilder::get()->get_object(ls) );
    if (!store) { cout << "ERROR: liststore " << ls << " not found!" << endl; return; }
	//int number_of_rows = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
	gtk_list_store_clear_debug(store);
    for (unsigned int i=0; i<list.size(); i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, list[i].c_str(), -1);
    }
}

void showDialog(string d) {
    GtkWidget* dialog = VRGuiBuilder::get()->get_widget(d);
    //gtk_dialog_run(dialog);
    gtk_widget_show_all(dialog);
}

void hideDialog(string d) {
    GtkWidget* dialog = VRGuiBuilder::get()->get_widget(d);
    gtk_widget_hide(dialog);
}

void setTooltip(string widget, string tp) {
    GtkWidget* w = VRGuiBuilder::get()->get_widget(widget);
    gtk_widget_set_tooltip_text(w, tp.c_str());
}

GtkImage* loadGTKIcon(GtkImage* img, string path, int w, int h) {
    if ( !exists( path ) ) {
        cout << "Warning (loadGTKIcon): " << path << " not found!" << endl;
        return img;
    }
    if (img == 0) img = (GtkImage*)gtk_image_new();
    auto pbuf = gdk_pixbuf_new_from_file(path.c_str(), 0);
    int W = gdk_pixbuf_get_width(pbuf);
    int H = gdk_pixbuf_get_height(pbuf);
    int Ox = floor((W-w)*0.5);
    int Oy = floor((H-h)*0.5);
    auto pbuf2 = gdk_pixbuf_new_subpixbuf(pbuf, max(0,Ox), max(0,Oy), min(w,W), min(h,H));
    gtk_image_set_from_pixbuf(img, pbuf2);
    return img;
}

bool on_close_frame_clicked(GdkEvent* event, GtkWidget* diag, bool hide) {
    if (hide) gtk_widget_hide(diag);
    return true;
}

void disableDestroyDiag(GtkWidget* widget, bool hide) {
    connect_signal<bool, GdkEvent*>(widget, bind(on_close_frame_clicked, PL::_1, widget, hide), "delete-event");
}

void disableDestroyDiag(string diag, bool hide) {
    auto widget = VRGuiBuilder::get()->get_widget(diag);
    disableDestroyDiag(widget, hide);
}
