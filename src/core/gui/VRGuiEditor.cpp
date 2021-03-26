#include <gtk/gtk.h>
#include "VRGuiEditor.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"
#include "VRGuiCodeCompletion.h"
#include "core/scene/VRSceneManager.h"
#include "core/scripting/VRScript.h"
#include "core/utils/VRFunction.h"

#include <iostream>

#if GTK_MAJOR_VERSION == 2
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcecompletionprovider.h>
#else
#include "gtksourceview/gtksource.h"
#endif

OSG_BEGIN_NAMESPACE;
using namespace std;

string padding = "\n\n\n\n\n\n\n\n\n";

void VRGuiEditor::setCore(string core) {
    gtk_source_buffer_begin_not_undoable_action(sourceBuffer);
    string data = core+padding;
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(sourceBuffer), data.c_str(), data.size());
    gtk_source_buffer_end_not_undoable_action(sourceBuffer);
}

string VRGuiEditor::getCore(int i) {
    GtkTextIter itr_s, itr_e;
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(sourceBuffer), &itr_s);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(sourceBuffer), &itr_e);
    for (int j=0; j<i; j++) gtk_text_iter_forward_line(&itr_s); // skip head
    string data = string( gtk_text_buffer_get_text( GTK_TEXT_BUFFER(sourceBuffer), &itr_s, &itr_e, true) );
    while(data.back() == '\n') data.pop_back();
    return data;
}

void VRGuiEditor::focus(int line, int column) {
    grabFocus();
    setCursor(line, column);
}

void VRGuiEditor::setCursor(int line, int column) {
    // get iterator at line and column and set cursor to iterator
    GtkTextIter itr;
    GtkTextBuffer* buffer = gtk_text_view_get_buffer((GtkTextView*)editor);
    gtk_text_buffer_get_iter_at_line(buffer, &itr, line-1);
    gtk_text_iter_forward_chars(&itr, max(column-1, 0));
    gtk_text_buffer_place_cursor(buffer, &itr);

    // scroll to line
    gtk_text_iter_set_line_offset(&itr, 0);
    GtkTextMark* mark = gtk_text_buffer_create_mark(buffer, 0, &itr, false);
    gtk_text_view_scroll_to_mark((GtkTextView*)editor, mark, 0.25, false, 0, 0);
    cout << " VRGuiEditor::setCursor l: " << line << " c: " << column << " m: " << gtk_text_iter_get_line(&itr) << endl;
}

void VRGuiEditor::getCursor(int& line, int& column) {
    GtkTextIter itr;
    GtkTextBuffer* buffer = gtk_text_view_get_buffer((GtkTextView*)editor);
    gtk_text_buffer_get_iter_at_mark(buffer, &itr, gtk_text_buffer_get_insert(buffer));
    line = gtk_text_iter_get_line(&itr);
    column = gtk_text_iter_get_line_offset(&itr);
}

void VRGuiEditor::printViewerLanguages() {
	cout << "VRGuiEditor::printViewerLanguages" << endl;
    GtkSourceLanguageManager* langMgr = gtk_source_language_manager_get_default();
	cout << " langMgr: " << langMgr << endl;
    const gchar* const* ids = gtk_source_language_manager_get_language_ids(langMgr);
	cout << "  ids: " << ids << endl;
	if (ids) {
		for (auto id = ids; *id != NULL; ++id)
			if (ids != NULL) cout << "  LID " << *id << endl;
	} else cout << "  WARNING! no languages found for source highlighting!" << endl;
	cout << " VRGuiEditor::printViewerLanguages done" << endl;
}

bool VRGuiEditor::on_editor_shortkey( GdkEventKey* e ) {
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
}

string VRGuiEditor::getSelection() { return selection; }

void VRGuiEditor::addStyle( string style, string fg, string bg, bool italic, bool bold, bool underlined ) {
    GtkTextTag* tag = gtk_text_buffer_create_tag(editorBuffer, NULL, NULL);
    g_object_set(tag, "foreground", fg.c_str(), NULL);
    g_object_set(tag, "background", bg.c_str(), NULL);

    if (underlined) g_object_set(tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
    if (italic) g_object_set(tag, "style", PANGO_STYLE_ITALIC, NULL);
    if (bold) g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);

    editorStyles[style] = tag;
    styleStates[style] = false;
}

void VRGuiEditor::grabFocus() {
    gtk_widget_grab_focus(editor);
}

void VRGuiEditor::addKeyBinding(string name, VRUpdateCbPtr cb) { keyBindings[name] = cb; }

void VRGuiEditor::highlightStrings(string search, string style) {
    auto tag = editorStyles[style];
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
    }
}

void VRGuiEditor::setSelection(string s) {
    selection = s;
    highlightStrings(selection, "asSelected");
}

