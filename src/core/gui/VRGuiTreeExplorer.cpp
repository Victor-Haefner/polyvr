#include "VRGuiTreeExplorer.h"
#include "VRGuiManager.h"
#include "VRGuiUtils.h"

#include <cstdarg>
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"

#include <gtk/gtk.h>

using namespace OSG;

// this was developed for exploring the STEPCode BREP structure

VRGuiTreeExplorer::VRGuiTreeExplorer(string cols, string title) {
    cout << "VRGuiTreeExplorer::VRGuiTreeExplorer" << endl;
    auto mgr = VRGuiManager::get();
    win = mgr->newWindow();
    this->cols = cols;

    gtk_window_set_title(GTK_WINDOW(win), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(win), 400, 200);

    auto m_VBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    searchEntry = GTK_ENTRY(gtk_entry_new());
    auto m_HBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    m_TextView = GTK_TEXT_VIEW(gtk_text_view_new());
    auto m_ScrolledWindow = gtk_scrolled_window_new(0,0);
    m_TreeView = GTK_TREE_VIEW(gtk_tree_view_new());
    auto m_ButtonBox = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    auto m_Button_Quit = gtk_button_new_with_label("Close");

    setEntryCallback(GTK_WIDGET(searchEntry), bind(&VRGuiTreeExplorer::on_search_edited, this), false);
    function<void()> fktE = bind(&VRGuiTreeExplorer::on_row_select, this);
    connect_signal(GTK_WIDGET(m_TreeView), fktE, "cursor_changed", false);

    infoBuffer = gtk_text_buffer_new(0);
    gtk_text_view_set_editable(m_TextView, false);
    gtk_text_view_set_buffer(m_TextView, infoBuffer);

    gtk_container_add(GTK_CONTAINER(win), m_VBox);
    gtk_container_add(GTK_CONTAINER(m_ScrolledWindow), GTK_WIDGET(m_TreeView));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_ScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


    gtk_box_pack_start(GTK_BOX(m_HBox), m_ScrolledWindow, true, true, 1);
    gtk_box_pack_start(GTK_BOX(m_HBox), GTK_WIDGET(m_TextView), true, true, 1);
    gtk_box_pack_start(GTK_BOX(m_VBox), GTK_WIDGET(searchEntry), false, true, 1);
    gtk_box_pack_start(GTK_BOX(m_VBox), m_HBox, true, true, 1);
    gtk_box_pack_start(GTK_BOX(m_VBox), m_ButtonBox, false, true, 1);

    gtk_box_pack_start(GTK_BOX(m_ButtonBox), m_Button_Quit, false, true, 1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(m_ButtonBox), GTK_BUTTONBOX_END);
    function<void()> fktQ = bind(gtk_widget_hide, GTK_WIDGET(win));
    connect_signal(GTK_WIDGET(m_Button_Quit), fktQ, "clicked", false);

    //Create the Tree model:
    vector<GType> types;
    for (auto c : cols) {
        if (c == 'i') types.push_back(G_TYPE_INT);
        if (c == 's') types.push_back(G_TYPE_STRING);
        if (c == 'p') types.push_back(G_TYPE_POINTER);
    }
    m_refTreeModel = gtk_tree_store_newv(cols.size(), &types[0]);
    gtk_tree_view_set_model(m_TreeView, GTK_TREE_MODEL(m_refTreeModel));
    gtk_tree_view_set_enable_tree_lines(m_TreeView, true);
    gtk_tree_view_set_headers_visible(m_TreeView, false);

    //Add the TreeView's view columns:
    //This number will be shown with the default numeric formatting.
    for (int i=0; i<cols.size(); i++) {
        if (cols[i] == 'p') continue;
        string name = "Col" + toString(i);
        GtkTreeViewColumn* column = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(column, name.c_str());
        GtkCellRenderer* cellrenderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(column, cellrenderer, true);
        gtk_tree_view_column_add_attribute(column, cellrenderer, "text", i);
        gtk_tree_view_append_column(m_TreeView, column);
    }

    gtk_widget_show_all(GTK_WIDGET(win));
    //gtk_widget_hide(GTK_WIDGET(win));

    selected = new GtkTreeIter();
    cout << "VRGuiTreeExplorer::VRGuiTreeExplorer done" << endl;
}

VRGuiTreeExplorer::~VRGuiTreeExplorer() { delete selected; }

VRGuiTreeExplorerPtr VRGuiTreeExplorer::create(string cols, string title) { return VRGuiTreeExplorerPtr( new VRGuiTreeExplorer(cols, title) ); }

