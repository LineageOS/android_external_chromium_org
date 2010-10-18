// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/constrained_window_gtk.h"

#include <gdk/gdkkeysyms.h>

#include "chrome/browser/browser_list.h"
#include "chrome/browser/gtk/gtk_util.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_contents_view_gtk.h"

ConstrainedWindowGtkDelegate::~ConstrainedWindowGtkDelegate() {
}

bool ConstrainedWindowGtkDelegate::GetBackgroundColor(GdkColor* color) {
  return false;
}

ConstrainedWindowGtk::ConstrainedWindowGtk(
    TabContents* owner, ConstrainedWindowGtkDelegate* delegate)
    : owner_(owner),
      delegate_(delegate),
      visible_(false),
      factory_(this) {
  DCHECK(owner);
  DCHECK(delegate);
  GtkWidget* dialog = delegate->GetWidgetRoot();

  // Unlike other users of CreateBorderBin, we need a dedicated frame around
  // our "window".
  GtkWidget* ebox = gtk_event_box_new();
  GtkWidget* frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

  GtkWidget* alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
      gtk_util::kContentAreaBorder, gtk_util::kContentAreaBorder,
      gtk_util::kContentAreaBorder, gtk_util::kContentAreaBorder);
  GdkColor background;
  if (delegate->GetBackgroundColor(&background)) {
    gtk_widget_modify_base(ebox, GTK_STATE_NORMAL, &background);
    gtk_widget_modify_fg(ebox, GTK_STATE_NORMAL, &background);
    gtk_widget_modify_bg(ebox, GTK_STATE_NORMAL, &background);
  }

  gtk_container_add(GTK_CONTAINER(alignment), dialog);
  gtk_container_add(GTK_CONTAINER(frame), alignment);
  gtk_container_add(GTK_CONTAINER(ebox), frame);
  border_.Own(ebox);

  gtk_widget_add_events(widget(), GDK_KEY_PRESS_MASK);
  g_signal_connect(widget(), "key-press-event", G_CALLBACK(OnKeyPressThunk),
                   this);
}

ConstrainedWindowGtk::~ConstrainedWindowGtk() {
  border_.Destroy();
}

void ConstrainedWindowGtk::ShowConstrainedWindow() {
  gtk_widget_show_all(border_.get());

  // We collaborate with TabContentsViewGtk and stick ourselves in the
  // TabContentsViewGtk's floating container.
  ContainingView()->AttachConstrainedWindow(this);

  visible_ = true;
}

void ConstrainedWindowGtk::CloseConstrainedWindow() {
  if (visible_)
    ContainingView()->RemoveConstrainedWindow(this);
  delegate_->DeleteDelegate();
  owner_->WillClose(this);

  delete this;
}

TabContentsViewGtk* ConstrainedWindowGtk::ContainingView() {
  return static_cast<TabContentsViewGtk*>(owner_->view());
}

gboolean ConstrainedWindowGtk::OnKeyPress(GtkWidget* sender,
                                          GdkEventKey* key) {
  if (key->keyval == GDK_Escape) {
    // Let the stack unwind so the event handler can release its ref
    // on widget().
    MessageLoop::current()->PostTask(FROM_HERE,
        factory_.NewRunnableMethod(
            &ConstrainedWindowGtk::CloseConstrainedWindow));
    return TRUE;
  }

  return FALSE;
}

// static
ConstrainedWindow* ConstrainedWindow::CreateConstrainedDialog(
    TabContents* parent,
    ConstrainedWindowGtkDelegate* delegate) {
  return new ConstrainedWindowGtk(parent, delegate);
}
