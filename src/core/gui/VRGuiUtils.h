#ifndef VRGUIUTILS_H_INCLUDED
#define VRGUIUTILS_H_INCLUDED

#include <gtk/gtk.h>
#include <map>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"

struct _GtkWidget;
#if GTK_MAJOR_VERSION == 2
struct _GtkObject;
#else
struct _GObject;
#define _GtkObject _GObject
#endif
struct _GtkButton;
struct _GtkToggleButton;
struct _GtkToolButton;
struct _GtkCheckButton;
struct _GtkRadioToolButton;
struct _GtkRadioButton;
struct _GtkComboBox;
struct _GtkEntry;
struct _GtkNotebook;
struct _GtkHScale;
struct _GtkImage;
struct _GtkCellRendererCombo;
struct _GtkTreeIter;
struct _GtkTreePath;
struct _GtkTreeViewColumn;
struct _GdkEventKey;
struct _GdkEventButton;
struct _GtkBuilder;
typedef void* gpointer;
typedef unsigned int guint;

//OSG_BEGIN_NAMESPACE;
using namespace std;

template<class R, class... Args>
struct functor {
    function<R(Args...)> cb;
    functor(function<R(Args...)> f) : cb(f) {}

    //static void callback(_GtkWidget* widget, T* t, gpointer* data) {

    static R callback(_GtkWidget* widget, Args... args, gpointer* data) {
        return ((functor*)data)->cb(args...);
    }
};

template<class... Args>
struct functor<void, Args...> {
    function<void(Args...)> cb;
    functor(function<void(Args...)> f) : cb(f) {}

    //static void callback(_GtkWidget* widget, T* t, gpointer* data) {

    static void callback(_GtkWidget* widget, Args... args, gpointer* data) {
        ((functor*)data)->cb(args...);
    }
};

template<class R, class... Args, typename T1>
void connect_signal(T1* widget, function<R(Args...)> cb, string event, bool after = false) {
    auto proxy = new functor<R, Args...>(cb);
    if (after)  g_signal_connect_after((_GtkObject*)widget, event.c_str(), G_CALLBACK((functor<R, Args...>::callback)), proxy);
    else        g_signal_connect((_GtkObject*)widget, event.c_str(), G_CALLBACK((functor<R, Args...>::callback)), proxy);
}

template<class R, class... Args, typename T1, typename T2>
void connect_signal(T1* widget, T2 bin, string event, bool after = false) {
    function<R(Args...)> cb = bin;
    connect_signal(widget, cb, event, after);
}

void clearContainer(_GtkWidget* container);
void setWidgetVisibility(string e, bool b);
void setWidgetSensitivity(string e, bool b);
void disableDestroyDiag(GtkWidget* widget, bool hide = true);
void disableDestroyDiag(string diag, bool hide = true);

// callback helpers
void setButtonCallback(string b, function<void()> sig );
void setToggleButtonCallback(string b, function<void()> sig );
void setToolButtonCallback(string b, function<void()> sig );
void setCheckButtonCallback(string cb, function<void()> sig );
void setRadioToolButtonCallback(string cb, function<void()> sig );
void setRadioButtonCallback(string cb, function<void()> sig );
void setComboboxCallback(string b, function<void()> sig);
void setTreeviewSelectCallback(string treeview, function<void()> sig);
void setCellRendererCallback(string renderer, function<void(char*, char*)> sig, bool after = true);
void setNoteBookCallback(string nb, function<void(_GtkWidget*, guint, gpointer)> sig);
void setSliderCallback(string s, function<bool(int,double)> sig);
void setEntryCallback(string e, function<void()> sig, bool onEveryChange = false, bool onFocusOut = true, bool onActivate = true);
void setEntryCallback(GtkWidget* e, function<void()> sig, bool onEveryChange = false, bool onFocusOut = true, bool onActivate = true);

// TEXT
void setLabel(string l, string txt);
void setTextEntry(string entry, string text);
string getTextEntry(string entry);

// BUTTONS
void setToggleButton(string b, bool v);
bool getCheckButtonState(string b);
bool getRadioButtonState(string b);
bool getRadioToolButtonState(string b);
bool getToggleButtonState(string b);
bool getToggleToolButtonState(string b);
void setButtonText(string cb, string txt );

// COMBOBOX
void setComboboxLastActive(string cb);
void setCombobox(string cb, int i);
int getListStorePos(string ls, string s);
string getComboboxPtrText(_GtkComboBox* cb);
string getComboboxText(string cbn);
string gtk_combo_box_get_active_text(GtkComboBox* b);
int getComboboxI(string cbn);
_GtkTreeIter getComboboxIter(string cbn);
void eraseComboboxActive(string cb);

// SLIDER
float getSliderState(string s);

// LISTVIEWS
void fillStringListstore(string ls, vector<string> list);
void setCellRendererCombo(string treeviewcolumn, string combolist, int col, function<void(const char*, _GtkTreeIter*)> fkt);
void delTreeviewSelected(string treeview);

// STUFF
void setNotebookPage(string nb, int p);
void setTooltip(string widget, string tp);

bool keySignalProxy(_GdkEventKey* e, string k, function<void(void)> sig );
void notifyUser(string msg1, string msg2);
bool askUser(string msg1, string msg2);
string askUserInput(string msg);
string askUserPass(string msg);
OSG::Color4f chooseColor(string drawable, OSG::Color4f current);
void setColorChooser(string drawable, function<void(_GdkEventButton*)> sig);
void setColorChooserColor(string drawable, OSG::Color3f col);

_GtkImage* loadGTKIcon(_GtkImage* img, string path, int w, int h);

OSG::VRTexturePtr takeSnapshot();
void saveScene(string path = "", bool saveas = false, string encryptionKey = "");

void showDialog(string d);
void hideDialog(string d);

//OSG_END_NAMESPACE;

#endif // VRGUIUTILS_H_INCLUDED