bool VRGuiEditor_on_editor_select(GtkWidget* widget, GdkEvent* event, VRGuiEditor* self) {
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
}

void VRGuiEditor::setLanguage(string lang) {
    if (lang == "Python") gtk_source_buffer_set_language(sourceBuffer, python);
    if (lang == "GLSL") gtk_source_buffer_set_language(sourceBuffer, glsl);
    if (lang == "HTML") gtk_source_buffer_set_language(sourceBuffer, web);
}

_GtkSourceBuffer* VRGuiEditor::getSourceBuffer() { return sourceBuffer; }
_GtkWidget* VRGuiEditor::getEditor() { return editor; }

VRGuiEditor::VRGuiEditor(string window) {
    // init source view editor
    GtkSourceLanguageManager* langMgr = gtk_source_language_manager_get_default();
    static bool onInitLangMgr = true;
    string langPath = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/gui/gtksv";
    char** new_langs = g_new(char*, 2);
    new_langs[0] = (char*)langPath.c_str();
    new_langs[1] = NULL;
    if (onInitLangMgr) {
        gtk_source_language_manager_set_search_path (langMgr, new_langs);
        onInitLangMgr = false;
    }

    if (!python) python = gtk_source_language_manager_get_language(langMgr, "python");
    if (!glsl) glsl = gtk_source_language_manager_get_language(langMgr, "glsl");
    if (!web) web = gtk_source_language_manager_get_language(langMgr, "html");

	cout << "VRGuiEditor::VRGuiEditor langs: " << python << " " << glsl << " " << web << endl;
	//printViewerLanguages();
	if (!python) sourceBuffer = gtk_source_buffer_new(0);
    else sourceBuffer = gtk_source_buffer_new_with_language(python);
    GtkSourceStyleSchemeManager* manager = gtk_source_style_scheme_manager_get_default();
    gtk_source_style_scheme_manager_set_search_path(manager, new_langs);
    GtkSourceStyleScheme* scheme = gtk_source_style_scheme_manager_get_scheme(manager, "classic");
    gtk_source_buffer_set_style_scheme(sourceBuffer, scheme);
    gtk_source_buffer_set_highlight_syntax(sourceBuffer, true);
    gtk_source_buffer_set_highlight_matching_brackets(sourceBuffer, true);

    GtkScrolledWindow* win = (GtkScrolledWindow*)VRGuiBuilder::get()->get_widget(window);
    editor = gtk_source_view_new_with_buffer(sourceBuffer);
    editorBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));

    gtk_text_view_set_editable(GTK_TEXT_VIEW(editor), true);
    //gtk_widget_add_events((GtkWidget*)win, (GdkEventMask)GDK_KEY_PRESS_MASK);
    //gtk_widget_add_events((GtkWidget*)win, (GdkEventMask)GDK_KEY_RELEASE_MASK);
    //gtk_widget_set_can_focus((GtkWidget*)win, true);

    gtk_container_add(GTK_CONTAINER(win), editor);

    // buffer changed callback
    function<void(GdkEventKey*)> sig = bind(&VRGuiEditor::on_editor_shortkey, this, placeholders::_1);
    connect_signal((GtkWidget*)win, sig, "key_press_event");

    // editor signals
    g_signal_connect_after(editor, "event", G_CALLBACK(VRGuiEditor_on_editor_select), this );

    // editor options
    gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (editor), 4);
    gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_indent_width (GTK_SOURCE_VIEW (editor), 4);
    gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (editor), TRUE);
    gtk_source_view_set_right_margin_position (GTK_SOURCE_VIEW (editor), 80); // default is 70 chars
    gtk_source_view_set_show_right_margin (GTK_SOURCE_VIEW (editor), FALSE);

    // editor font
    PangoFontDescription *font_desc = pango_font_description_new();
    pango_font_description_set_family (font_desc, "monospace");
    gtk_widget_modify_font (editor, font_desc);
    gtk_widget_show_all(editor);

#ifndef _WIN32 // TODO: loading ressources/gui/gtksourcecompletion.ui fails on windows
    cout << "VRGuiEditor::VRGuiEditor, enable code completion" << endl;
    auto provider = VRGuiCodeCompletionNew();
    auto completion = gtk_source_view_get_completion(GTK_SOURCE_VIEW(editor));
    GError* error = 0;
    gtk_source_completion_add_provider(completion, GTK_SOURCE_COMPLETION_PROVIDER(provider), &error);
    if (error) {
        cout << "  -- source view completion error: " << error->message << endl;
        g_clear_error(&error);
        g_error_free(error);
    }
#endif

    addStyle( "asSelected", "#000", "#FF0", false, false, false);
	cout << " VRGuiEditor::VRGuiEditor done" << endl;
}

OSG_END_NAMESPACE;
