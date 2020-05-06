#ifndef VRGUIUTILS_H_INCLUDED
#define VRGUIUTILS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm-2.4/gtkmm/treemodel.h>
#include <sigc++/functors/slot.h>
#include "core/objects/VRObjectFwd.h"

//OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiBuilder {
    private:
        map<string, GtkWidget*> widgets;
        map<string, GtkObject*> objects;

    public:
        VRGuiBuilder();
        ~VRGuiBuilder();

        void read(string path);

        GtkWidget* get_widget(string name);
        GtkObject* get_object(string name);
};

VRGuiBuilder* getGUIBuilder(bool standalone = false);

template<typename T1, typename T2, typename T3>
struct functor3 {
    function<void(T1*, T2*, T3*)> cb;
    functor3(function<void(T1*, T2*, T3*)> f) : cb(f) {}
    static void callback(GtkWidget* widget, T1* t1, T2* t2, T3* t3, gpointer* data) {
        ((functor3*)data)->cb(t1,t2,t3);
    }
};

template<typename T1, typename T2, typename T3>
void connect_signal3(GtkWidget* widget, function<void(T1*, T2*, T3*)> cb, string event, bool after = false) {
    auto proxy = new functor3<T1,T2,T3>(cb);
    if (after)  g_signal_connect_after((GtkObject*)widget, event.c_str(), G_CALLBACK((functor3<T1,T2,T3>::callback)), proxy);
    else        g_signal_connect((GtkObject*)widget, event.c_str(), G_CALLBACK((functor3<T1,T2,T3>::callback)), proxy);
}

template<typename T1, typename T2>
struct functor2 {
    function<void(T1*, T2*)> cb;
    functor2(function<void(T1*, T2*)> f) : cb(f) {}
    static void callback(GtkWidget* widget, T1* t1, T2* t2, gpointer* data) {
        ((functor2*)data)->cb(t1,t2);
    }
};

template<typename T1, typename T2>
void connect_signal2(GtkWidget* widget, function<void(T1*, T2*)> cb, string event, bool after = false) {
    auto proxy = new functor2<T1,T2>(cb);
    if (after)  g_signal_connect_after((GtkObject*)widget, event.c_str(), G_CALLBACK((functor2<T1,T2>::callback)), proxy);
    else        g_signal_connect((GtkObject*)widget, event.c_str(), G_CALLBACK((functor2<T1,T2>::callback)), proxy);
}

template<typename T>
struct functor {
    function<void(T*)> cb;
    functor(function<void(T*)> f) : cb(f) {}
    static void callback(GtkWidget* widget, T* t, gpointer* data) {
        ((functor*)data)->cb(t);
    }
};

template<typename T>
void connect_signal(GtkWidget* widget, function<void(T*)> cb, string event, bool after = false) {
    auto proxy = new functor<T>(cb);
    if (after)  g_signal_connect_after((GtkObject*)widget, event.c_str(), G_CALLBACK(functor<T>::callback), proxy);
    else        g_signal_connect((GtkObject*)widget, event.c_str(), G_CALLBACK(functor<T>::callback), proxy);
}

void connect_signal_void(GtkWidget* widget, function<void()> cb, string event);

void setWidgetSensitivity(string e, bool b);

// callback helpers
void setButtonCallback(string b, function<void(GtkButton*)> sig );
void setToggleButtonCallback(string b, function<void(GtkToggleButton*)> sig );
void setToolButtonCallback(string b, function<void(GtkToolButton*)> sig );
void setCheckButtonCallback(string cb, function<void(GtkCheckButton*)> sig );
void setRadioToolButtonCallback(string cb, function<void(GtkRadioToolButton*)> sig );
void setRadioButtonCallback(string cb, function<void(GtkRadioButton*)> sig );
void setComboboxCallback(string b, function<void(GtkComboBox*)> sig);
void setTreeviewSelectCallback(string treeview, function<void(GtkTreeView*)> sig);
void setCellRendererCallback(string renderer, function<void(GtkEntry*, const char*, const char*)> sig, bool after = true);
void setNoteBookCallback(string nb, function<void(GtkNotebook*, GtkNotebookPage*, guint, gpointer)> sig);
void setSliderCallback(string s, function<void(GtkHScale*,int,double)> sig);
void setEntryCallback(string e, function<void(GtkEntry*)> sig, bool onEveryChange = false, bool onFocusOut = true, bool onActivate = true);

// TEXT
void setLabel(string l, string txt);
void setTextEntry(string entry, string text);
string getTextEntry(string entry);
void focusEntry(string e);

// BUTTONS
void setToggleButton(string b, bool v);
bool getCheckButtonState(string b);
bool getRadioButtonState(string b);
bool getRadioToolButtonState(string b);
bool getToggleButtonState(string b);
void setRadioButton(string cb, bool b );
void setRadioToolButton(string cb, bool b );
void setButtonText(string cb, string txt );

// COMBOBOX
void setComboboxLastActive(string cb);
void setCombobox(string cb, int i);
int getListStorePos(string ls, string s);
string getComboboxText(string cbn);
int getComboboxI(string cbn);
GtkTreeIter getComboboxIter(string cbn);
void eraseComboboxActive(string cb);

// SLIDER
float getSliderState(string s);

// LISTVIEWS
void fillStringListstore(string ls, vector<string> list);
void setCellRendererCombo(string treeviewcolumn, string combolist, int col, function<void(GtkCellRendererCombo*, gchar*, GtkTreeIter*)> fkt);
GtkTreeSelection* getTreeviewSelected(string treeview);
void selectTreestoreRow(string treeview, GtkTreePath* p, GtkTreeViewColumn* c);
void focusTreeView(string treeview);

// STUFF
void setNotebookPage(string nb, int p);
void setTooltip(string widget, string tp);

bool keySignalProxy(GdkEventKey* e, string k, sigc::slot<void> sig );
bool askUser(string msg1, string msg2);
string askUserInput(string msg);
string askUserPass(string msg);
OSG::Color4f chooseColor(string drawable, OSG::Color4f current);
void setColorChooser(string drawable, function<void(GdkEventButton*)> sig);
void setColorChooserColor(string drawable, OSG::Color3f col);

GtkImage* loadGTKIcon(GtkImage* img, string path, int w, int h);

OSG::VRTexturePtr takeSnapshot();
void saveScene(string path = "", bool saveas = false, string encryptionKey = "");

void showDialog(string d);
void hideDialog(string d);

//OSG_END_NAMESPACE;

#endif // VRGUIUTILS_H_INCLUDED
