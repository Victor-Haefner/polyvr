#ifndef VRGUIEDITOR_H_INCLUDED
#define VRGUIEDITOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"

struct _GtkWidget;
struct _GtkSourceLanguage;
struct _GtkSourceBuffer;
struct _GtkTextBuffer;
struct _GtkTextTag;
struct _GdkEventKey;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiEditor {
    private:
        _GtkWidget* editor = 0;
        _GtkSourceBuffer* sourceBuffer = 0;
        _GtkSourceLanguage* python = 0;
        _GtkSourceLanguage* web = 0;
        _GtkSourceLanguage* glsl = 0;
        _GtkTextBuffer* editorBuffer = 0;
	    map<string, VRUpdateCbPtr> keyBindings;

	    string selection;

        map<string, _GtkTextTag*> editorStyles;
        map<string, bool> styleStates;

        void printViewerLanguages();
        bool on_editor_shortkey( _GdkEventKey* e );
        void addStyle( string style, string fg, string bg, bool italiq, bool bold, bool underlined );

    public:
        VRGuiEditor(string window);

        string getCore(int i);
        void setCore(string c);
        void focus(int line, int column);
        void highlightStrings(string s, string c);
        void addKeyBinding(string name, VRUpdateCbPtr cb);
        void setLanguage(string lang);
        void grabFocus();
        void setCursor(int line, int column);
        void getCursor(int& line, int& column);
        void setSelection(string s);
        string getSelection();
        _GtkWidget* getEditor();
        _GtkSourceBuffer* getSourceBuffer();
};

OSG_END_NAMESPACE

#endif // VRGUIEDITOR_H_INCLUDED
