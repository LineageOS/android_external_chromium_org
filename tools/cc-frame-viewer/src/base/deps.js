// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * WARNING: This file is generated by calcdeps.py
 *
 *        Do not edit directly.
 */
base.addModuleStylesheet(
    'base.unittest',
    'base.unittest');
base.addModuleRawScriptDependency(
    'base.gl_matrix',
    '../third_party/gl-matrix/src/gl-matrix/common.js');
base.addModuleRawScriptDependency(
    'base.gl_matrix',
    '../third_party/gl-matrix/src/gl-matrix/mat2d.js');
base.addModuleRawScriptDependency(
    'base.gl_matrix',
    '../third_party/gl-matrix/src/gl-matrix/vec2.js');
base.addModuleDependency(
    'base.rect2',
    'base.gl_matrix');
base.addModuleDependency(
    'base.bbox2',
    'base.gl_matrix');
base.addModuleDependency(
    'base.bbox2',
    'base.rect2');
base.addModuleDependency(
    'model.tile',
    'model.constants');
base.addModuleDependency(
    'model.layer_impl',
    'model.constants');
base.addModuleDependency(
    'model.layer_tree_impl',
    'base.bbox2');
base.addModuleDependency(
    'model.layer_tree_impl',
    'model.constants');
base.addModuleDependency(
    'model.layer_tree_impl',
    'model.layer_impl');
base.addModuleDependency(
    'model.layer_tree_host_impl',
    'base.bbox2');
base.addModuleDependency(
    'model.layer_tree_host_impl',
    'model.constants');
base.addModuleDependency(
    'model.layer_tree_host_impl',
    'model.tile');
base.addModuleDependency(
    'model.layer_tree_host_impl',
    'model.layer_tree_impl');
base.addModuleDependency(
    'model',
    'model.layer_tree_host_impl');
base.addModuleDependency(
    'ui.drag_handle',
    'ui');
base.addModuleStylesheet(
    'ui.drag_handle',
    'ui.drag_handle');
base.addModuleDependency(
    'ui.list_view',
    'ui');
base.addModuleStylesheet(
    'ui.list_view',
    'ui.list_view');
base.addModuleDependency(
    'tile_view',
    'ui');
base.addModuleStylesheet(
    'tile_view',
    'tile_view');
base.addModuleDependency(
    'ui.list_and_associated_view',
    'ui');
base.addModuleDependency(
    'ui.list_and_associated_view',
    'ui.list_view');
base.addModuleStylesheet(
    'ui.list_and_associated_view',
    'ui.list_and_associated_view');
base.addModuleDependency(
    'analysis_view',
    'tile_view');
base.addModuleDependency(
    'analysis_view',
    'ui');
base.addModuleDependency(
    'analysis_view',
    'ui.list_and_associated_view');
base.addModuleStylesheet(
    'analysis_view',
    'analysis_view');
base.addModuleDependency(
    'quad_view_viewport',
    'base.range');
base.addModuleDependency(
    'quad_view_viewport',
    'ui.event_target');
base.addModuleDependency(
    'color_mappings',
    'base.color');
base.addModuleDependency(
    'color_mappings',
    'model.constants');
base.addModuleDependency(
    'quad_view',
    'ui');
base.addModuleDependency(
    'quad_view',
    'quad_view_viewport');
base.addModuleStylesheet(
    'quad_view',
    'quad_view');
base.addModuleDependency(
    'tree_quad_view',
    'ui');
base.addModuleDependency(
    'tree_quad_view',
    'color_mappings');
base.addModuleDependency(
    'tree_quad_view',
    'quad_view');
base.addModuleDependency(
    'layer_impl_view',
    'ui');
base.addModuleStylesheet(
    'layer_impl_view',
    'layer_impl_view');
base.addModuleDependency(
    'layer_tree_impl_view',
    'ui');
base.addModuleDependency(
    'layer_tree_impl_view',
    'ui.list_and_associated_view');
base.addModuleDependency(
    'layer_tree_impl_view',
    'layer_impl_view');
base.addModuleStylesheet(
    'layer_tree_impl_view',
    'layer_tree_impl_view');
base.addModuleDependency(
    'lthi_view',
    'ui');
base.addModuleDependency(
    'lthi_view',
    'ui.list_view');
base.addModuleDependency(
    'lthi_view',
    'quad_view_viewport');
base.addModuleDependency(
    'lthi_view',
    'tree_quad_view');
base.addModuleDependency(
    'lthi_view',
    'layer_tree_impl_view');
base.addModuleStylesheet(
    'lthi_view',
    'lthi_view');
base.addModuleDependency(
    'model_view',
    'ui');
base.addModuleDependency(
    'model_view',
    'model');
base.addModuleDependency(
    'model_view',
    'ui.drag_handle');
base.addModuleDependency(
    'model_view',
    'ui.list_view');
base.addModuleDependency(
    'model_view',
    'analysis_view');
base.addModuleDependency(
    'model_view',
    'lthi_view');
base.addModuleStylesheet(
    'model_view',
    'model_view');
