// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/gtk/gtk_custom_menu_item.h"

#include "base/i18n/rtl.h"
#include "chrome/browser/ui/gtk/gtk_custom_menu.h"

enum {
  BUTTON_PUSHED,
  TRY_BUTTON_PUSHED,
  LAST_SIGNAL
};

static guint custom_menu_item_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(GtkCustomMenuItem, gtk_custom_menu_item, GTK_TYPE_MENU_ITEM)

static void set_selected(GtkCustomMenuItem* item, GtkWidget* selected) {
  if (selected != item->currently_selected_button) {
    if (item->currently_selected_button) {
      gtk_widget_set_state(item->currently_selected_button, GTK_STATE_NORMAL);
      gtk_widget_set_state(
          gtk_bin_get_child(GTK_BIN(item->currently_selected_button)),
          GTK_STATE_NORMAL);
    }

    item->currently_selected_button = selected;
    if (item->currently_selected_button) {
      gtk_widget_set_state(item->currently_selected_button, GTK_STATE_SELECTED);
      gtk_widget_set_state(
          gtk_bin_get_child(GTK_BIN(item->currently_selected_button)),
          GTK_STATE_PRELIGHT);
    }
  }
}

// When GtkButtons set the label text, they rebuild the widget hierarchy each
// and every time. Therefore, we can't just fish out the label from the button
// and set some properties; we have to create this callback function that
// listens on the button's "notify" signal, which is emitted right after the
// label has been (re)created. (Label values can change dynamically.)
static void on_button_label_set(GObject* object) {
  GtkButton* button = GTK_BUTTON(object);
  gtk_widget_set_sensitive(GTK_BIN(button)->child, FALSE);
  gtk_misc_set_padding(GTK_MISC(GTK_BIN(button)->child), 2, 0);
}

static void gtk_custom_menu_item_finalize(GObject *object);
static gint gtk_custom_menu_item_expose(GtkWidget* widget,
                                        GdkEventExpose* event);
static gboolean gtk_custom_menu_item_hbox_expose(GtkWidget* widget,
                                                 GdkEventExpose* event,
                                                 GtkCustomMenuItem* menu_item);
static void gtk_custom_menu_item_select(GtkItem *item);
static void gtk_custom_menu_item_deselect(GtkItem *item);
static void gtk_custom_menu_item_activate(GtkMenuItem* menu_item);

static void gtk_custom_menu_item_init(GtkCustomMenuItem* item) {
  item->all_widgets = NULL;
  item->button_widgets = NULL;
  item->currently_selected_button = NULL;
  item->previously_selected_button = NULL;

  GtkWidget* menu_hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(item), menu_hbox);

  item->label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(item->label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(menu_hbox), item->label, TRUE, TRUE, 0);

  item->hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_end(GTK_BOX(menu_hbox), item->hbox, FALSE, FALSE, 0);

  g_signal_connect(item->hbox, "expose-event",
                   G_CALLBACK(gtk_custom_menu_item_hbox_expose),
                   item);

  gtk_widget_show_all(menu_hbox);
}

static void gtk_custom_menu_item_class_init(GtkCustomMenuItemClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
  GtkItemClass* item_class = GTK_ITEM_CLASS(klass);
  GtkMenuItemClass* menu_item_class = GTK_MENU_ITEM_CLASS(klass);

  gobject_class->finalize = gtk_custom_menu_item_finalize;

  widget_class->expose_event = gtk_custom_menu_item_expose;

  item_class->select = gtk_custom_menu_item_select;
  item_class->deselect = gtk_custom_menu_item_deselect;

  menu_item_class->activate = gtk_custom_menu_item_activate;

  custom_menu_item_signals[BUTTON_PUSHED] =
      g_signal_new("button-pushed",
                   G_OBJECT_CLASS_TYPE(gobject_class),
                   G_SIGNAL_RUN_FIRST,
                   0,
                   NULL, NULL,
                   gtk_marshal_NONE__INT,
                   G_TYPE_NONE, 1, GTK_TYPE_INT);
  // TODO(erg): Change from BOOL__POINTER to BOOLEAN__INTEGER when we get to
  // use a modern GTK+.
  custom_menu_item_signals[TRY_BUTTON_PUSHED] =
      g_signal_new("try-button-pushed",
                   G_OBJECT_CLASS_TYPE(gobject_class),
                   G_SIGNAL_RUN_LAST,
                   0,
                   NULL, NULL,
                   gtk_marshal_BOOL__POINTER,
                   G_TYPE_BOOLEAN, 1, GTK_TYPE_INT);
}

