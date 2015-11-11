#include "VRCodeCompletion.h"
#include <gtksourceview/gtksourceview-typebuiltins.h>
#include <gtksourceview/gtksourcecompletionitem.h>
#include <string.h>
#include <vector>
#include <iostream>

using namespace std;

struct _VRCodeCompletionPrivate {
	gchar* name = 0;
	GdkPixbuf* icon = 0;

	gchar* word = 0;
	int word_len = 0;
	int minimum_word_size = 0;
	int cancel_id = 0;

	GtkSourceCompletionContext* context = 0;
	gint interactive_delay = -1;
	gint priority = 0;
	GtkSourceCompletionActivation activation = GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE;

	/*
    GTK_SOURCE_COMPLETION_ACTIVATION_NONE
	GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE -> allways when user enters a charachter
	GTK_SOURCE_COMPLETION_ACTIVATION_USER_REQUESTED -> only on crtl+space
	*/
};

static void vr_code_completion_iface_init (GtkSourceCompletionProviderIface *iface);

G_DEFINE_TYPE_WITH_CODE (VRCodeCompletion, vr_code_completion, G_TYPE_OBJECT,
			 G_ADD_PRIVATE (VRCodeCompletion) G_IMPLEMENT_INTERFACE (GTK_TYPE_SOURCE_COMPLETION_PROVIDER, vr_code_completion_iface_init))

static gchar* vr_code_completion_get_name (GtkSourceCompletionProvider *self) {
	return g_strdup (GTK_SOURCE_COMPLETION_WORDS (self)->priv->name);
}

static GdkPixbuf* vr_code_completion_get_icon (GtkSourceCompletionProvider *self) {
	return GTK_SOURCE_COMPLETION_WORDS (self)->priv->icon;
}

static gboolean valid_word_char(gunichar ch) {
	return g_unichar_isprint (ch) && (ch == '_' || g_unichar_isalnum (ch));
}

static gboolean valid_start_char(gunichar ch) {
	return !g_unichar_isdigit (ch);
}

gchar* get_end_word(gchar* text) {
	gchar *cur_char = text + strlen (text);
	gboolean word_found = FALSE;
	gunichar ch;

	while(TRUE) {
		gchar *prev_char = g_utf8_find_prev_char (text, cur_char);
		if (prev_char == NULL) break;
		ch = g_utf8_get_char (prev_char);
		if (!valid_word_char(ch)) break;
		word_found = TRUE;
		cur_char = prev_char;
	}

	if (!word_found) return NULL;
	ch = g_utf8_get_char (cur_char);
	if (!valid_start_char (ch)) return NULL;
	return g_strdup (cur_char);
}

static gchar* get_word_at_iter(GtkTextIter *iter) {
	GtkTextBuffer *buffer;
	GtkTextIter start_line;
	gchar *line_text;
	gchar *word;

	buffer = gtk_text_iter_get_buffer (iter);
	start_line = *iter;
	gtk_text_iter_set_line_offset (&start_line, 0);

	line_text = gtk_text_buffer_get_text (buffer, &start_line, iter, FALSE);

	word = get_end_word (line_text);

	g_free (line_text);
	return word;
}

static void vr_code_completion_populate (GtkSourceCompletionProvider* provider, GtkSourceCompletionContext* context) {
	VRCodeCompletion* words = GTK_SOURCE_COMPLETION_WORDS(provider);
	GtkSourceCompletionActivation activation;
	GtkTextIter iter;
	gchar* word;

    gtk_source_completion_context_get_iter(context, &iter);
	if (gtk_text_iter_is_end(&iter)) { gtk_source_completion_context_add_proposals(context, provider, NULL, TRUE); return; }

	g_free(words->priv->word);
	words->priv->word = NULL;
	word = get_word_at_iter(&iter);

	if (word == NULL || (g_utf8_strlen(word, -1) < words->priv->minimum_word_size)) {
		g_free(word);
		gtk_source_completion_context_add_proposals(context, provider, NULL, TRUE);
		return;
	}

	words->priv->context = context;
	words->priv->word = word;
	words->priv->word_len = strlen(word);

	vector<string> dict;
	dict.push_back("apple");
	dict.push_back("tree");
	dict.push_back("pencil");

	string w(word);
    GList* ret = NULL;
	for (auto d : dict) {
        if (d.substr(0, w.size()) == w) {
            auto proposal = gtk_source_completion_item_new(d.c_str(), d.c_str(), 0, 0);
            ret = g_list_prepend (ret, proposal);
        }
	}

	//ret = g_list_reverse (ret);
	gtk_source_completion_context_add_proposals(context, provider, ret, FALSE);
	gtk_source_completion_context_add_proposals(context, provider, NULL, TRUE);
}

static void vr_code_completion_class_init(VRCodeCompletionClass* klass) { }

static gint vr_code_completion_get_interactive_delay (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS(provider)->priv->interactive_delay;
}

static gint vr_code_completion_get_priority (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS(provider)->priv->priority;
}

static GtkSourceCompletionActivation vr_code_completion_get_activation (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS(provider)->priv->activation;
}

static void vr_code_completion_iface_init (GtkSourceCompletionProviderIface *iface) {
	iface->get_name = vr_code_completion_get_name;
	iface->get_icon = vr_code_completion_get_icon;
	iface->populate = vr_code_completion_populate;
	iface->get_interactive_delay = vr_code_completion_get_interactive_delay;
	iface->get_priority = vr_code_completion_get_priority;
	iface->get_activation = vr_code_completion_get_activation;
}

static void vr_code_completion_init(VRCodeCompletion *self) {
	self->priv = new VRCodeCompletionPrivate();
}

VRCodeCompletion* vr_code_completion_new () {
	return (VRCodeCompletion*) g_object_new (GTK_TYPE_SOURCE_COMPLETION_WORDS, NULL);
}

