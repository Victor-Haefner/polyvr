#include "VRGuiEditor.h"
#include "VRGuiManager.h"
#include "core/scene/VRSceneManager.h"
#include "core/scripting/VRScript.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"

#include <iostream>


OSG_BEGIN_NAMESPACE;
using namespace std;

string padding = "\n\n\n\n\n\n\n\n\n";

void VRGuiEditor::setCore(string core, int i) {
    buffer = core+padding;
    headerLines = i;
    uiSignal("script_editor_set_buffer", {{"data", buffer}});
}

string VRGuiEditor::getCore() {
    uiSignal("script_editor_request_buffer", {{"skipLines", toString(headerLines)}});
    // onCoreUpdate will be called and buffer updated
    return buffer;
}

void VRGuiEditor::onCoreUpdate(string& data) {
    buffer = data;
    while(buffer.size() > 0 && buffer.back() == '\n') buffer.pop_back();
}

void VRGuiEditor::focus(int line, int column) {
    grabFocus();
    setCursorPosition(line, column);
}

void VRGuiEditor::setCursorPosition(int line, int column) {
    if (line <= 0) line = 1;
    if (column <= 0) column = 1;
    uiSignal("script_editor_set_cursor", {{"line", toString(line)}, {"column", toString(column)}});
}

int* tmpLine = 0;
int* tmpColumn = 0;

void VRGuiEditor::onCursorUpdate(int l, int c) {
    if (tmpLine) *tmpLine = l;
    if (tmpColumn) *tmpColumn = c;
}

void VRGuiEditor::getCursorPosition(int& line, int& column) {
    tmpLine = &line;
    tmpColumn = &column;
    uiSignal("script_editor_request_cursor", {});
    tmpLine = 0;
    tmpColumn = 0;
}

void VRGuiEditor::printViewerLanguages() {
	/*cout << "VRGuiEditor::printViewerLanguages" << endl;
    GtkSourceLanguageManager* langMgr = gtk_source_language_manager_get_default();
	cout << " langMgr: " << langMgr << endl;
    const gchar* const* ids = gtk_source_language_manager_get_language_ids(langMgr);
	cout << "  ids: " << ids << endl;
	if (ids) {
		for (auto id = ids; *id != NULL; ++id)
			if (ids != NULL) cout << "  LID " << *id << endl;
	} else cout << "  WARNING! no languages found for source highlighting!" << endl;
	cout << " VRGuiEditor::printViewerLanguages done" << endl;*/
}

/*bool VRGuiEditor::on_editor_shortkey( GdkEventKey* e ) {
    //cout << "VRGuiEditor::on_editor_shortkey " << e->keyval << endl;
    if ( !(e->state & GDK_CONTROL_MASK) ) return false;

    auto getCurrentLine = [&]() {
        GtkTextIter itr;
        auto b = GTK_TEXT_BUFFER(sourceBuffer);
        auto m = gtk_text_buffer_get_insert(b);
        gtk_text_buffer_get_iter_at_mark( b, &itr, m);
        return gtk_text_iter_get_line(&itr);
        //return itr;
    };

    auto getLine = [&](int l) -> string {
        auto b = GTK_TEXT_BUFFER(sourceBuffer);
        GtkTextIter itr1;
        gtk_text_buffer_get_iter_at_line_index(b, &itr1, l, 0);
        GtkTextIter itr2 = itr1;
        gtk_text_iter_forward_to_line_end(&itr2);
        string data = gtk_text_buffer_get_slice(b, &itr1, &itr2, true);
        if (data[0] == '\n') return "";
        return data;
    };

    auto insertLineAfter = [&](string line, int l) {
        auto b = GTK_TEXT_BUFFER(sourceBuffer);
        GtkTextIter itr;
        gtk_text_buffer_get_iter_at_line_index(b, &itr, l, 0);
        int l2 = gtk_text_iter_get_line(&itr);
        if (l2 < l) {
            gtk_text_buffer_get_end_iter(b, &itr);
            line = "\n"+line;
        } else {
            line = line+"\n";
        }
        gtk_source_buffer_begin_not_undoable_action(sourceBuffer);
        gtk_text_buffer_insert(b, &itr, line.c_str(), line.length());
        gtk_source_buffer_end_not_undoable_action(sourceBuffer);
    };

    auto eraseLine = [&](int l) {
        string line = getLine(l);
        auto b = GTK_TEXT_BUFFER(sourceBuffer);
        GtkTextIter itr1;
        gtk_text_buffer_get_iter_at_line_index(b, &itr1, l, 0);
        GtkTextIter itr2 = itr1;
        if (line != "") gtk_text_iter_forward_to_line_end(&itr2);
        gtk_text_iter_forward_char(&itr2);
        gtk_source_buffer_begin_not_undoable_action(sourceBuffer);
        gtk_text_buffer_delete(b, &itr1, &itr2);
        gtk_source_buffer_end_not_undoable_action(sourceBuffer);
    };

    auto triggerCB = [&](string name) {
        //cout << "VRGuiEditor::on_editor_shortkey trigger " << name << endl;
        if (!keyBindings.count(name)) return;
        if (!keyBindings[name]) return;
        (*keyBindings[name])();
        //cout << " VRGuiEditor::on_editor_shortkey trigger " << name << " done" << endl;
    };

    if (e->keyval == 102) {// f
        triggerCB("find");
        return true;
    }

    if (e->keyval == 119) {// w
        triggerCB("wipe");
        return true;
    }

    if (e->keyval == 115) {// s
        triggerCB("save");
        return true;
    }

    if (e->keyval == 101) {// e
        triggerCB("exec");
        return true;
    }

    if (e->keyval == 104) {// h
        triggerCB("help");
        return true;
    }

    if (e->keyval == 100) {// d
        auto l = getCurrentLine();
        string line = getLine(l);
        insertLineAfter(line, l);
        return true;
    }

    if (e->keyval == 116) {// t
        auto l = getCurrentLine();
        if (l <= 1) return true;
        string line = getLine(l-1);
        insertLineAfter(line, l+1);
        eraseLine(l-1);
        return true;
    }

    return false;
}*/