static void gtk_custom_menu_item_finalize(GObject *object) {
  GtkCustomMenuItem* item = GTK_CUSTOM_MENU_ITEM(object);
  g_list_free(item->all_widgets);
  g_list_free(item->button_widgets);

  G_OBJECT_CLASS(gtk_custom_menu_item_parent_class)->finalize(object);
}

static gint gtk_custom_menu_item_expose(GtkWidget* widget,
                                        GdkEventExpose* event) {
  if (GTK_WIDGET_VISIBLE(widget) &&
      GTK_WIDGET_MAPPED(widget) &&
      gtk_bin_get_child(GTK_BIN(widget))) {
    // We skip the drawing in the GtkMenuItem class it draws the highlighted
    // background and we don't want that.
    gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                   gtk_bin_get_child(GTK_BIN(widget)),
                                   event);
  }

  return FALSE;
}

static void gtk_custom_menu_item_expose_button(GtkWidget* hbox,
                                               GdkEventExpose* event,
                                               GList* button_item) {
  // We search backwards to find the leftmost and rightmost buttons. The
  // current button may be that button.
  GtkWidget* current_button = GTK_WIDGET(button_item->data);
  GtkWidget* first_button = current_button;
  for (GList* i = button_item; i && GTK_IS_BUTTON(i->data);
       i = g_list_previous(i)) {
    first_button = GTK_WIDGET(i->data);
  }

  GtkWidget* last_button = current_button;
  for (GList* i = button_item; i && GTK_IS_BUTTON(i->data);
       i = g_list_next(i)) {
    last_button = GTK_WIDGET(i->data);
  }

  if (base::i18n::IsRTL())
    std::swap(first_button, last_button);

  int x = first_button->allocation.x;
  int y = first_button->allocation.y;
  int width = last_button->allocation.width + last_button->allocation.x -
              first_button->allocation.x;
  int height = last_button->allocation.height;

  gtk_paint_box(hbox->style, hbox->window,
                static_cast<GtkStateType>(
                    GTK_WIDGET_STATE(current_button)),
                GTK_SHADOW_OUT,
                &current_button->allocation, hbox, "button",
                x, y, width, height);

  // Propagate to the button's children.
  gtk_container_propagate_expose(
      GTK_CONTAINER(current_button),
      gtk_bin_get_child(GTK_BIN(current_button)),
      event);
}

static gboolean gtk_custom_menu_item_hbox_expose(GtkWidget* widget,
                                                 GdkEventExpose* event,
                                                 GtkCustomMenuItem* menu_item) {
  // First render all the buttons that aren't the currently selected item.
  for (GList* current_item = menu_item->all_widgets;
       current_item != NULL; current_item = g_list_next(current_item)) {
    if (GTK_IS_BUTTON(current_item->data)) {
      if (GTK_WIDGET(current_item->data) !=
          menu_item->currently_selected_button) {
        gtk_custom_menu_item_expose_button(widget, event, current_item);
      }
    }
  }

  // As a separate pass, draw the buton separators above. We need to draw the
  // separators in a separate pass because we are drawing on top of the
  // buttons. Otherwise, the vlines are overwritten by the next button.
  for (GList* current_item = menu_item->all_widgets;
       current_item != NULL; current_item = g_list_next(current_item)) {
    if (GTK_IS_BUTTON(current_item->data)) {
      // Check to see if this is the last button in a run.
      GList* next_item = g_list_next(current_item);
      if (next_item && GTK_IS_BUTTON(next_item->data)) {
        GtkWidget* current_button = GTK_WIDGET(current_item->data);
        GtkAllocation child_alloc =
            gtk_bin_get_child(GTK_BIN(current_button))->allocation;
        int half_offset = widget->style->xthickness / 2;
        gtk_paint_vline(widget->style, widget->window,
                        static_cast<GtkStateType>(
                            GTK_WIDGET_STATE(current_button)),
                        &event->area, widget, "button",
                        child_alloc.y,
                        child_alloc.y + child_alloc.height,
                        current_button->allocation.x +
                        current_button->allocation.width - half_offset);
      }
    }
  }

  // Finally, draw the selected item on top of the separators so there are no
  // artifacts inside the button area.
  GList* selected = g_list_find(menu_item->all_widgets,
                                menu_item->currently_selected_button);
  if (selected) {
    gtk_custom_menu_item_expose_button(widget, event, selected);
  }

  return TRUE;
}

