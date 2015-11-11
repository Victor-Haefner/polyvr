#include "VRCodeCompletion.h"
#include <gtksourceview/gtksourceview-typebuiltins.h>
#include <string.h>

struct _VRCodeCompletionPrivate {
	gchar* name = 0;
	GdkPixbuf* icon = 0;

	GtkSourceCompletionContext* context = 0;
	gint interactive_delay = 0;
	gint priority = 0;
	GtkSourceCompletionActivation activation;
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

static void population_finished (VRCodeCompletion *words) {
	/*if (words->priv->idle_id != 0) {
		g_source_remove (words->priv->idle_id);
		words->priv->idle_id = 0;
	}

	g_free (words->priv->word);
	words->priv->word = NULL;

	if (words->priv->context != NULL) {
		if (words->priv->cancel_id) {
			g_signal_handler_disconnect (words->priv->context, words->priv->cancel_id);
			words->priv->cancel_id = 0;
		}

		g_clear_object (&words->priv->context);
	}*/
}

static gboolean add_in_idle (VRCodeCompletion *words) {
	guint idx = 0;
	GList *ret = NULL;
	gboolean finished = FALSE;

	//if (words->priv->populate_iter == NULL) {}

	/*while (idx < words->priv->proposals_batch_size && words->priv->populate_iter) {
		VRCodeCompletionProposal *proposal = vr_code_completion_library_get_proposal (words->priv->populate_iter);

		// Only add non-exact matches
		if (strcmp (vr_code_completion_proposal_get_word (proposal), words->priv->word) != 0) {
			ret = g_list_prepend (ret, proposal);
		}

		words->priv->populate_iter = vr_code_completion_library_find_next (words->priv->populate_iter, words->priv->word, words->priv->word_len);
		++idx;
	}

	ret = g_list_reverse (ret);
	finished = words->priv->populate_iter == NULL;

	gtk_source_completion_context_add_proposals (words->priv->context,
	                                             GTK_SOURCE_COMPLETION_PROVIDER (words),
	                                             ret,
	                                             finished);

	g_list_free (ret);

	if (finished) {
		//vr_code_completion_library_unlock (words->priv->library);
		population_finished (words);
	}*/

	return !finished;
}

static gchar* get_word_at_iter (GtkTextIter *iter) {
	GtkTextBuffer *buffer;
	GtkTextIter start_line;
	gchar *line_text;
	gchar *word;

	buffer = gtk_text_iter_get_buffer (iter);
	start_line = *iter;
	gtk_text_iter_set_line_offset (&start_line, 0);

	line_text = gtk_text_buffer_get_text (buffer, &start_line, iter, FALSE);

	//word = _vr_code_completion_utils_get_end_word (line_text);

	g_free (line_text);
	return word;
}

static void
vr_code_completion_populate (GtkSourceCompletionProvider *provider,
                                      GtkSourceCompletionContext  *context)
{
	VRCodeCompletion *words = GTK_SOURCE_COMPLETION_WORDS (provider);
	GtkSourceCompletionActivation activation;
	GtkTextIter iter;
	gchar *word;

	/*if (!gtk_source_completion_context_get_iter (context, &iter))
	{
		gtk_source_completion_context_add_proposals (context, provider, NULL, TRUE);
		return;
	}

	g_free (words->priv->word);
	words->priv->word = NULL;

	word = get_word_at_iter (&iter);

	activation = gtk_source_completion_context_get_activation (context);

	if (word == NULL ||
	    (activation == GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE &&
	     g_utf8_strlen (word, -1) < words->priv->minimum_word_size))
	{
		g_free (word);
		gtk_source_completion_context_add_proposals (context, provider, NULL, TRUE);
		return;
	}

	words->priv->cancel_id =
		g_signal_connect_swapped (context,
			                  "cancelled",
			                  G_CALLBACK (population_finished),
			                  provider);

	words->priv->context = g_object_ref (context);

	words->priv->word = word;
	words->priv->word_len = strlen (word);

	// Do first right now
	if (add_in_idle (words))
	{
		vr_code_completion_library_lock (words->priv->library);
		words->priv->idle_id = gdk_threads_add_idle ((GSourceFunc)add_in_idle,
		                                             words);
	}*/
}

static void
vr_code_completion_dispose (GObject *object)
{
	/*VRCodeCompletion *provider = GTK_SOURCE_COMPLETION_WORDS (object);

	population_finished (provider);

	while (provider->priv->buffers != NULL)
	{
		BufferBinding *binding = provider->priv->buffers->data;
		GtkTextBuffer *buffer = vr_code_completion_buffer_get_buffer (binding->buffer);

		vr_code_completion_unregister (provider, buffer);
	}

	g_free (provider->priv->name);
	provider->priv->name = NULL;

	g_clear_object (&provider->priv->icon);
	g_clear_object (&provider->priv->library);

	G_OBJECT_CLASS (vr_code_completion_parent_class)->dispose (object);*/
}

static void
update_buffers_batch_size (VRCodeCompletion *words)
{
	/*GList *item;

	for (item = words->priv->buffers; item != NULL; item = g_list_next (item))
	{
		BufferBinding *binding = item->data;
		vr_code_completion_buffer_set_scan_batch_size (binding->buffer,
		                                                        words->priv->scan_batch_size);
	}*/
}

static void
update_buffers_minimum_word_size (VRCodeCompletion *words)
{
	/*GList *item;

	for (item = words->priv->buffers; item != NULL; item = g_list_next (item))
	{
		BufferBinding *binding = (BufferBinding *)item->data;
		vr_code_completion_buffer_set_minimum_word_size (binding->buffer,
		                                                          words->priv->minimum_word_size);
	}*/
}

static void
vr_code_completion_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
	/*VRCodeCompletion *self = GTK_SOURCE_COMPLETION_WORDS (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_free (self->priv->name);
			self->priv->name = g_value_dup_string (value);

			if (self->priv->name == NULL)
			{
				self->priv->name = g_strdup (_("Document Words"));
			}
			break;

		case PROP_ICON:
			g_clear_object (&self->priv->icon);
			self->priv->icon = g_value_dup_object (value);
			break;

		case PROP_PROPOSALS_BATCH_SIZE:
			self->priv->proposals_batch_size = g_value_get_uint (value);
			break;

		case PROP_SCAN_BATCH_SIZE:
			self->priv->scan_batch_size = g_value_get_uint (value);
			update_buffers_batch_size (self);
			break;

		case PROP_MINIMUM_WORD_SIZE:
			self->priv->minimum_word_size = g_value_get_uint (value);
			update_buffers_minimum_word_size (self);
			break;

		case PROP_INTERACTIVE_DELAY:
			self->priv->interactive_delay = g_value_get_int (value);
			break;

		case PROP_PRIORITY:
			self->priv->priority = g_value_get_int (value);
			break;

		case PROP_ACTIVATION:
			self->priv->activation = g_value_get_flags (value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}*/
}