string VRGuiEditor::getSelection() { return selection; }

void VRGuiEditor::addStyle( string style, string fg, string bg, bool italic, bool bold, bool underlined ) {
    /*GtkTextTag* tag = gtk_text_buffer_create_tag(editorBuffer, NULL, NULL);
    g_object_set(tag, "foreground", fg.c_str(), NULL);
    g_object_set(tag, "background", bg.c_str(), NULL);

    if (underlined) g_object_set(tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
    if (italic) g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
    if (bold) g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);

    editorStyles[style] = tag;
    styleStates[style] = false;*/
}

void VRGuiEditor::grabFocus() {
    //gtk_widget_grab_focus(editor);
}

void VRGuiEditor::addKeyBinding(string name, VRUpdateCbPtr cb) { keyBindings[name] = cb; }

void VRGuiEditor::highlightStrings(string search, string style) {
    uiSignal("script_editor_highlight", {{"string", search}, {"style", style}});
    /*auto tag = editorStyles[style];
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(editorBuffer, &start, &end);
    gtk_text_buffer_remove_tag(editorBuffer, tag, &start, &end);
    if (search == "") return;

    auto S = VRScript::Search();
    map<int, bool> res;

    string core = getCore(1);
    unsigned int pos = core.find(search, 0);
    while(pos != string::npos && pos <= core.size()) { res[pos] = false; pos = core.find(search, pos+1); }
    pos = core.find("\n", 0);
    while(pos != string::npos && pos <= core.size()) { res[pos] = true; pos = core.find("\n", pos+1); }

    int l = 2;
    int lpo = 0;
    for (auto r : res) {
        if (r.second) { l++; lpo = r.first; continue; } // new line
        if (S.result.count(l) == 0) S.result[l] = vector<int>();
        S.result[l].push_back(r.first - lpo);
    }

    GtkTextIter SB, SE, A, B;
    gtk_text_buffer_get_selection_bounds(editorBuffer, &SB, &SE);

    for (auto line : S.result) {
        for (auto column : line.second) {
            if (line.first == 2) column++; // strange hack..
            gtk_text_buffer_get_iter_at_line(editorBuffer, &A, line.first-1);
            gtk_text_buffer_get_iter_at_line(editorBuffer, &B, line.first-1);
            gtk_text_iter_forward_chars(&A, max(column-1, 0));
            gtk_text_iter_forward_chars(&B, column-1+search.size());
            if (gtk_text_iter_compare(&A, &B) == 0) continue;
            gtk_text_buffer_apply_tag(editorBuffer, tag, &A, &B);
            styleStates[style] = true;
        }
    }*/
}

void VRGuiEditor::setSelection(string s) {
    selection = s;
    highlightStrings(selection, "asSelected");
}

/*bool VRGuiEditor_on_editor_select(GtkWidget* widget, GdkEvent* event, VRGuiEditor* self) {
    GdkEventButton* event_btn = (GdkEventButton*)event;

    if (event->type == GDK_BUTTON_RELEASE && event_btn->button == 1) {
        auto editor = GTK_TEXT_VIEW(widget);
        auto buffer = gtk_text_view_get_buffer(editor);

        GtkTextIter A, B;
        gchar* selection = 0;
        if ( gtk_text_buffer_get_selection_bounds(buffer, &A, &B) ) {
            selection = gtk_text_buffer_get_text(buffer, &A, &B, true);
        }
        self->setSelection(selection?selection:"");
        return false;
    }

    if (event->type == GDK_KEY_RELEASE || event->type == GDK_BUTTON_RELEASE) { // remove selection on any key or button
        self->setSelection("");
        return false;
    }

    return false;
}*/

void VRGuiEditor::setLanguage(string lang) {
    uiSignal("script_editor_set_lang", {{"lang", lang}});
    /*if (lang == "Python") gtk_source_buffer_set_language(sourceBuffer, python);
    if (lang == "GLSL") gtk_source_buffer_set_language(sourceBuffer, glsl);
    if (lang == "HTML") gtk_source_buffer_set_language(sourceBuffer, web);*/
}

//_GtkSourceBuffer* VRGuiEditor::getSourceBuffer() { return sourceBuffer; }
//_GtkWidget* VRGuiEditor::getEditor() { return editor; }

VRGuiEditor::VRGuiEditor(string window) {
    auto mgr = OSG::VRGuiSignals::get();
    mgr->addCallback("script_editor_transmit_core", [&](OSG::VRGuiSignals::Options o) { onCoreUpdate(o["core"]); return true; } );
    mgr->addCallback("script_editor_transmit_cursor", [&](OSG::VRGuiSignals::Options o) { onCursorUpdate(toInt(o["line"]), toInt(o["column"])); return true; } );
	cout << " VRGuiEditor::VRGuiEditor done" << endl;
}

OSG_END_NAMESPACE;
