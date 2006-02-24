/* $Id$ */
/*-
 * Copyright (c) 2003-2006 Benedikt Meurer <benny@xfce.org>.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <exo-helper/exo-helper-chooser-dialog.h>
#include <exo-helper/exo-helper-utils.h>



static void     exo_helper_chooser_dialog_class_init      (ExoHelperChooserDialogClass  *klass);
static void     exo_helper_chooser_dialog_init            (ExoHelperChooserDialog       *chooser_dialog);
static gboolean exo_helper_chooser_dialog_key_press_event (GtkWidget                    *widget,
                                                           GdkEventKey                  *event);



struct _ExoHelperChooserDialogClass
{
  GtkDialogClass __parent__;
};

struct _ExoHelperChooserDialog
{
  GtkDialog __parent__;
};



static GObjectClass *exo_helper_chooser_dialog_parent_class;



GType
exo_helper_chooser_dialog_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (ExoHelperChooserDialogClass),
        NULL,
        NULL,
        (GClassInitFunc) exo_helper_chooser_dialog_class_init,
        NULL,
        NULL,
        sizeof (ExoHelperChooserDialog),
        0,
        (GInstanceInitFunc) exo_helper_chooser_dialog_init,
        NULL,
      };

      type = g_type_register_static (GTK_TYPE_DIALOG, I_("ExoHelperChooserDialog"), &info, 0);
    }

  return type;
}



static void
exo_helper_chooser_dialog_class_init (ExoHelperChooserDialogClass *klass)
{
  GtkWidgetClass *gtkwidget_class;

  /* determine the parent type class */
  exo_helper_chooser_dialog_parent_class = g_type_class_peek_parent (klass);

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->key_press_event = exo_helper_chooser_dialog_key_press_event;
}



static void
exo_helper_chooser_dialog_init (ExoHelperChooserDialog *chooser_dialog)
{
  PangoAttribute *attribute;
  PangoAttrList  *attr_list_bold;
  AtkRelationSet *relations;
  AtkRelation    *relation;
  AtkObject      *object;
  GtkWidget      *notebook;
  GtkWidget      *chooser;
  GtkWidget      *header;
  GtkWidget      *frame;
  GtkWidget      *label;
  GtkWidget      *vbox;
  GtkWidget      *box;

  /* verify category settings */
  g_assert (EXO_HELPER_N_CATEGORIES == 3);

  gtk_dialog_add_button (GTK_DIALOG (chooser_dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (chooser_dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_set_has_separator (GTK_DIALOG (chooser_dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (chooser_dialog), _("Preferred Applications"));

  header = exo_helper_create_header ("preferences-desktop-default-applications", _("Preferred Applications"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (chooser_dialog)->vbox), header, FALSE, FALSE, 0);
  gtk_widget_show (header);

  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (chooser_dialog)->vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  /* allocate shared bold label attributes */
  attr_list_bold = pango_attr_list_new ();
  attribute = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attribute->start_index = 0;
  attribute->end_index = -1;
  pango_attr_list_insert (attr_list_bold, attribute);

  /*
     Internet
   */
  label = gtk_label_new (_("Internet"));
  vbox = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 24, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);

  /*
     Web Browser
   */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "attributes", attr_list_bold, "label", _("Default Web Browser"), NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  box = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("The preferred Web Browser will be used to open\n"
                           "hyperlinks and display help contents."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.0f);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  chooser = exo_helper_chooser_new (EXO_HELPER_WEBBROWSER);
  gtk_box_pack_start (GTK_BOX (box), chooser, FALSE, FALSE, 0);
  gtk_widget_show (chooser);

  /* set Atk label relation for the chooser */
  object = gtk_widget_get_accessible (chooser);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  /*
     Mail Reader
   */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "attributes", attr_list_bold, "label", _("Default Mail Reader"), NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  box = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("The preferred Mail Reader will be used to compose\n"
                           "emails when you click on email addresses."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.0f);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  chooser = exo_helper_chooser_new (EXO_HELPER_MAILREADER);
  gtk_box_pack_start (GTK_BOX (box), chooser, FALSE, FALSE, 0);
  gtk_widget_show (chooser);

  /* set Atk label relation for the chooser */
  object = gtk_widget_get_accessible (chooser);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  /*
     Utilities
   */
  label = gtk_label_new (_("Utilities"));
  vbox = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 24, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (label);
  gtk_widget_show (vbox);

  /*
     Terminal Emulator
   */
  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  label = g_object_new (GTK_TYPE_LABEL, "attributes", attr_list_bold, "label", _("Default Terminal Emulator"), NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_widget_show (label);

  box = g_object_new (GTK_TYPE_VBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("The preferred Terminal Emulator will be used to\n"
                           "run commands that require a CLI environment."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.0f);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  chooser = exo_helper_chooser_new (EXO_HELPER_TERMINALEMULATOR);
  gtk_box_pack_start (GTK_BOX (box), chooser, FALSE, FALSE, 0);
  gtk_widget_show (chooser);

  /* set Atk label relation for the chooser */
  object = gtk_widget_get_accessible (chooser);
  relations = atk_object_ref_relation_set (gtk_widget_get_accessible (label));
  relation = atk_relation_new (&object, 1, ATK_RELATION_LABEL_FOR);
  atk_relation_set_add (relations, relation);
  g_object_unref (G_OBJECT (relation));

  /* cleanup */
  pango_attr_list_unref (attr_list_bold);
}



static gboolean
exo_helper_chooser_dialog_key_press_event (GtkWidget   *widget,
                                           GdkEventKey *event)
{
  if (G_UNLIKELY (event->keyval == GDK_Escape || ((event->state & GDK_CONTROL_MASK) != 0 && (event->keyval == GDK_W || event->keyval == GDK_w))))
    {
      gtk_dialog_response (GTK_DIALOG (widget), GTK_RESPONSE_CLOSE);
      return TRUE;
    }

  return (*GTK_WIDGET_CLASS (exo_helper_chooser_dialog_parent_class)->key_press_event) (widget, event);
}



/**
 * exo_helper_chooser_dialog_new:
 *
 * Allocates a new #ExoHelperChooserDialog.
 *
 * Return value: the newly allocated #ExoHelperChooserDialog.
 **/
GtkWidget*
exo_helper_chooser_dialog_new (void)
{
  return g_object_new (EXO_TYPE_HELPER_CHOOSER_DIALOG, NULL);
}


