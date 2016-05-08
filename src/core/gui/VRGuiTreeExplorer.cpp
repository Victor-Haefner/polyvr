#include "VRGuiTreeExplorer.h"
#include "VRGuiManager.h"

#include <gtkmm/window.h>
#include <cstdarg>
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"

using namespace OSG;

//Tree model columns:
class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:

    int N;
    map<int, void*> columns;
    string cols;

    ModelColumns(string cols) {
        N = cols.size();
        this->cols = cols;

        int i=0;
        for (auto c : cols) {
            if (c == 'i') {
                auto col = new Gtk::TreeModelColumn<int>();
                add(*col);
                columns[i] = (void*)col;
            }
            if (c == 's') {
                auto col = new Gtk::TreeModelColumn<string>();
                add(*col);
                columns[i] = (void*)col;
            }
            i++;
        }
    }

    ~ModelColumns() {
        int i=0;
        for (auto c : cols) {
            if (c == 'i') delete (Gtk::TreeModelColumn<int>*)columns[i];
            if (c == 's') delete (Gtk::TreeModelColumn<string>*)columns[i];
            i++;
        }
    }

    template<class T>
    Gtk::TreeModelColumn<T>& col(int i) {
        return *((Gtk::TreeModelColumn<T>*)columns[i]);
    }
};

VRGuiTreeExplorer::VRGuiTreeExplorer(string cols) {
    auto mgr = VRGuiManager::get();
    win = mgr->newWindow();
    this->cols = cols;

    win->set_title("Gtk::TreeView (ListStore) example");
    win->set_border_width(5);
    win->set_default_size(400, 200);

    auto m_VBox = Gtk::manage(new Gtk::VBox());
    searchEntry = Gtk::manage(new Gtk::Entry());
    auto m_HBox = Gtk::manage(new Gtk::HBox());
    m_TextView = Gtk::manage(new Gtk::TextView());
    auto m_ScrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());
    m_TreeView = Gtk::manage(new Gtk::TreeView());
    auto m_ButtonBox = Gtk::manage(new Gtk::VButtonBox());
    auto m_Button_Quit = Gtk::manage(new Gtk::Button("Close"));

    searchEntry->signal_changed().connect( sigc::mem_fun(*this, &VRGuiTreeExplorer::on_search_edited) );
    m_TreeView->signal_cursor_changed().connect( sigc::mem_fun(*this, &VRGuiTreeExplorer::on_row_select) );

    infoBuffer = Gtk::TextBuffer::create();
    m_TextView->set_editable(0);
    m_TextView->set_buffer(infoBuffer);

    win->add(*m_VBox);

    m_ScrolledWindow->add(*m_TreeView);
    m_ScrolledWindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    m_HBox->pack_start(*m_ScrolledWindow);
    m_HBox->pack_start(*m_TextView);
    m_VBox->pack_start(*searchEntry, 0,1);
    m_VBox->pack_start(*m_HBox);
    m_VBox->pack_start(*m_ButtonBox, Gtk::PACK_SHRINK);

    m_ButtonBox->pack_start(*m_Button_Quit, Gtk::PACK_SHRINK);
    m_ButtonBox->set_border_width(5);
    m_ButtonBox->set_layout(Gtk::BUTTONBOX_END);
    m_Button_Quit->signal_clicked().connect( sigc::mem_fun(*win, &Gtk::Window::hide) );

    //Create the Tree model:
    ModelColumns m_Columns(cols);
    m_refTreeModel = Gtk::TreeStore::create(m_Columns);
    m_TreeView->set_model(m_refTreeModel);

    //Add the TreeView's view columns:
    //This number will be shown with the default numeric formatting.
    int i=0;
    for (auto c : cols) {
        string name = "Col" + toString(i);
        if (c == 'i') m_TreeView->append_column(name, m_Columns.col<int>(i));
        if (c == 's') m_TreeView->append_column(name, m_Columns.col<string>(i));
        i++;
    }

    win->show_all_children();
}

VRGuiTreeExplorer::~VRGuiTreeExplorer() {}

VRGuiTreeExplorerPtr VRGuiTreeExplorer::create(string cols) { return VRGuiTreeExplorerPtr( new VRGuiTreeExplorer(cols) ); }

void VRGuiTreeExplorer::on_search_edited() {
    string txt = searchEntry->get_text();
    // TODO;
}

int VRGuiTreeExplorer::add(int parent, ...) {
    if (parent > 0 && rows.count(parent) == 0) cout << "VRGuiTreeExplorer::add unknown parent " << parent << endl;

    va_list ap;
    va_start(ap, cols.size()); //Requires the last fixed parameter (to get the address)

    ModelColumns m_Columns(cols);
    Gtk::TreeModel::iterator itr;
    if (parent > 0 && rows.count(parent)) itr = m_refTreeModel->append( rows[parent]->children() );
    else itr = m_refTreeModel->append();
    Gtk::TreeModel::Row row = *itr;
    int i=0;
    for (auto c : cols) {
        if ( c == 'i' ) row[m_Columns.col<int>(i)] = va_arg(ap, int);
        if ( c == 's' ) row[m_Columns.col<string>(i)] = string( va_arg(ap, const char*) );
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
    m_refTreeModel->move(itr1, itr2);
}

void VRGuiTreeExplorer::remove(int i) {
    auto itr1 = rows[i];
    if (m_refTreeModel->iter_depth(itr1) == 0) {
        m_refTreeModel->erase(itr1);
        rows.erase(i);
    }
}

void VRGuiTreeExplorer::setInfo(string s) {
    info = s;
    infoBuffer->set_text(s);
}

void VRGuiTreeExplorer::setSelectCallback( shared_ptr< VRFunction<VRGuiTreeExplorer*> > cb ) {
    this->cb = cb;
}

void VRGuiTreeExplorer::on_row_select() {
    selected = m_TreeView->get_selection()->get_selected();
    if(!selected) { setInfo("No data"); return; }

    if (cb) (*cb)(this);
}

Gtk::TreeModel::iterator VRGuiTreeExplorer::getSelected() { return selected; }

template<class T>
T VRGuiTreeExplorer::get(Gtk::TreeModel::iterator itr, int i) {
    ModelColumns m_Columns(cols);
    Gtk::TreeModel::Row row = *itr;
    return row[m_Columns.col<T>(i)];
}

template int VRGuiTreeExplorer::get<int>(Gtk::TreeModel::iterator itr, int i);
template string VRGuiTreeExplorer::get<string>(Gtk::TreeModel::iterator itr, int i);