void VRGuiTreeExplorer::on_search_edited() {
    string txt = gtk_entry_get_text(searchEntry);
    if (txt.size() < 3) return;

    auto match = [&](string s) {
        vector<int> res;
        uint pos = s.find(txt, 0);
        while(pos != string::npos && pos <= s.size()) {
            res.push_back(pos);
            pos = s.find(txt, pos+1);
        }
        return res;
    };

    auto expand = [&](GtkTreeIter& i) {
        auto p = gtk_tree_model_get_path(GTK_TREE_MODEL(m_refTreeModel), &i);
        gtk_tree_view_expand_to_path(m_TreeView, p);
    };

    gtk_tree_view_collapse_all(m_TreeView);
    int N = 0;
    for (auto r : rows) {
        string row_string;
        for (int i=0; i<cols.size(); i++) {
            if (cols[i] == 's') {
                gchar* ctxt = 0;
                gtk_tree_model_get(GTK_TREE_MODEL(m_refTreeModel), &r.second, i, &ctxt, -1);
                string txt = ctxt ? ctxt : "";
                g_free(ctxt);
                row_string += txt;
            }

            if (cols[i] == 'i') {
                int j = 0;
                gtk_tree_model_get(GTK_TREE_MODEL(m_refTreeModel), &r.second, i, &j, -1);
                string txt = toString(j);
                row_string += txt;
            }
        }

        auto res = match(row_string);
        if (res.size() > 0) {
            N += res.size();
            expand(r.second);
        }
    }

    cout << "search for txt resulted in " << N << " finds\n";
}

int VRGuiTreeExplorer::add(int parent, int N, ...) {
    if (parent > 0 && rows.count(parent) == 0) cout << "VRGuiTreeExplorer::add unknown parent " << parent << endl;
    if (N != int(cols.size())) { cout << "VRGuiTreeExplorer::add wrong size " << N << endl; return -1; }


    //ModelColumns m_Columns(cols);
    GtkTreeIter itr;
    if (parent > 0 && rows.count(parent)) gtk_tree_store_append(m_refTreeModel, &itr, &rows[parent]);
    else gtk_tree_store_append(m_refTreeModel, &itr, 0);

    va_list ap;
    va_start(ap, N); //Requires the last fixed parameter (to get the address)
    int i=0;
    for (auto c : cols) {
        if ( c == 'i' ) gtk_tree_store_set(m_refTreeModel, &itr, i, va_arg(ap, int), -1);
        if ( c == 's' ) gtk_tree_store_set(m_refTreeModel, &itr, i, va_arg(ap, const char*), -1);
        if ( c == 'p' ) gtk_tree_store_set(m_refTreeModel, &itr, i, va_arg(ap, void*), -1);
        i++;
    }
    va_end(ap);

    static int rkey = 0; rkey++;
    rows[rkey] = itr;
    return rkey;
}

void VRGuiTreeExplorer::move(int i1, int i2) {
    auto itr1 = rows[i1];
    auto itr2 = rows[i2];
    gtk_tree_store_move_after(m_refTreeModel, &itr1, &itr2);
}

void VRGuiTreeExplorer::remove(int i) {
    auto itr1 = &rows[i];
    int r = gtk_tree_store_iter_depth(m_refTreeModel, itr1);
    if (r == 0) {
        gtk_tree_store_remove(m_refTreeModel, itr1);
        rows.erase(i);
    }
}

void VRGuiTreeExplorer::setInfo(string s) {
    info = s;
    gtk_text_buffer_set_text(infoBuffer, s.c_str(), s.size());
}

void VRGuiTreeExplorer::setSelectCallback( shared_ptr< VRFunction<VRGuiTreeExplorer*> > cb ) {
    this->cb = cb;
}

void VRGuiTreeExplorer::on_row_select() {
    GtkTreeSelection* sel = gtk_tree_view_get_selection(m_TreeView);
    GtkTreeModel* model = GTK_TREE_MODEL(m_refTreeModel);

    if (!gtk_tree_selection_get_selected(sel, &model, selected)) {
        setInfo("No data");
        return;
    }

    if (cb) (*cb)(this);
}

GtkTreeIter* VRGuiTreeExplorer::getSelected() { return selected; }

template<class T>
T VRGuiTreeExplorer::get(GtkTreeIter* itr, int i) {
    T data;
    gtk_tree_model_get(GTK_TREE_MODEL(m_refTreeModel), itr, i, &data, -1);
    return data;
}

template int VRGuiTreeExplorer::get<int>(GtkTreeIter* itr, int i);
template string VRGuiTreeExplorer::get<string>(GtkTreeIter* itr, int i);
template const char* VRGuiTreeExplorer::get<const char*>(GtkTreeIter* itr, int i);
template void* VRGuiTreeExplorer::get<void*>(GtkTreeIter* itr, int i);


