/*
descq.c
Michael Leidel (Feb 2021)
OTHER FILES:
    descq.glade     programs GUI layout xml file
    data/*.txt      data files for user's content ...
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "stringalt.h"
#include "gtk/gtk.h"
#include "gdk/gdk.h"

#define EDITOR "gedit"
#define BUFFER1 1024
#define BUFFER2 2048
#define BUFFER3 1048576

// Global Pointers
GtkWidget *g_dlg_listbox;
GtkWidget *g_dialog_box;
GtkWidget *g_entry;
GtkWidget *g_label;
GtkWidget *g_messagebox;
GtkClipboard *g_clipboard;
GtkWidget *g_wnd;

char g_last[BUFFER2];  // holds previous command

int main(int argc, char *argv[])
{
    GtkBuilder      *builder;
    GtkWidget       *window;
    FILE            *fh;
    int             w_top;
    int             w_left;
    int             w_width;
    int             w_height;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "descq.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    gtk_builder_connect_signals(builder, NULL);

    g_wnd = window;  // to use in subroutines

    // set GtkWidget Pointers to their builder objects
    // g_ = GTK_WIDGET(gtk_builder_get_object(builder, "WIDGET_ID"));
    g_dlg_listbox = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_listbox"));
    g_dialog_box = GTK_WIDGET(gtk_builder_get_object(builder, "dialog_list"));
    g_entry = GTK_WIDGET(gtk_builder_get_object(builder, "entry"));
    g_clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_messagebox = GTK_WIDGET(gtk_builder_get_object(builder, "dlg_messagebox"));

    g_object_unref(builder);

    // load the window metrics
    char line[64];
    fh = open_for_read("data/winmet.txt");
    fgets(line, 64, fh);
    fclose(fh);
    removen(line);  // remove new line character
    fields(line, ",");
    w_left      = atoi(_fields[0]);
    w_top       = atoi(_fields[1]);
    w_width     = atoi(_fields[2]);
    w_height    = atoi(_fields[3]);

    gtk_widget_show(window);
    gtk_window_move(GTK_WINDOW(g_wnd), w_left, w_top);  // set metrics ...
    gtk_window_resize(GTK_WINDOW(g_wnd), w_width, w_height);
    if (argc < 2) {
        gtk_window_set_decorated (GTK_WINDOW(g_wnd), 0);  // turn off caption bar
    }

    gtk_main();

    return 0;
}

// Display a message box
void show_message(char pmsg[64], char smsg[64]) {
    gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG(g_messagebox), pmsg);
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(g_messagebox), smsg, NULL);
    gtk_dialog_run(GTK_DIALOG(g_messagebox));
}

// called when window is closed
void on_window1_destroy()
{
    gtk_main_quit();
}

void displayListDlg(char * target) {
    GtkListBoxRow *g_row;
    FILE *fh;
    char line[BUFFER2];
    char filename[25];

    // remove any existing rows
    while (1) {
        g_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(g_dlg_listbox), 0);
        if (g_row == NULL) break;
        gtk_widget_destroy (GTK_WIDGET(g_row));
    }
    // Set the target file and caption heading
    if (strcmp(target, "urls") == 0) {
        strcpy(filename, "data/urls.txt");
        gtk_window_set_title(GTK_WINDOW(g_dialog_box), "Saved URL List");
    } else {
        strcpy(filename, "data/hist.txt");
        gtk_window_set_title(GTK_WINDOW(g_dialog_box), "Saved Search Queries");
    }

    fh = open_for_read(filename);
    while(1) {
        fgets(line, BUFFER2, fh);
        if (feof(fh)) break;
        removen(line);  // remove newline character
        g_label = gtk_label_new (line);
        gtk_label_set_xalign (GTK_LABEL(g_label), 0.0);
        gtk_list_box_insert(GTK_LIST_BOX(g_dlg_listbox), g_label, -1);
    }
    fclose(fh);
    //  Show the dialog box with the listbox
    gtk_widget_show_all(g_dialog_box);
    gtk_dialog_run(GTK_DIALOG(g_dialog_box));
    gtk_widget_hide (g_dialog_box);
}

void on_dlg_listbox_row_activated(GtkListBox *oList, GtkListBoxRow *oRow) {
    GtkWidget *bin;
    char listdata[BUFFER2];
    char url[BUFFER2];

    bin = gtk_bin_get_child(GTK_BIN(oRow));
    strcpy(listdata, gtk_label_get_text(GTK_LABEL(bin)));
    if (g_str_has_prefix(listdata, "http")) {
        strcpy(url, "xdg-open ");
        strcat(url, listdata);
        system(url);
        gtk_widget_hide (g_dialog_box);
    } else {
        listdata[strlen(listdata) - 14] = '\0';  // cut off the date
        replace(listdata, " ", "%20");
        strcpy(url, "xdg-open ");
        strcat(url, "https://duckduckgo.com/?q=");
        strcat(url, "\"");
        strcat(url, listdata);  // quotes necessary incase of embeded quote
        strcat(url, "\"");
        system(url);
    }
}

// Save url to urls.txt file
void write_url(char *text) {
    FILE *fh;
    char rec[500][BUFFER2];
    int count = 0;
    fh = open_for_read("data/urls.txt");
    while(1) {
        fgets(rec[count], BUFFER2, fh);
        if (feof(fh)) break;
        removen(rec[count++]);  // remove newline character
        if (count > 498) return;  // too many urls !!
    }
    fclose(fh);
    fh = open_for_write("data/urls.txt");
    fprintf(fh, "%s\n", text);
    for(int x=0; x < count; x++) {
       fprintf(fh, "%s\n", rec[x]);
    }
    fclose(fh);
}

// Add a seach text to hist.txt file
void write_history(char *text) {
    FILE *fh;
    char buf[80];
    char rec[500][BUFFER2];
    int count = 0;
    fh = open_for_read("data/hist.txt");
    while(1) {
        fgets(rec[count], BUFFER2, fh);
        if (feof(fh)) break;
        removen(rec[count++]);  // remove newline character
        if (count > 498) return;  // too many hist recs !!
    }
    fclose(fh);
    fh = open_for_write("data/hist.txt");
    fprintf(fh, "%s -- %s\n", text, today(buf));
    for(int x=0; x < count; x++) {
       fprintf(fh, "%s\n", rec[x]);
    }
    fclose(fh);
}

// Save clipboard text contents to "clip.txt" file
void save_clipboard_to_file() {
    FILE *fh;
    char cliptxt[BUFFER3];  // 1MG

    if (!gtk_clipboard_wait_is_text_available(g_clipboard)) {
        return;
    }
    strcpy(cliptxt, gtk_clipboard_wait_for_text(g_clipboard));
    fh = open_for_append("data/clip.txt");
    fprintf(fh,"%s\n", cliptxt);
    fclose(fh);
}

// "Close" was clicked in the messagebox
void on_btn_dlg_msg_close_clicked() {
    gtk_widget_hide(g_messagebox);
}

/**************************************
    The user has typed something into the entry field and hit Enter.
    (or might have clicked the button instead of hitting Enter)
    Basically, we need to determine if it is a:
    1. descq command (e.g. list ...)
    2. command from the serv.txt file
    3. Internet search text
    NOTE:
        Any time the text in the entry field starts with "http"
        the text gets saved to the urls.txt file. (list)
        Every Internet search gets saved in the hist.txt file. (hist)
***************************************/
void on_entry_activate(GtkEntry *entry) {
    FILE *fh;
    char action[BUFFER2];
    char line[BUFFER2];
    char *ptr;
    int count = 0;
    int rsp = 0;
    gchar out_str[BUFFER2] = {0};
    int w_top;
    int w_left;
    int w_width;
    int w_height;

    sprintf(out_str, "%s", gtk_entry_get_text(entry));
    gtk_entry_set_text(entry, "");  // clear the entry widget text
    trim(out_str);  // stringalt.h
    if (strcmp(out_str, "") == 0) {
        return;
    }

    strcpy(g_last, out_str);  // store command text for up/down arrow

    if (strcmp(out_str, "list") == 0) {             // list urls
        displayListDlg("urls");

    } else if (strcmp(out_str, "hist") == 0) {      // list history
        displayListDlg("hist");

    } else if (startswith("http", out_str)) {       // saves URL to urls.txt
        write_url(out_str);

    } else if (strcmp(out_str, "sc") == 0) {        // Save clipboard to clip.txt
        save_clipboard_to_file();

    } else if (strcmp(out_str, "es") == 0) {        // edit serv.txt
        strcpy(action, EDITOR);
        strcat(action, " data/serv.txt &");
        system(action);

    } else if (strcmp(out_str, "eh") == 0) {        // edit hist.txt
        strcpy(action, EDITOR);
        strcat(action, " data/hist.txt &");
        system(action);

    } else if (strcmp(out_str, "ec") == 0) {        // edit clip.txt
        strcpy(action, EDITOR);
        strcat(action, " data/clip.txt &");
        system(action);

    } else if (strcmp(out_str, "eu") == 0) {        // edit urls.txt
        strcpy(action, EDITOR);
        strcat(action, " data/urls.txt &");
        system(action);

    } else if (strcmp(out_str, "help") == 0) {        // edit help.txt
        strcpy(action, EDITOR);
        strcat(action, " data/help.txt &");
        int rsp = system(action);
        if (rsp != 0) {
            show_message("<big>gedit</big>", "is not installed.");
        }

    } else if (strcmp(out_str, "x") == 0) {         // exit program
        on_window1_destroy();

    } else if (strcmp(out_str, "cap") == 0) {        // toggle window caption bar
        if (gtk_window_get_decorated(GTK_WINDOW(g_wnd))) {
            gtk_window_set_decorated (GTK_WINDOW(g_wnd), 0);
        } else {
            gtk_window_set_decorated (GTK_WINDOW(g_wnd), 1);
        }

    } else if (strcmp(out_str, "winset") == 0) {        // save window position
        gtk_window_get_position (GTK_WINDOW(g_wnd), &w_left, &w_top);
        gtk_window_get_size (GTK_WINDOW(g_wnd), &w_width, &w_height);
        fh = open_for_write("data/winmet.txt");
        fprintf(fh, "%d,%d,%d,%d\n", w_left, w_top, w_width, w_height);
        fclose(fh);
        // printf("top = %d  left = %d\n", w_top, w_left);
        // printf("width = %d  height = %d\n", w_width, w_height);

    } else if (strchr(".$;", out_str[0])) {         // run a command from serv.txt
        // open serv.txt and locate the commnad
        fh = open_for_read("data/serv.txt");
        while(1) {
            fgets(line, BUFFER2, fh);
            if (feof(fh)) break;
            removen(line);  // remove newline character
            ptr = strchr(line, ',');  // get pointer to the first ','
            if (ptr) {
                strcpy(action, ptr + 1);  // one past the ','
                *ptr = '\0';  // replace ',' with end of string \0
                ltrim(action);  // incase of space after the ','
                if (startswith(out_str+1, line+1)) {  // skip past the '.;$' char
                    // if action is a website
                     if (g_str_has_prefix(action, "http")) {
                        strcpy(line, "xdg-open ");
                        strcat(line, action);
                        printf("%s\n", line);
                        system(line);
                     } else {  // it has to be a local command
                        rsp = system(action);
                        if (rsp != 0) {
                            show_message(out_str,"returned an error");
                        }
                     }
                     fclose(fh);
                     return;
                }
            }
        }  // end while
        fclose(fh);
        show_message(out_str, "<b>Not Found in serv.txt</b>");

    } else if (out_str[1] == ':') {  // run a service query from serv.txt
        action[0] = out_str[0];
        action[1] = ',';
        action[2] = '\0';
        // Read the serv.txt file to find the correct line: letter:searchtext
        fh = open_for_read("data/serv.txt");
        while(1) {
            fgets(line, BUFFER2, fh);
            if (feof(fh)) break;
            if (startswith(action, line)) {
                write_history(out_str);
                replace(out_str, " ", "%20");
                removen(line);  // remove newline character
                count = fields(line, ",");   // stringalt.h
                strcpy(line, "xdg-open ");  // build the command ...
                strcat(line, _fields[2]);  // should return count = 3 (0,1,2)
                strcat(line, out_str+2);  // bypass first two chars
                system(line);
            }
        }
        fclose(fh);

    } else {
        // Internet Search using default browser
        write_history(out_str);
        replace(out_str, " ", "%20");
        strcpy(action, "xdg-open ");
        strcat(action, "https://duckduckgo.com/?q=");
        strcat(action, "\"");
        strcat(action, out_str);  // quotes needed incase of embedded quote
        strcat(action, "\"");
        system(action);
    }
}

 // Execute the entry text,
// or if entry is empty save to clipboard.
void on_btn_entry_clicked() {
    gchar out_str[BUFFER2] = {0};
    sprintf(out_str, "%s", gtk_entry_get_text(GTK_ENTRY(g_entry)));
    trim(out_str);  // stringalt.h
    if (strcmp(out_str, "") == 0) {  // Try to save clipboard contents to clip.txt
        // when entry field is empty.
        save_clipboard_to_file();
    } else {
        on_entry_activate(GTK_ENTRY(g_entry));  // else treat it as Enter key pressed
    }
}

_Bool on_window1_key_press_event(GtkWidget *w, GdkEvent *e) {
    guint keyval;

    gdk_event_get_keyval (e, &keyval);
    //printf("%d\n", keyval);

    if (keyval == 65362 || keyval == 65364) {
        if (strcmp(g_last, "") != 0) {
            gtk_entry_set_text(GTK_ENTRY(g_entry), g_last);
        }
    } else {
        return FALSE;
    }
}


// Exit the program
void on_btn_dlg_close_clicked() {
    gtk_widget_hide(g_dialog_box);
}