static void gtk_custom_menu_item_select(GtkItem* item) {
  GtkCustomMenuItem* custom_item = GTK_CUSTOM_MENU_ITEM(item);

  // When we are selected, the only thing we do is clear information from
  // previous selections. Actual selection of a button is done either in the
  // "mouse-motion-event" or is manually set from GtkCustomMenu's overridden
  // "move-current" handler.
  custom_item->previously_selected_button = NULL;

  gtk_widget_queue_draw(GTK_WIDGET(item));
}

static void gtk_custom_menu_item_deselect(GtkItem* item) {
  GtkCustomMenuItem* custom_item = GTK_CUSTOM_MENU_ITEM(item);

  // When we are deselected, we store the item that was currently selected so
  // that it can be acted on. Menu items are first deselected before they are
  // activated.
  custom_item->previously_selected_button =
      custom_item->currently_selected_button;
  if (custom_item->currently_selected_button)
    set_selected(custom_item, NULL);

  gtk_widget_queue_draw(GTK_WIDGET(item));
}

static void gtk_custom_menu_item_activate(GtkMenuItem* menu_item) {
  GtkCustomMenuItem* custom_item = GTK_CUSTOM_MENU_ITEM(menu_item);

  // We look at |previously_selected_button| because by the time we've been
  // activated, we've already gone through our deselect handler.
  if (custom_item->previously_selected_button) {
    gpointer id_ptr = g_object_get_data(
        G_OBJECT(custom_item->previously_selected_button), "command-id");
    if (id_ptr != NULL) {
      int command_id = GPOINTER_TO_INT(id_ptr);
      g_signal_emit(custom_item, custom_menu_item_signals[BUTTON_PUSHED], 0,
                    command_id);
      set_selected(custom_item, NULL);
    }
  }
}

GtkWidget* gtk_custom_menu_item_new(const char* title) {
  GtkCustomMenuItem* item = GTK_CUSTOM_MENU_ITEM(
      g_object_new(GTK_TYPE_CUSTOM_MENU_ITEM, NULL));
  gtk_label_set_text(GTK_LABEL(item->label), title);
  return GTK_WIDGET(item);
}

