/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gio/gio.h>
#include <exo-desktop-item-edit/exo-die-editor.h>
#include <exo-desktop-item-edit/exo-die-utils.h>

#include <exo-support/exo-support.h>

#if defined(GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

/* string with fallback spoort */
#define STR_FB(string,fallback) ((string != NULL && *string != '\0') ? string : fallback)



/* --- constants --- */
static const gchar *CREATE_TITLES[] = { N_ ("Create Launcher"), N_ ("Create Link") };
static const gchar *EDIT_TITLES[] = { N_ ("Edit Launcher"), N_ ("Edit Link") };
static const gchar *ICON_NAMES[] = { "applications-other", "applications-internet" };



/* --- globals --- */
static gboolean opt_create_new = FALSE;
static gboolean opt_version = FALSE;
static gchar   *opt_type = NULL;
static gchar   *opt_name = NULL;
static gchar   *opt_comment = NULL;
static gchar   *opt_command = NULL;
static gchar   *opt_url = NULL;
static gchar   *opt_icon = NULL;
static gint     opt_xid = 0;



/* --- command line options --- */
static GOptionEntry option_entries[] =
{
  { "create-new", 'c', 0, G_OPTION_ARG_NONE, &opt_create_new, N_ ("Create a new desktop file in the given directory"), NULL, },
  { "type", 't', 0, G_OPTION_ARG_STRING, &opt_type, N_ ("Type of desktop file to create (Application or Link)"), NULL, },
  { "name", 0, 0, G_OPTION_ARG_STRING, &opt_name, N_ ("Preset name when creating a desktop file"), NULL, },
  { "comment", 0, 0, G_OPTION_ARG_STRING, &opt_comment, N_ ("Preset comment when creating a desktop file"), NULL, },
  { "command", 0, 0, G_OPTION_ARG_STRING, &opt_command, N_ ("Preset command when creating a launcher"), NULL, },
  { "url", 0, 0, G_OPTION_ARG_STRING, &opt_url, N_ ("Preset URL when creating a link"), NULL, },
  { "icon", 0, 0, G_OPTION_ARG_STRING, &opt_icon, N_ ("Preset icon when creating a desktop file"), NULL, },
  { "version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, N_ ("Print version information and exit"), NULL, },
  { "xid", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &opt_xid, NULL, NULL, },
  { NULL, },
};



int
main (int argc, char **argv)
{
  ExoDieEditorMode mode;
  GEnumClass      *enum_klass;
  GEnumValue      *enum_value;
#if defined(GDK_WINDOWING_X11)
  GdkWindow       *xwindow;
#endif
  GtkWidget       *chooser;
  GtkWidget       *message;
  GtkWidget       *button;
  GtkWidget       *dialog;
  GtkWidget       *editor;
  GKeyFile        *key_file;
  GError          *error = NULL;
  gchar           *base_name;
  gchar           *value;
  gchar           *s;
  gint             response;
  gint             result = EXIT_SUCCESS;
  GFile           *gfile, *gfile_parent;
  gchar           *contents;
  gsize            length = 0;
  gboolean         res;
  GFileType        file_type;

  /* setup translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifdef G_ENABLE_DEBUG
  /* Do NOT remove this line for now, If something doesn't work,
   * fix your code instead!
   */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* initialize the GThread system */
  if (!g_thread_supported ())
    g_thread_init (NULL);

  /* initialize Gtk+ */
  if (!gtk_init_with_args (&argc, &argv, _("[FILE|FOLDER]"), option_entries, GETTEXT_PACKAGE, &error))
    {
      /* determine the error message */
      if (G_UNLIKELY (error != NULL))
        {
          /* use the supplied error message */
          g_critical ("%s", error->message);
          g_error_free (error);
        }
      else
        {
          /* no error message, the GUI initialization failed */
          g_critical ("%s %s", _("Failed to open display"),
                      STR_FB (gdk_get_display_arg_name (), ""));
        }

      /* and fail */
      return EXIT_FAILURE;
    }

  /* check if we should print version information */
  if (G_UNLIKELY (opt_version))
    {
      g_print ("%s %s\n\n", g_get_prgname (), PACKAGE_VERSION);
      g_print (_("Copyright (c) %s\n"
                 "        os-cillation e.K. All rights reserved.\n\n"
                 "Written by Benedikt Meurer <benny@xfce.org>.\n\n"),
               "2005-2007");
      g_print (_("%s comes with ABSOLUTELY NO WARRANTY,\n"
                 "You may redistribute copies of %s under the terms of\n"
                 "the GNU Lesser General Public License which can be found in the\n"
                 "%s source package.\n\n"), g_get_prgname (), g_get_prgname (), PACKAGE_TARNAME);
      g_print (_("Please report bugs to <%s>.\n"), PACKAGE_BUGREPORT);
      return EXIT_SUCCESS;
    }

  /* verify that a file/folder is specified */
  if (G_UNLIKELY (argc != 2))
    {
      g_critical (_("No file/folder specified"));
      return EXIT_FAILURE;
    }

  /* allocate a key file */
  key_file = g_key_file_new ();
  gfile = g_file_new_for_commandline_arg (argv[1]);

  /* create new key file if --create-new was specified */
  if (G_LIKELY (opt_create_new))
    {
      /* generic stuff */
      g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_VERSION, "1.0");
      g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_TYPE, STR_FB (opt_type, "Application"));
      g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_NAME, STR_FB (opt_name, ""));
      g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                            G_KEY_FILE_DESKTOP_KEY_COMMENT, STR_FB (opt_comment, ""));

      /* type specific stuff */
      if (exo_str_is_equal (opt_type, G_KEY_FILE_DESKTOP_TYPE_LINK))
        {
          g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_ICON, STR_FB (opt_icon, "gnome-fs-bookmark"));
          g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_URL, STR_FB (opt_url, ""));
        }
      else
        {
          g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_EXEC, STR_FB (opt_command, ""));
          g_key_file_set_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_ICON, STR_FB (opt_icon, ""));
        }
    }
  else
    {
      /* try to load the entire file into memory */
      res = g_file_load_contents (gfile, NULL, &contents, &length, NULL, &error);
      if (G_UNLIKELY (!res || length == 0))
        {
          /* we cannot open the file */
          if (G_LIKELY (error != NULL))
            {
              g_critical (_("Failed to load contents from \"%s\": %s"), argv[1], error->message);
              g_error_free (error);
            }
          else
            {
              g_critical (_("The file \"%s\" contains no data"), argv[1]);
            }

          return EXIT_FAILURE;
        }

      /* load the data into the key file */
      res = g_key_file_load_from_data (key_file, contents, length, G_KEY_FILE_KEEP_COMMENTS
                                       | G_KEY_FILE_KEEP_TRANSLATIONS, &error);
      g_free (contents);
      if (G_UNLIKELY (!res))
        {
          /* failed to parse the file */
          g_critical (_("Failed to parse contents of \"%s\": %s"), argv[1], error->message);
          g_error_free (error);
          return EXIT_FAILURE;
        }
    }

  /* determine the type of the desktop file */
  value = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 G_KEY_FILE_DESKTOP_KEY_TYPE, NULL);
  if (G_UNLIKELY (value == NULL))
    {
      /* we cannot continue without a type */
      g_critical (_("File \"%s\" has no type key"), argv[1]);
      return EXIT_FAILURE;
    }

  /* verify that we support the type */
  enum_klass = g_type_class_ref (EXO_DIE_TYPE_EDITOR_MODE);
  enum_value = g_enum_get_value_by_nick (enum_klass, value);
  if (G_UNLIKELY (enum_value == NULL))
    {
      /* tell the user that we don't support the type */
      g_critical (_("Unsupported desktop file type \"%s\""), value);
      return EXIT_FAILURE;
    }
  g_free (value);
  mode = enum_value->value;
  g_type_class_unref (enum_klass);

  /* allocate the dialog */
  dialog = xfce_titled_dialog_new_with_buttons (opt_create_new ? _(CREATE_TITLES[mode]) : _(EDIT_TITLES[mode]),
                                                NULL, GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 375);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), ICON_NAMES[mode]);

  /* add the "Create"/"Save" button (as default) */
  button = gtk_button_new_from_stock (opt_create_new ? _("C_reate") : GTK_STOCK_SAVE);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_ACCEPT);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /* allocate the editor widget */
  editor = exo_die_editor_new ();
  exo_die_editor_set_mode (EXO_DIE_EDITOR (editor), mode);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  exo_binding_new (G_OBJECT (editor), "complete", G_OBJECT (button), "sensitive");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  /* setup the name */
  value = g_key_file_get_locale_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
  exo_die_editor_set_name (EXO_DIE_EDITOR (editor), (value != NULL) ? value : "");
  g_free (value);

  /* setup the comment */
  value = g_key_file_get_locale_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_COMMENT, NULL, NULL);
  exo_die_editor_set_comment (EXO_DIE_EDITOR (editor), (value != NULL) ? value : "");
  g_free (value);

  /* setup the icon (automatically fixing broken icons) */
  value = g_key_file_get_locale_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_ICON, NULL, NULL);
  if (value != NULL && !g_path_is_absolute (value))
    {
      /* check if this is an invalid icon declaration */
      s = strrchr (value, '.');
      if (G_UNLIKELY (s != NULL))
        *s = '\0';
    }
  exo_die_editor_set_icon (EXO_DIE_EDITOR (editor), (value != NULL) ? value : "");
  g_free (value);

  /* mode specific stuff */
  switch (mode)
    {
    case EXO_DIE_EDITOR_MODE_APPLICATION:
      /* setup the command */
      value = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                     G_KEY_FILE_DESKTOP_KEY_EXEC, NULL);
      exo_die_editor_set_command (EXO_DIE_EDITOR (editor), (value != NULL) ? value : "");
      g_free (value);

      /* setup launcher options */
      exo_die_editor_set_snotify (EXO_DIE_EDITOR (editor),
          g_key_file_get_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                  G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY, NULL));
      exo_die_editor_set_terminal (EXO_DIE_EDITOR (editor),
          g_key_file_get_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                  G_KEY_FILE_DESKTOP_KEY_TERMINAL, NULL));
      break;

    case EXO_DIE_EDITOR_MODE_LINK:
      /* setup the URL */
      value = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                     G_KEY_FILE_DESKTOP_KEY_URL, NULL);
      exo_die_editor_set_url (EXO_DIE_EDITOR (editor), (value != NULL) ? value : "");
      g_free (value);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /* check if a parent window id was specified */
