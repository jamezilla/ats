/*
POPUP.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"

/*
 * CloseDialog
 *
 * Close the dialog window.  The dialog handle is passed
 * in as the data.
 */
void CloseDialog (GtkWidget *widget, gpointer data)
{

    /* --- Close it. --- */
    gtk_widget_destroy (GTK_WIDGET (data));
}

/*
 * ClosingDialog
 *
 * Calls when window is about to close.  returns FALSE
 * to let it close.
 */
void ClosingDialog (GtkWidget *widget, gpointer data)
{
    gtk_grab_remove (GTK_WIDGET (widget));
}


/*
 * Popup
 *
 * Display a popup dialog window with a message and an 
 * "Ok" button
 */
void Popup (char *szMessage)
{
    static GtkWidget *label;
    GtkWidget *button;
    GtkWidget *dialog_window;

    /* --- Create a dialog window --- */
    dialog_window = gtk_dialog_new ();
    gtk_window_set_resizable(GTK_WINDOW (dialog_window),FALSE);
    /* --- Trap the window close signal to release the grab --- */
    g_signal_connect (G_OBJECT (dialog_window), "destroy",
                      G_CALLBACK(ClosingDialog),
                      &dialog_window);

    /* --- Add a title to the window --- */
    gtk_window_set_title (GTK_WINDOW (dialog_window), "HEY...!!!");

    /* --- Create a small border --- */
    gtk_container_border_width (GTK_CONTAINER (dialog_window), 5);

    /*
     * --- Create the message
     */

    /* --- Create the message in a label --- */
    label = gtk_label_new (szMessage);

    /* --- Put some room around the label --- */
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);

    /* --- Add the label to the dialog --- */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
                        label, TRUE, TRUE, 0);

    /* --- Make the label visible --- */
    gtk_widget_show (label);

    /* 
     * --- "ok" button
     */ 

    /* --- Create the "ok" button --- */
    button = gtk_button_new_with_label ("Ok");

    /* --- Need to close the window if they press "ok" --- */
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK(CloseDialog),
                      dialog_window);

    /* --- Allow it to be the default button --- */
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    /* --- Add the button to the dialog --- */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
			  button, TRUE, TRUE, 0);

    /* --- Make the button the default button --- */
    gtk_widget_grab_default (button);

    /* --- Make the button visible --- */
    gtk_widget_show (button);

    /* --- Make the dialog visible --- */
    gtk_window_set_position(GTK_WINDOW(dialog_window),GTK_WIN_POS_CENTER);
    gtk_widget_show (dialog_window);

    gtk_grab_add (dialog_window);
}





