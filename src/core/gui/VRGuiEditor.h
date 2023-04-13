#ifndef VRGUIEDITOR_H_INCLUDED
#define VRGUIEDITOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFunctionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiEditor {
    private:
        /*_GtkWidget* editor = 0;
        _GtkSourceBuffer* sourceBuffer = 0;
        _GtkSourceLanguage* python = 0;
        _GtkSourceLanguage* web = 0;
        _GtkSourceLanguage* glsl = 0;
        _GtkTextBuffer* editorBuffer = 0;*/
        string buffer;
        int headerLines = 0;
	    map<string, VRUpdateCbPtr> keyBindings;

	    string selection;

        //map<string, _GtkTextTag*> editorStyles;
        map<string, bool> styleStates;

        void onCoreUpdate(string& data);

        void printViewerLanguages();
        //bool on_editor_shortkey( _GdkEventKey* e );
        void addStyle( string style, string fg, string bg, bool italiq, bool bold, bool underlined );

    public:
        VRGuiEditor(string window);

        string getCore();
        void setCore(string c, int i);
        void focus(int line, int column);
        void highlightStrings(string s, string c);
        void addKeyBinding(string name, VRUpdateCbPtr cb);
        void setLanguage(string lang);
        void grabFocus();
        void setCursorPosition(int line, int column);
        void getCursorPosition(int& line, int& column);
        void setSelection(string s);
        string getSelection();
        //_GtkWidget* getEditor();
        //_GtkSourceBuffer* getSourceBuffer();
};

OSG_END_NAMESPACE

#endif // VRGUIEDITOR_H_INCLUDED
