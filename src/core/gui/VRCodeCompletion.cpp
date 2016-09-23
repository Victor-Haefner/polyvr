#include "VRCodeCompletion.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/utils/toString.h"

#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtksourceview/gtksourceview-typebuiltins.h>
#include <gtksourceview/gtksourcecompletionitem.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <map>

using namespace std;
using GtkProvider = GtkSourceCompletionProvider;

GType vr_code_completion_get_type (void) G_GNUC_CONST;
#define VRCodeCompletionType (vr_code_completion_get_type ())
#define VRCodeCompletionCast(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), VRCodeCompletionType, VRCodeCompletion))

struct VRCodeCompletionPrivate {
	gchar* name = 0;
	GdkPixbuf* icon = 0;
	gint interactive_delay = -1;
	gint priority = 0;
	map<string, vector<string> > dict;
	bool VRMod_initiated = false;
};

void VRModDictInit(VRCodeCompletion *self) {
    auto scene = OSG::VRSceneManager::getCurrent();
    if (scene == 0) return;
    for ( auto mod : scene->getPyVRModules() ) {
        if (mod != "VR") self->priv->dict["VR"].push_back(mod);
        for ( auto t : scene->getPyVRTypes(mod) ) {
            if (t == "globals") {
                for (auto m : scene->getPyVRMethods(mod,t)) self->priv->dict[mod].push_back(m);
                continue;
            }

            self->priv->dict[mod].push_back(t);
            //for (auto m : scene->getPyVRMethods(mod,t)) self->priv->dict[mod].push_back(m);
        }
    }
    self->priv->VRMod_initiated = true;
}

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
	auto startsWith = [](string s, string sw) { return s.substr(0, sw.size()) == sw; };

    string mod = "VR";
	string word = get_word(context);
	if (word.size() <= 0 || !startsWith(word, "VR.")) { setProposals(); return; }
    auto psplit = splitString(word, '.');
    if (word[word.size()-1] == '.') psplit.push_back("");
    if (psplit.size() > 1) word = psplit[psplit.size()-1];
    if (psplit.size() > 2) mod = psplit[psplit.size()-2];

    GList* ret = NULL;
	VRCodeCompletion* data = VRCodeCompletionCast(provider);
	if (!data->priv->VRMod_initiated) VRModDictInit(data);

    if (!data->priv->dict.count(mod)) { setProposals(); return; }
    for (auto d : data->priv->dict[mod]) {
        if (startsWith(d, word)) {
            auto proposal = gtk_source_completion_item_new(d.c_str(), d.c_str(), 0, 0);
            ret = g_list_prepend (ret, proposal);
        }
    }

	setProposals( g_list_reverse(ret) );
}

static void vr_code_completion_class_init(VRCodeCompletionClass* klass) {}

gint provDelay(GtkProvider *provider) { return VRCodeCompletionCast(provider)->priv->interactive_delay; }
gint provPriority(GtkProvider *provider) { return VRCodeCompletionCast(provider)->priv->priority; }
gchar* provName(GtkProvider *self) { return g_strdup(VRCodeCompletionCast (self)->priv->name); }

void initInterface(GtkSourceCompletionProviderIface *iface) {
	iface->get_name = provName;
	iface->populate = provPopulate;
	iface->get_interactive_delay = provDelay;
	iface->get_priority = provPriority;
}

static void vr_code_completion_init(VRCodeCompletion *self) {
	self->priv = new VRCodeCompletionPrivate();
}

VRCodeCompletion* vr_code_completion_new () {
	return (VRCodeCompletion*) g_object_new (VRCodeCompletionType, NULL);
}

G_DEFINE_TYPE_WITH_CODE( VRCodeCompletion, vr_code_completion, G_TYPE_OBJECT, G_ADD_PRIVATE(VRCodeCompletion) G_IMPLEMENT_INTERFACE(GTK_TYPE_SOURCE_COMPLETION_PROVIDER, initInterface) )




