// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/passwords/manage_password_item_view.h"

#include "grit/generated_resources.h"
#include "grit/ui_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"

ManagePasswordItemView::ManagePasswordItemView(
    ManagePasswordsBubbleModel* manage_passwords_bubble_model,
    autofill::PasswordForm password_form,
    int field_1_width,
    int field_2_width)
    : manage_passwords_bubble_model_(manage_passwords_bubble_model),
      password_form_(password_form),
      delete_password_(false),
      field_1_width_(field_1_width),
      field_2_width_(field_2_width) {
  views::GridLayout* layout = new views::GridLayout(this);
  ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
  SetLayoutManager(layout);

  const int column_set_save_id = 0;
  views::ColumnSet* column_set_save = layout->AddColumnSet(column_set_save_id);
  column_set_save->AddPaddingColumn(0, views::kItemLabelSpacing);
  column_set_save->AddColumn(
      views::GridLayout::FILL, views::GridLayout::FILL, 0,
      views::GridLayout::FIXED, field_1_width_, field_1_width_);
  column_set_save->AddPaddingColumn(0, views::kItemLabelSpacing);
  column_set_save->AddColumn(
      views::GridLayout::FILL, views::GridLayout::FILL, 1,
      views::GridLayout::USE_PREF, field_2_width_, field_2_width_);
  column_set_save->AddPaddingColumn(0, views::kItemLabelSpacing);

  const int column_set_manage_id = 1;
  views::ColumnSet* column_set_manage =
      layout->AddColumnSet(column_set_manage_id);
  column_set_manage->AddPaddingColumn(0, views::kItemLabelSpacing);
  column_set_manage->AddColumn(
      views::GridLayout::FILL, views::GridLayout::FILL, 0,
      views::GridLayout::FIXED, field_1_width_, field_1_width_);
  column_set_manage->AddPaddingColumn(0, views::kItemLabelSpacing);
  column_set_manage->AddColumn(
      views::GridLayout::FILL, views::GridLayout::FILL, 1,
      views::GridLayout::USE_PREF, field_2_width_, field_2_width_);
  column_set_manage->AddColumn(views::GridLayout::TRAILING,
      views::GridLayout::FILL, 0, views::GridLayout::USE_PREF, 0, 0);

  if (manage_passwords_bubble_model_->manage_passwords_bubble_state() !=
      ManagePasswordsBubbleModel::PASSWORD_TO_BE_SAVED)
    layout->StartRow(0, column_set_manage_id);
  else
    layout->StartRow(0, column_set_save_id);

  label_1_ = new views::Label(password_form_.username_value);
  label_1_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  label_2_ =
      new views::Label(GetPasswordDisplayString(password_form_.password_value));
  label_2_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  delete_or_undo_button_ = new views::LabelButton(this, string16());
  delete_or_undo_button_->SetStyle(views::Button::STYLE_TEXTBUTTON);
  delete_or_undo_button_->SetImage(views::Button::STATE_NORMAL,
                                   *rb->GetImageSkiaNamed(IDR_CLOSE_2));

  const int button_height = delete_or_undo_button_->GetPreferredSize().height();

  layout->AddView(label_1_, 1, 1,
                  views::GridLayout::FILL, views::GridLayout::FILL,
                  -1, button_height);
  layout->AddView(label_2_, 1, 1,
                  views::GridLayout::FILL, views::GridLayout::FILL,
                  -1, button_height);
  if (manage_passwords_bubble_model_->manage_passwords_bubble_state() !=
      ManagePasswordsBubbleModel::PASSWORD_TO_BE_SAVED) {
    layout->AddView(delete_or_undo_button_);
  }
}

// static
string16 ManagePasswordItemView::GetPasswordDisplayString(
    const string16& password) {
  const wchar_t password_bullet = 0x2022;
  const size_t max_password_char = 22;
  return string16(std::min(password.length(), max_password_char),
                  password_bullet);
}

ManagePasswordItemView::~ManagePasswordItemView() {
  if (delete_password_)
    manage_passwords_bubble_model_->DeleteFromBestMatches(password_form_);
}

void ManagePasswordItemView::Refresh() {
  ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
  if (delete_password_) {
    label_1_->SetText(l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_DELETED));
    label_2_->SetText(l10n_util::GetStringUTF16(IDS_MANAGE_PASSWORDS_UNDO));
    label_2_->SetHorizontalAlignment(gfx::ALIGN_RIGHT);
    delete_or_undo_button_->SetImage(
        views::Button::STATE_NORMAL,
        *rb->GetImageSkiaNamed(IDR_PASSWORD_UNDO_ARROW));
    manage_passwords_bubble_model_->PasswordAction(password_form_, true);
  } else {
    label_1_->SetText(password_form_.username_value);
    label_2_->SetText(GetPasswordDisplayString(password_form_.password_value));
    label_2_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    delete_or_undo_button_->SetImage(views::Button::STATE_NORMAL,
                                     *rb->GetImageSkiaNamed(IDR_CLOSE_2));
    manage_passwords_bubble_model_->PasswordAction(password_form_, false);
  }
}

void ManagePasswordItemView::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  DCHECK_EQ(delete_or_undo_button_, sender);
  delete_password_ = !delete_password_;
  Refresh();
}
