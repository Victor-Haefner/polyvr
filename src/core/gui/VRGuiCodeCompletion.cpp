#include <gtk/gtk.h>
#include "VRGuiCodeCompletion.h"
#include "VRGuiManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"
#include "core/scripting/VRPyCodeCompletion.h"
#include "core/scripting/VRScriptFwd.h"

#if GTK_MAJOR_VERSION == 2
#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtksourceview/gtksourceview-typebuiltins.h>
#include <gtksourceview/gtksourcecompletionitem.h>
#else
#include "gtksourceview/gtksource.h"
#endif

#include <string.h>
#include <vector>
#include <iostream>
#include <map>

using namespace OSG;
using GtkProvider = GtkSourceCompletionProvider;

GType vr_code_completion_get_type (void) G_GNUC_CONST;
#define VRGuiCodeCompletionType (vr_code_completion_get_type ())
#define VRGuiCodeCompletionCast(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), VRGuiCodeCompletionType, VRGuiCodeCompletion))

struct VRGuiCodeCompletionPrivate {
	gchar* name = 0;
	GdkPixbuf* icon = 0;
	gint interactive_delay = -1;
	gint priority = 0;
    VRPyCodeCompletion pyCompletion;
};

string get_end_word(gchar* text) {
	gchar* cur_char = text + strlen(text);
	gboolean word_found = FALSE;
	gunichar ch;

	auto validChar = [](gunichar ch) {
        return g_unichar_isprint(ch) && (ch == '.' || ch == '_' || g_unichar_isalnum(ch));
    };

	while(TRUE) {
		gchar *prev_char = g_utf8_find_prev_char (text, cur_char);
		if (prev_char == NULL) break;
		ch = g_utf8_get_char (prev_char);
		if (!validChar(ch)) break;
		word_found = TRUE;
		cur_char = prev_char;
	}

	if (!word_found) return "";
	ch = g_utf8_get_char (cur_char);
	if (g_unichar_isdigit (ch)) return "";
	return string( g_strdup (cur_char) );
}

string get_word(GtkSourceCompletionContext* context) {
	GtkTextIter iter, iter2;
    gtk_source_completion_context_get_iter(context, &iter);
    iter2 = iter;
	gtk_text_iter_set_line_offset(&iter2, 0);

	GtkTextBuffer* buffer = gtk_text_iter_get_buffer(&iter);
	gchar* line_text = gtk_text_buffer_get_text(buffer, &iter2, &iter, FALSE);

	string word = string( get_end_word(line_text) );
	g_free(line_text);
	return word;
}

void provPopulate(GtkProvider* provider, GtkSourceCompletionContext* context) {
	auto setProposals = [&](GList* ret = NULL) { gtk_source_completion_context_add_proposals(context, provider, ret, TRUE); };
	//auto startsWith = [](string s, string sw) { return s.substr(0, sw.size()) == sw; };

	VRGuiCodeCompletion* data = VRGuiCodeCompletionCast(provider);
	string word = get_word(context);
	vector<string> suggestions = data->priv->pyCompletion.getSuggestions(word);

	/*VRScriptPtr script;
	int line;
	int column;
	VRGuiManager::get()->getScriptFocus(script, line, column);
	vector<string> suggestions = data->priv->pyCompletion.getJediSuggestions(script,line,column);*/
	if (suggestions.size() == 0) { setProposals(); return; }

    GList* ret = NULL;
    for (auto d : suggestions) {
        auto proposal = gtk_source_completion_item_new(d.c_str(), d.c_str(), 0, 0);
        ret = g_list_prepend (ret, proposal);
    }
	setProposals( g_list_reverse(ret) );
}

static void vr_code_completion_class_init(VRGuiCodeCompletionClass* klass) {}

gint provDelay(GtkProvider *provider) { return VRGuiCodeCompletionCast(provider)->priv->interactive_delay; }
gint provPriority(GtkProvider *provider) { return VRGuiCodeCompletionCast(provider)->priv->priority; }
gchar* provName(GtkProvider *self) { return g_strdup(VRGuiCodeCompletionCast (self)->priv->name); }

void initInterface(GtkSourceCompletionProviderIface *iface) {
	iface->get_name = provName;
	iface->populate = provPopulate;
	iface->get_interactive_delay = provDelay;
	iface->get_priority = provPriority;
}

static void vr_code_completion_init(VRGuiCodeCompletion *self) {
	self->priv = new VRGuiCodeCompletionPrivate();
}

VRGuiCodeCompletion* VRGuiCodeCompletionNew () {
	return (VRGuiCodeCompletion*) g_object_new (VRGuiCodeCompletionType, NULL);
}

#if GTK_MAJOR_VERSION == 2
G_DEFINE_TYPE_WITH_CODE( VRGuiCodeCompletion, vr_code_completion, G_TYPE_OBJECT, G_ADD_PRIVATE(VRGuiCodeCompletion) G_IMPLEMENT_INTERFACE(GTK_TYPE_SOURCE_COMPLETION_PROVIDER, initInterface) )
#else
//G_DEFINE_TYPE_WITH_CODE(VRGuiCodeCompletion, vr_code_completion, G_TYPE_OBJECT, G_ADD_PRIVATE(VRGuiCodeCompletion) G_IMPLEMENT_INTERFACE(vr_code_completion_get_type(), initInterface))
G_DEFINE_TYPE_WITH_CODE( VRGuiCodeCompletion, vr_code_completion, G_TYPE_OBJECT, G_ADD_PRIVATE(VRGuiCodeCompletion) G_IMPLEMENT_INTERFACE(GTK_SOURCE_TYPE_COMPLETION_PROVIDER, initInterface) )
#endif