static void
vr_code_completion_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
	/*VRCodeCompletion *self = GTK_SOURCE_COMPLETION_WORDS (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_value_set_string (value, self->priv->name);
			break;

		case PROP_ICON:
			g_value_set_object (value, self->priv->icon);
			break;

		case PROP_PROPOSALS_BATCH_SIZE:
			g_value_set_uint (value, self->priv->proposals_batch_size);
			break;

		case PROP_SCAN_BATCH_SIZE:
			g_value_set_uint (value, self->priv->scan_batch_size);
			break;

		case PROP_MINIMUM_WORD_SIZE:
			g_value_set_uint (value, self->priv->minimum_word_size);
			break;

		case PROP_INTERACTIVE_DELAY:
			g_value_set_int (value, self->priv->interactive_delay);
			break;

		case PROP_PRIORITY:
			g_value_set_int (value, self->priv->priority);
			break;

		case PROP_ACTIVATION:
			g_value_set_flags (value, self->priv->activation);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}*/
}

static void
vr_code_completion_class_init (VRCodeCompletionClass *klass)
{
	/*GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = vr_code_completion_dispose;

	object_class->set_property = vr_code_completion_set_property;
	object_class->get_property = vr_code_completion_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name",
	                                                      "The provider name",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_ICON,
	                                 g_param_spec_object ("icon",
	                                                      "Icon",
	                                                      "The provider icon",
	                                                      GDK_TYPE_PIXBUF,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_PROPOSALS_BATCH_SIZE,
	                                 g_param_spec_uint ("proposals-batch-size",
	                                                    "Proposals Batch Size",
	                                                    "Number of proposals added in one batch",
	                                                    1,
	                                                    G_MAXUINT,
	                                                    300,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_SCAN_BATCH_SIZE,
	                                 g_param_spec_uint ("scan-batch-size",
	                                                    "Scan Batch Size",
	                                                    "Number of lines scanned in one batch",
	                                                    1,
	                                                    G_MAXUINT,
	                                                    50,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_MINIMUM_WORD_SIZE,
	                                 g_param_spec_uint ("minimum-word-size",
	                                                    "Minimum Word Size",
	                                                    "The minimum word size to complete",
	                                                    2,
	                                                    G_MAXUINT,
	                                                    2,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_INTERACTIVE_DELAY,
	                                 g_param_spec_int ("interactive-delay",
	                                                   "Interactive Delay",
	                                                   "The delay before initiating interactive completion",
	                                                   -1,
	                                                   G_MAXINT,
	                                                   50,
	                                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
	                                 PROP_PRIORITY,
	                                 g_param_spec_int ("priority",
	                                                   "Priority",
	                                                   "Provider priority",
	                                                   G_MININT,
	                                                   G_MAXINT,
	                                                   0,
	                                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


	g_object_class_install_property (object_class,
	                                 PROP_ACTIVATION,
					 g_param_spec_flags ("activation",
							     "Activation",
							     "The type of activation",
							     GTK_SOURCE_TYPE_COMPLETION_ACTIVATION,
							     GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE |
							     GTK_SOURCE_COMPLETION_ACTIVATION_USER_REQUESTED,
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));*/
}