#if defined(GDK_WINDOWING_X11)
  if (G_UNLIKELY (opt_xid != 0))
    {
      /* try to determine the window for the id */
      xwindow = gdk_window_foreign_new (opt_xid);
      if (G_LIKELY (xwindow != NULL))
        {
          /* realize the dialog first... */
          gtk_widget_realize (dialog);

          /* ...and set the "transient for" relation */
          gdk_window_set_transient_for (dialog->window, xwindow);
        }
    }
#endif

  /* run the dialog */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_ACCEPT)
    {
      /* save common values (localized if possible) */
      exo_die_g_key_file_set_locale_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                           G_KEY_FILE_DESKTOP_KEY_NAME,
                                           exo_die_editor_get_name (EXO_DIE_EDITOR (editor)));
      exo_die_g_key_file_set_locale_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                           G_KEY_FILE_DESKTOP_KEY_ICON,
                                           exo_die_editor_get_icon (EXO_DIE_EDITOR (editor)));
      exo_die_g_key_file_set_locale_value (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                           G_KEY_FILE_DESKTOP_KEY_COMMENT,
                                           exo_die_editor_get_comment (EXO_DIE_EDITOR (editor)));

      /* save mode specific stuff */
      switch (mode)
        {
        case EXO_DIE_EDITOR_MODE_APPLICATION:
          /* save the new command */
          g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 G_KEY_FILE_DESKTOP_KEY_EXEC,
                                 exo_die_editor_get_command (EXO_DIE_EDITOR (editor)));

          /* save the new launcher options */
          g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 G_KEY_FILE_DESKTOP_KEY_TERMINAL,
                                 exo_die_editor_get_terminal (EXO_DIE_EDITOR (editor)));
          g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY,
                                 exo_die_editor_get_snotify (EXO_DIE_EDITOR (editor)));
          break;

        case EXO_DIE_EDITOR_MODE_LINK:
          /* save the new URL */
          g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP,
                                 G_KEY_FILE_DESKTOP_KEY_URL,
                                 exo_die_editor_get_url (EXO_DIE_EDITOR (editor)));
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      /* try to save the file */
      if (!exo_die_g_key_file_save (key_file, opt_create_new, gfile, &error) && opt_create_new)
        {
          /* reset the error */
          g_clear_error (&error);

          /* create failed, ask the user to specify a file name */
          chooser = gtk_file_chooser_dialog_new (_("Choose filename"),
                                                 GTK_WINDOW (dialog),
                                                 GTK_FILE_CHOOSER_ACTION_SAVE,
                                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                 NULL);
          gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);
          gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser), TRUE);

          file_type = g_file_query_file_type (gfile, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL);

          /* if base is a folder, enter the folder */
          if (file_type == G_FILE_TYPE_DIRECTORY)
            {
              gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (chooser), gfile, NULL);
              gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), "new-file.desktop");
            }
          else if (file_type == G_FILE_TYPE_REGULAR)
            {
              gfile_parent = g_file_get_parent (gfile);
              if (G_LIKELY (gfile_parent != NULL))
                {
                  gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (chooser), gfile_parent, NULL);
                  g_object_unref (G_OBJECT (gfile_parent));
                }

              base_name = g_file_get_basename (gfile);
              gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), base_name);
              g_free (base_name);
            }

          /* run the chooser */
          response = gtk_dialog_run (GTK_DIALOG (chooser));
          if (G_LIKELY (response == GTK_RESPONSE_ACCEPT))
            {
              /* release the previous file name */
              if (G_LIKELY (gfile != NULL))
                g_object_unref (G_OBJECT (gfile));

              /* try again to save to the new file */
              gfile = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
              exo_die_g_key_file_save (key_file, FALSE, gfile, &error);
            }

          /* destroy the chooser */
          gtk_widget_destroy (chooser);
        }

      /* check if we failed to save/create */
      if (G_UNLIKELY (response == GTK_RESPONSE_ACCEPT && error != NULL))
        {
          /* display an error message to the user */
          s = g_file_get_uri (gfile);
          message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                            GTK_DIALOG_DESTROY_WITH_PARENT
                                            | GTK_DIALOG_MODAL,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            opt_create_new ? _("Failed to create \"%s\".") : _("Failed to save \"%s\"."),
                                            s);
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s.", error->message);
          gtk_dialog_run (GTK_DIALOG (message));
          gtk_widget_destroy (message);
          g_free (s);

          /* jep, we failed */
          result = EXIT_FAILURE;
        }

      /* reset the error */
      g_clear_error (&error);
    }

  /* destroy the editor dialog */
  gtk_widget_destroy (dialog);

  /* cleanup */
  g_key_file_free (key_file);
  g_object_unref (G_OBJECT (gfile));

  return result;
}