GtkWidget* gtk_custom_menu_item_add_button(GtkCustomMenuItem* menu_item,
                                           int command_id) {
  GtkWidget* button = gtk_button_new();
  g_object_set_data(G_OBJECT(button), "command-id",
                    GINT_TO_POINTER(command_id));
  gtk_box_pack_start(GTK_BOX(menu_item->hbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  menu_item->all_widgets = g_list_append(menu_item->all_widgets, button);
  menu_item->button_widgets = g_list_append(menu_item->button_widgets, button);

  return button;
}

GtkWidget* gtk_custom_menu_item_add_button_label(GtkCustomMenuItem* menu_item,
                                                 int command_id) {
  GtkWidget* button = gtk_button_new_with_label("");
  g_object_set_data(G_OBJECT(button), "command-id",
                    GINT_TO_POINTER(command_id));
  gtk_box_pack_start(GTK_BOX(menu_item->hbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "notify::label",
                   G_CALLBACK(on_button_label_set), NULL);
  gtk_widget_show(button);

  menu_item->all_widgets = g_list_append(menu_item->all_widgets, button);

  return button;
}

void gtk_custom_menu_item_add_space(GtkCustomMenuItem* menu_item) {
  GtkWidget* fixed = gtk_fixed_new();
  gtk_widget_set_size_request(fixed, 5, -1);

  gtk_box_pack_start(GTK_BOX(menu_item->hbox), fixed, FALSE, FALSE, 0);
  gtk_widget_show(fixed);

  menu_item->all_widgets = g_list_append(menu_item->all_widgets, fixed);
}

void gtk_custom_menu_item_receive_motion_event(GtkCustomMenuItem* menu_item,
                                               gdouble x, gdouble y) {
  GtkWidget* new_selected_widget = NULL;
  GList* current = menu_item->button_widgets;
  for (; current != NULL; current = current->next) {
    GtkWidget* current_widget = GTK_WIDGET(current->data);
    GtkAllocation alloc = current_widget->allocation;
    int offset_x, offset_y;
    gtk_widget_translate_coordinates(current_widget, GTK_WIDGET(menu_item),
                                     0, 0, &offset_x, &offset_y);
    if (x >= offset_x && x < (offset_x + alloc.width) &&
        y >= offset_y && y < (offset_y + alloc.height)) {
      new_selected_widget = current_widget;
      break;
    }
  }

  set_selected(menu_item, new_selected_widget);
}

gboolean gtk_custom_menu_item_handle_move(GtkCustomMenuItem* menu_item,
                                          GtkMenuDirectionType direction) {
  GtkWidget* current = menu_item->currently_selected_button;
  if (menu_item->button_widgets && current) {
    switch (direction) {
      case GTK_MENU_DIR_PREV: {
        if (g_list_first(menu_item->button_widgets)->data == current)
          return FALSE;

        set_selected(menu_item, GTK_WIDGET(g_list_previous(g_list_find(
            menu_item->button_widgets, current))->data));
        break;
      }
      case GTK_MENU_DIR_NEXT: {
        if (g_list_last(menu_item->button_widgets)->data == current)
          return FALSE;

        set_selected(menu_item, GTK_WIDGET(g_list_next(g_list_find(
            menu_item->button_widgets, current))->data));
        break;
      }
      default:
        break;
    }
  }

  return TRUE;
}

void gtk_custom_menu_item_select_item_by_direction(
    GtkCustomMenuItem* menu_item, GtkMenuDirectionType direction) {
  menu_item->previously_selected_button = NULL;

  // If we're just told to be selected by the menu system, select the first
  // item.
  if (menu_item->button_widgets) {
    switch (direction) {
      case GTK_MENU_DIR_PREV: {
        GtkWidget* last_button =
            GTK_WIDGET(g_list_last(menu_item->button_widgets)->data);
        if (last_button)
          set_selected(menu_item, last_button);
        break;
      }
      case GTK_MENU_DIR_NEXT: {
        GtkWidget* first_button =
            GTK_WIDGET(g_list_first(menu_item->button_widgets)->data);
        if (first_button)
          set_selected(menu_item, first_button);
        break;
      }
      default:
        break;
    }
  }

  gtk_widget_queue_draw(GTK_WIDGET(menu_item));
}

gboolean gtk_custom_menu_item_is_in_clickable_region(
    GtkCustomMenuItem* menu_item) {
  return menu_item->currently_selected_button != NULL;
}

gboolean gtk_custom_menu_item_try_no_dismiss_command(
    GtkCustomMenuItem* menu_item) {
  GtkCustomMenuItem* custom_item = GTK_CUSTOM_MENU_ITEM(menu_item);
  gboolean activated = TRUE;

  // We work with |currently_selected_button| instead of
  // |previously_selected_button| since we haven't been "deselect"ed yet.
  gpointer id_ptr = g_object_get_data(
      G_OBJECT(custom_item->currently_selected_button), "command-id");
  if (id_ptr != NULL) {
    int command_id = GPOINTER_TO_INT(id_ptr);
    g_signal_emit(custom_item, custom_menu_item_signals[TRY_BUTTON_PUSHED], 0,
                  command_id, &activated);
  }

  return activated;
}

void gtk_custom_menu_item_foreach_button(GtkCustomMenuItem* menu_item,
                                         GtkCallback callback,
                                         gpointer callback_data) {
  // Even though we're filtering |all_widgets| on GTK_IS_BUTTON(), this isn't
  // equivalent to |button_widgets| because we also want the button-labels.
  for (GList* i = menu_item->all_widgets; i && GTK_IS_BUTTON(i->data);
       i = g_list_next(i)) {
    if (GTK_IS_BUTTON(i->data)) {
      callback(GTK_WIDGET(i->data), callback_data);
    }
  }
}