static gboolean
vr_code_completion_get_start_iter (GtkSourceCompletionProvider *provider,
                                            GtkSourceCompletionContext  *context,
                                            GtkSourceCompletionProposal *proposal,
                                            GtkTextIter                 *iter)
{
	/*gchar *word;
	glong nb_chars;

	if (!gtk_source_completion_context_get_iter (context, iter))
	{
		return FALSE;
	}

	word = get_word_at_iter (iter);
	g_return_val_if_fail (word != NULL, FALSE);

	nb_chars = g_utf8_strlen (word, -1);
	gtk_text_iter_backward_chars (iter, nb_chars);

	g_free (word);*/
	return TRUE;
}

static gint vr_code_completion_get_interactive_delay (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS (provider)->priv->interactive_delay;
}

static gint vr_code_completion_get_priority (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS (provider)->priv->priority;
}

static GtkSourceCompletionActivation vr_code_completion_get_activation (GtkSourceCompletionProvider *provider) {
	return GTK_SOURCE_COMPLETION_WORDS (provider)->priv->activation;
}

static void vr_code_completion_iface_init (GtkSourceCompletionProviderIface *iface) {
	iface->get_name = vr_code_completion_get_name;
	iface->get_icon = vr_code_completion_get_icon;
	iface->populate = vr_code_completion_populate;
	iface->get_start_iter = vr_code_completion_get_start_iter;
	iface->get_interactive_delay = vr_code_completion_get_interactive_delay;
	iface->get_priority = vr_code_completion_get_priority;
	iface->get_activation = vr_code_completion_get_activation;
}

static void vr_code_completion_init(VRCodeCompletion *self) {
	self->priv = new VRCodeCompletionPrivate();//vr_code_completion_get_instance_private(self);
	//self->priv->library = vr_code_completion_library_new();
}

VRCodeCompletion* vr_code_completion_new () {
	return (VRCodeCompletion*) g_object_new (GTK_TYPE_SOURCE_COMPLETION_WORDS, NULL);
}

void vr_code_completion_register (VRCodeCompletion* words, GtkTextBuffer* buffer) {
	/*VRCodeCompletionBuffer *buf;
	BufferBinding *binding;

	g_return_if_fail (GTK_SOURCE_IS_COMPLETION_WORDS (words));
	g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));

	binding = g_object_get_data (G_OBJECT (buffer), BUFFER_KEY);

	if (binding != NULL)
	{
		return;
	}

	buf = vr_code_completion_buffer_new (words->priv->library,
	                                              buffer);

	vr_code_completion_buffer_set_scan_batch_size (buf,
	                                                        words->priv->scan_batch_size);

	vr_code_completion_buffer_set_minimum_word_size (buf,
	                                                          words->priv->minimum_word_size);

	binding = g_slice_new (BufferBinding);
	binding->words = words;
	binding->buffer = buf;

	g_object_set_data_full (G_OBJECT (buffer),
	                        BUFFER_KEY,
	                        binding,
	                        (GDestroyNotify)buffer_destroyed);

	words->priv->buffers = g_list_prepend (words->priv->buffers,
	                                       binding);*/
}

/**
 * vr_code_completion_unregister:
 * @words: a #VRCodeCompletion
 * @buffer: a #GtkTextBuffer
 *
 * Unregisters @buffer from the @words provider.
 */
void
vr_code_completion_unregister (VRCodeCompletion *words,
                                        GtkTextBuffer            *buffer)
{
	/*g_return_if_fail (GTK_SOURCE_IS_COMPLETION_WORDS (words));
	g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));

	g_object_set_data (G_OBJECT (buffer), BUFFER_KEY, NULL);*/
}
