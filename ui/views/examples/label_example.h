// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_
#define UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_

#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/examples/example_base.h"

namespace views {

class Checkbox;
class GridLayout;
class Label;

namespace examples {

class ExampleComboboxModel;

class VIEWS_EXAMPLES_EXPORT LabelExample : public ExampleBase,
                                           public ButtonListener,
                                           public ComboboxListener,
                                           public TextfieldController {
 public:
  LabelExample();
  virtual ~LabelExample();

  // ExampleBase:
  virtual void CreateExampleView(View* container) OVERRIDE;

  // ButtonListener:
  virtual void ButtonPressed(Button* button, const ui::Event& event) OVERRIDE;

  // ComboboxListener:
  virtual void OnPerformAction(Combobox* combobox) OVERRIDE;

  // TextfieldController:
  virtual void ContentsChanged(Textfield* sender,
                               const base::string16& new_contents) OVERRIDE;

 private:
   // Add a customizable label and various controls to modify its presentation.
   void AddCustomLabel(View* container);

   // Creates and adds a combobox to the layout.
   Combobox* AddCombobox(GridLayout* layout,
                         const char* name,
                         const char** strings,
                         int count);

  Textfield* textfield_;
  Combobox* alignment_;
  Combobox* elide_behavior_;
  ScopedVector<ExampleComboboxModel> example_combobox_models_;
  Checkbox* multiline_;
  Checkbox* shadows_;
  Label* custom_label_;

  DISALLOW_COPY_AND_ASSIGN(LabelExample);
};

}  // namespace examples
}  // namespace views

#endif  // UI_VIEWS_EXAMPLES_LABEL_EXAMPLE_H_
