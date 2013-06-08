# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'conditions': [
    ['use_system_mesa==0', {
      'target_defaults': {
        'conditions': [
          ['OS!="win"', {
            'defines': [
              # For talloc
              'HAVE_VA_COPY',
            ],
          }],
          ['OS!="mac"', {
            'defines': [
              # For talloc
              'HAVE_STRNLEN',
            ],
          }],
          ['os_posix == 1 and OS != "mac" and OS != "android"', {
            'cflags': [
              '-fPIC',
            ],
          }],
        ],
        'defines': [
          # For Mesa
          'MAPI_GLAPI_CURRENT',
        ],
      },
      'targets': [
        {
          'target_name': 'mesa_headers',
          'type': 'none',
          'direct_dependent_settings': {
            'include_dirs': [
              'src/include',
            ],
          },
        },
        {
          'target_name': 'mesa',
          'type': 'static_library',
          'include_dirs': [
            '../talloc',
            'src/src/glsl',
            'src/src/mapi',
            'src/src/mesa',
            'src/src/mesa/main',
          ],
          'dependencies': [
            'mesa_headers',
          ],
          # TODO(scottmg): http://crbug.com/143877 These should be removed if
          # Mesa is ever rolled and the warnings are fixed.
          'msvs_disabled_warnings': [
              4005, 4018, 4065, 4090, 4099, 4113, 4133, 4146, 4267, 4273, 4291,
              4305, 4334, 4748,
          ],
          'sources': [
            '../talloc/talloc.c',
            'src/src/glsl/ast.h',
            'src/src/glsl/ast_expr.cpp',
            'src/src/glsl/ast_function.cpp',
            'src/src/glsl/ast_to_hir.cpp',
            'src/src/glsl/ast_type.cpp',
            'src/src/glsl/builtin_function.cpp',
            'src/src/glsl/builtin_types.h',
            'src/src/glsl/builtin_variables.h',
            'src/src/glsl/glsl_lexer.cpp',
            'src/src/glsl/glsl_parser.cpp',
            'src/src/glsl/glsl_parser.h',
            'src/src/glsl/glsl_parser_extras.cpp',
            'src/src/glsl/glsl_parser_extras.h',
            'src/src/glsl/glsl_symbol_table.cpp',
            'src/src/glsl/glsl_symbol_table.h',
            'src/src/glsl/glsl_types.cpp',
            'src/src/glsl/glsl_types.h',
            'src/src/glsl/hir_field_selection.cpp',
            'src/src/glsl/ir.cpp',
            'src/src/glsl/ir.h',
            'src/src/glsl/ir_algebraic.cpp',
            'src/src/glsl/ir_basic_block.cpp',
            'src/src/glsl/ir_basic_block.h',
            'src/src/glsl/ir_clone.cpp',
            'src/src/glsl/ir_constant_expression.cpp',
            'src/src/glsl/ir_constant_folding.cpp',
            'src/src/glsl/ir_constant_propagation.cpp',
            'src/src/glsl/ir_constant_variable.cpp',
            'src/src/glsl/ir_copy_propagation.cpp',
            'src/src/glsl/ir_dead_code.cpp',
            'src/src/glsl/ir_dead_code_local.cpp',
            'src/src/glsl/ir_dead_functions.cpp',
            'src/src/glsl/ir_div_to_mul_rcp.cpp',
            'src/src/glsl/ir_explog_to_explog2.cpp',
            'src/src/glsl/ir_expression_flattening.cpp',
            'src/src/glsl/ir_expression_flattening.h',
            'src/src/glsl/ir_function.cpp',
            'src/src/glsl/ir_function_can_inline.cpp',
            'src/src/glsl/ir_function_inlining.cpp',
            'src/src/glsl/ir_function_inlining.h',
            'src/src/glsl/ir_hierarchical_visitor.cpp',
            'src/src/glsl/ir_hierarchical_visitor.h',
            'src/src/glsl/ir_hv_accept.cpp',
            'src/src/glsl/ir_if_simplification.cpp',
            'src/src/glsl/ir_if_to_cond_assign.cpp',
            'src/src/glsl/ir_import_prototypes.cpp',
            'src/src/glsl/ir_lower_jumps.cpp',
            'src/src/glsl/ir_mat_op_to_vec.cpp',
            'src/src/glsl/ir_mod_to_fract.cpp',
            'src/src/glsl/ir_noop_swizzle.cpp',
            'src/src/glsl/ir_optimization.h',
            'src/src/glsl/ir_print_visitor.cpp',
            'src/src/glsl/ir_print_visitor.h',
            'src/src/glsl/ir_reader.cpp',
            'src/src/glsl/ir_reader.h',
            'src/src/glsl/ir_rvalue_visitor.cpp',
            'src/src/glsl/ir_rvalue_visitor.h',
            'src/src/glsl/ir_set_program_inouts.cpp',
            'src/src/glsl/ir_structure_splitting.cpp',
            'src/src/glsl/ir_sub_to_add_neg.cpp',
            'src/src/glsl/ir_swizzle_swizzle.cpp',
            'src/src/glsl/ir_tree_grafting.cpp',
            'src/src/glsl/ir_validate.cpp',
            'src/src/glsl/ir_variable.cpp',
            'src/src/glsl/ir_variable_refcount.cpp',
            'src/src/glsl/ir_variable_refcount.h',
            'src/src/glsl/ir_vec_index_to_cond_assign.cpp',
            'src/src/glsl/ir_vec_index_to_swizzle.cpp',
            'src/src/glsl/ir_visitor.h',
            'src/src/glsl/link_functions.cpp',
            'src/src/glsl/linker.cpp',
            'src/src/glsl/linker.h',
            'src/src/glsl/list.h',
            'src/src/glsl/loop_analysis.cpp',
            'src/src/glsl/loop_analysis.h',
            'src/src/glsl/loop_controls.cpp',
            'src/src/glsl/loop_unroll.cpp',
            'src/src/glsl/lower_noise.cpp',
            'src/src/glsl/lower_variable_index_to_cond_assign.cpp',
            'src/src/glsl/opt_redundant_jumps.cpp',
            'src/src/glsl/program.h',
            'src/src/glsl/s_expression.cpp',
            'src/src/glsl/s_expression.h',
            'src/src/glsl/safe_strcmp.c',
            'src/src/glsl/safe_strcmp.h',
            'src/src/glsl/glcpp/glcpp-lex.c',
            'src/src/glsl/glcpp/glcpp-parse.c',
            'src/src/glsl/glcpp/glcpp-parse.h',
            'src/src/glsl/glcpp/pp.c',
            'src/src/mapi/glapi/glapi.h',
            'src/src/mapi/glapi/glapi_dispatch.c',
            'src/src/mapi/glapi/glapi_entrypoint.c',
            'src/src/mapi/glapi/glapi_getproc.c',
            'src/src/mapi/glapi/glapi_nop.c',
            'src/src/mapi/glapi/glapi_priv.h',
            'src/src/mapi/glapi/glapidispatch.h',
            'src/src/mapi/glapi/glapioffsets.h',
            'src/src/mapi/glapi/glapitable.h',
            'src/src/mapi/glapi/glapitemp.h',
            'src/src/mapi/glapi/glprocs.h',
            'src/src/mapi/mapi/u_compiler.h',
            'src/src/mapi/mapi/u_current.c',
            'src/src/mapi/mapi/u_current.h',
            'src/src/mapi/mapi/u_execmem.c',
            'src/src/mapi/mapi/u_execmem.h',
            'src/src/mapi/mapi/u_macros.h',
            'src/src/mapi/mapi/u_thread.c',
            'src/src/mapi/mapi/u_thread.h',
            'src/src/mesa/main/accum.c',
            'src/src/mesa/main/accum.h',
            'src/src/mesa/main/api_arrayelt.c',
            'src/src/mesa/main/api_arrayelt.h',
            'src/src/mesa/main/api_exec.c',
            'src/src/mesa/main/api_exec.h',
            'src/src/mesa/main/api_loopback.c',
            'src/src/mesa/main/api_loopback.h',
            'src/src/mesa/main/api_noop.c',
            'src/src/mesa/main/api_noop.h',
            'src/src/mesa/main/api_validate.c',
            'src/src/mesa/main/api_validate.h',
            'src/src/mesa/main/arbprogram.c',
            'src/src/mesa/main/arbprogram.h',
            'src/src/mesa/main/arrayobj.c',
            'src/src/mesa/main/arrayobj.h',
            'src/src/mesa/main/atifragshader.c',
            'src/src/mesa/main/atifragshader.h',
            'src/src/mesa/main/attrib.c',
            'src/src/mesa/main/attrib.h',
            'src/src/mesa/main/bitset.h',
            'src/src/mesa/main/blend.c',
            'src/src/mesa/main/blend.h',
            'src/src/mesa/main/bufferobj.c',
            'src/src/mesa/main/bufferobj.h',
            'src/src/mesa/main/buffers.c',
            'src/src/mesa/main/buffers.h',
            'src/src/mesa/main/clear.c',
            'src/src/mesa/main/clear.h',
            'src/src/mesa/main/clip.c',
            'src/src/mesa/main/clip.h',
            'src/src/mesa/main/colormac.h',
            'src/src/mesa/main/colortab.c',
            'src/src/mesa/main/colortab.h',
            'src/src/mesa/main/compiler.h',
            'src/src/mesa/main/condrender.c',
            'src/src/mesa/main/condrender.h',
            'src/src/mesa/main/config.h',
            'src/src/mesa/main/context.c',
            'src/src/mesa/main/context.h',
            'src/src/mesa/main/convolve.c',
            'src/src/mesa/main/convolve.h',
            'src/src/mesa/main/core.h',
            'src/src/mesa/main/cpuinfo.c',
            'src/src/mesa/main/cpuinfo.h',
            'src/src/mesa/main/dd.h',
            'src/src/mesa/main/debug.c',
            'src/src/mesa/main/debug.h',
            'src/src/mesa/main/depth.c',
            'src/src/mesa/main/depth.h',
            'src/src/mesa/main/depthstencil.c',
            'src/src/mesa/main/depthstencil.h',
            'src/src/mesa/main/dispatch.h',
            'src/src/mesa/main/dlist.c',
            'src/src/mesa/main/dlist.h',
            'src/src/mesa/main/dlopen.c',
            'src/src/mesa/main/dlopen.h',
            'src/src/mesa/main/drawpix.c',
            'src/src/mesa/main/drawpix.h',
            'src/src/mesa/main/drawtex.c',
            'src/src/mesa/main/drawtex.h',
            'src/src/mesa/main/enable.c',
            'src/src/mesa/main/enable.h',
            'src/src/mesa/main/enums.c',
            'src/src/mesa/main/enums.h',
            'src/src/mesa/main/eval.c',
            'src/src/mesa/main/eval.h',
            'src/src/mesa/main/execmem.c',
            'src/src/mesa/main/extensions.c',
            'src/src/mesa/main/extensions.h',
            'src/src/mesa/main/fbobject.c',
            'src/src/mesa/main/fbobject.h',
            'src/src/mesa/main/feedback.c',
            'src/src/mesa/main/feedback.h',
            'src/src/mesa/main/ffvertex_prog.c',
            'src/src/mesa/main/ffvertex_prog.h',
            'src/src/mesa/main/fog.c',
            'src/src/mesa/main/fog.h',
            'src/src/mesa/main/formats.c',
            'src/src/mesa/main/formats.h',
            'src/src/mesa/main/framebuffer.c',
            'src/src/mesa/main/framebuffer.h',
            'src/src/mesa/main/get.c',
            'src/src/mesa/main/get.h',
            'src/src/mesa/main/getstring.c',
            'src/src/mesa/main/glheader.h',
            'src/src/mesa/main/hash.c',
            'src/src/mesa/main/hash.h',
            'src/src/mesa/main/hint.c',
            'src/src/mesa/main/hint.h',
            'src/src/mesa/main/histogram.c',
            'src/src/mesa/main/histogram.h',
            'src/src/mesa/main/image.c',
            'src/src/mesa/main/image.h',
            'src/src/mesa/main/imports.c',
            'src/src/mesa/main/imports.h',
            'src/src/mesa/main/light.c',
            'src/src/mesa/main/light.h',
            'src/src/mesa/main/lines.c',
            'src/src/mesa/main/lines.h',
            'src/src/mesa/main/macros.h',
            'src/src/mesa/main/matrix.c',
            'src/src/mesa/main/matrix.h',
            'src/src/mesa/main/mfeatures.h',
            'src/src/mesa/main/mipmap.c',
            'src/src/mesa/main/mipmap.h',
            'src/src/mesa/main/mm.c',
            'src/src/mesa/main/mm.h',
            'src/src/mesa/main/mtypes.h',
            'src/src/mesa/main/multisample.c',
            'src/src/mesa/main/multisample.h',
            'src/src/mesa/main/nvprogram.c',
            'src/src/mesa/main/nvprogram.h',
            'src/src/mesa/main/pixel.c',
            'src/src/mesa/main/pixel.h',
            'src/src/mesa/main/pixelstore.c',
            'src/src/mesa/main/pixelstore.h',
            'src/src/mesa/main/points.c',
            'src/src/mesa/main/points.h',
            'src/src/mesa/main/polygon.c',
            'src/src/mesa/main/polygon.h',
            'src/src/mesa/main/queryobj.c',
            'src/src/mesa/main/queryobj.h',
            'src/src/mesa/main/rastpos.c',
            'src/src/mesa/main/rastpos.h',
            'src/src/mesa/main/readpix.c',
            'src/src/mesa/main/readpix.h',
            'src/src/mesa/main/remap.c',
            'src/src/mesa/main/remap.h',
            'src/src/mesa/main/remap_helper.h',
            'src/src/mesa/main/renderbuffer.c',
            'src/src/mesa/main/renderbuffer.h',
            'src/src/mesa/main/scissor.c',
            'src/src/mesa/main/scissor.h',
            'src/src/mesa/main/shaderapi.c',
            'src/src/mesa/main/shaderapi.h',
            'src/src/mesa/main/shaderobj.c',
            'src/src/mesa/main/shaderobj.h',
            'src/src/mesa/main/shared.c',
            'src/src/mesa/main/shared.h',
            'src/src/mesa/main/simple_list.h',
            'src/src/mesa/main/state.c',
            'src/src/mesa/main/state.h',
            'src/src/mesa/main/stencil.c',
            'src/src/mesa/main/stencil.h',
            'src/src/mesa/main/syncobj.c',
            'src/src/mesa/main/syncobj.h',
            'src/src/mesa/main/texcompress.c',
            'src/src/mesa/main/texcompress.h',
            'src/src/mesa/main/texcompress_fxt1.c',
            'src/src/mesa/main/texcompress_fxt1.h',
            'src/src/mesa/main/texcompress_s3tc.c',
            'src/src/mesa/main/texcompress_s3tc.h',
            'src/src/mesa/main/texenv.c',
            'src/src/mesa/main/texenv.h',
            'src/src/mesa/main/texenvprogram.c',
            'src/src/mesa/main/texenvprogram.h',
            'src/src/mesa/main/texfetch.c',
            'src/src/mesa/main/texfetch.h',
            'src/src/mesa/main/texfetch_tmp.h',
            'src/src/mesa/main/texformat.c',
            'src/src/mesa/main/texformat.h',
            'src/src/mesa/main/texgen.c',
            'src/src/mesa/main/texgen.h',
            'src/src/mesa/main/texgetimage.c',
            'src/src/mesa/main/texgetimage.h',
            'src/src/mesa/main/teximage.c',
            'src/src/mesa/main/teximage.h',
            'src/src/mesa/main/texobj.c',
            'src/src/mesa/main/texobj.h',
            'src/src/mesa/main/texpal.c',
            'src/src/mesa/main/texpal.h',
            'src/src/mesa/main/texparam.c',
            'src/src/mesa/main/texparam.h',
            'src/src/mesa/main/texrender.c',
            'src/src/mesa/main/texrender.h',
            'src/src/mesa/main/texstate.c',
            'src/src/mesa/main/texstate.h',
            'src/src/mesa/main/texstore.c',
            'src/src/mesa/main/texstore.h',
            'src/src/mesa/main/transformfeedback.c',
            'src/src/mesa/main/transformfeedback.h',
            'src/src/mesa/main/uniforms.c',
            'src/src/mesa/main/uniforms.h',
            'src/src/mesa/main/varray.c',
            'src/src/mesa/main/varray.h',
            'src/src/mesa/main/version.c',
            'src/src/mesa/main/version.h',
            'src/src/mesa/main/viewport.c',
            'src/src/mesa/main/viewport.h',
            'src/src/mesa/main/vtxfmt.c',
            'src/src/mesa/main/vtxfmt.h',
            'src/src/mesa/main/vtxfmt_tmp.h',
            'src/src/mesa/math/m_clip_tmp.h',
            'src/src/mesa/math/m_copy_tmp.h',
            'src/src/mesa/math/m_debug.h',
            'src/src/mesa/math/m_debug_clip.c',
            'src/src/mesa/math/m_debug_norm.c',
            'src/src/mesa/math/m_debug_util.h',
            'src/src/mesa/math/m_debug_xform.c',
            'src/src/mesa/math/m_dotprod_tmp.h',
            'src/src/mesa/math/m_eval.c',
            'src/src/mesa/math/m_eval.h',
            'src/src/mesa/math/m_matrix.c',
            'src/src/mesa/math/m_matrix.h',
            'src/src/mesa/math/m_norm_tmp.h',
            'src/src/mesa/math/m_trans_tmp.h',
            'src/src/mesa/math/m_translate.c',
            'src/src/mesa/math/m_translate.h',
            'src/src/mesa/math/m_vector.c',
            'src/src/mesa/math/m_vector.h',
            'src/src/mesa/math/m_xform.c',
            'src/src/mesa/math/m_xform.h',
            'src/src/mesa/math/m_xform_tmp.h',
            'src/src/mesa/program/arbprogparse.c',
            'src/src/mesa/program/arbprogparse.h',
            'src/src/mesa/program/hash_table.c',
            'src/src/mesa/program/hash_table.h',
            'src/src/mesa/program/ir_to_mesa.cpp',
            'src/src/mesa/program/ir_to_mesa.h',
            'src/src/mesa/program/lex.yy.c',
            'src/src/mesa/program/nvfragparse.c',
            'src/src/mesa/program/nvfragparse.h',
            'src/src/mesa/program/nvvertparse.c',
            'src/src/mesa/program/nvvertparse.h',
            'src/src/mesa/program/prog_cache.c',
            'src/src/mesa/program/prog_cache.h',
            'src/src/mesa/program/prog_execute.c',
            'src/src/mesa/program/prog_execute.h',
            'src/src/mesa/program/prog_instruction.c',
            'src/src/mesa/program/prog_instruction.h',
            'src/src/mesa/program/prog_noise.c',
            'src/src/mesa/program/prog_noise.h',
            'src/src/mesa/program/prog_optimize.c',
            'src/src/mesa/program/prog_optimize.h',
            'src/src/mesa/program/prog_parameter.c',
            'src/src/mesa/program/prog_parameter.h',
            'src/src/mesa/program/prog_parameter_layout.c',
            'src/src/mesa/program/prog_parameter_layout.h',
            'src/src/mesa/program/prog_print.c',
            'src/src/mesa/program/prog_print.h',
            'src/src/mesa/program/prog_statevars.c',
            'src/src/mesa/program/prog_statevars.h',
            'src/src/mesa/program/prog_uniform.c',
            'src/src/mesa/program/prog_uniform.h',
            'src/src/mesa/program/program.c',
            'src/src/mesa/program/program.h',
            'src/src/mesa/program/program_parse.tab.c',
            'src/src/mesa/program/program_parse.tab.h',
            'src/src/mesa/program/program_parse_extra.c',
            'src/src/mesa/program/program_parser.h',
            'src/src/mesa/program/programopt.c',
            'src/src/mesa/program/programopt.h',
            'src/src/mesa/program/symbol_table.c',
            'src/src/mesa/program/symbol_table.h',
            'src/src/mesa/swrast/s_aaline.c',
            'src/src/mesa/swrast/s_aaline.h',
            'src/src/mesa/swrast/s_aalinetemp.h',
            'src/src/mesa/swrast/s_aatriangle.c',
            'src/src/mesa/swrast/s_aatriangle.h',
            'src/src/mesa/swrast/s_aatritemp.h',
            'src/src/mesa/swrast/s_accum.c',
            'src/src/mesa/swrast/s_accum.h',
            'src/src/mesa/swrast/s_alpha.c',
            'src/src/mesa/swrast/s_alpha.h',
            'src/src/mesa/swrast/s_atifragshader.c',
            'src/src/mesa/swrast/s_atifragshader.h',
            'src/src/mesa/swrast/s_bitmap.c',
            'src/src/mesa/swrast/s_blend.c',
            'src/src/mesa/swrast/s_blend.h',
            'src/src/mesa/swrast/s_blit.c',
            'src/src/mesa/swrast/s_clear.c',
            'src/src/mesa/swrast/s_context.c',
            'src/src/mesa/swrast/s_context.h',
            'src/src/mesa/swrast/s_copypix.c',
            'src/src/mesa/swrast/s_depth.c',
            'src/src/mesa/swrast/s_depth.h',
            'src/src/mesa/swrast/s_drawpix.c',
            'src/src/mesa/swrast/s_feedback.c',
            'src/src/mesa/swrast/s_feedback.h',
            'src/src/mesa/swrast/s_fog.c',
            'src/src/mesa/swrast/s_fog.h',
            'src/src/mesa/swrast/s_fragprog.c',
            'src/src/mesa/swrast/s_fragprog.h',
            'src/src/mesa/swrast/s_lines.c',
            'src/src/mesa/swrast/s_lines.h',
            'src/src/mesa/swrast/s_linetemp.h',
            'src/src/mesa/swrast/s_logic.c',
            'src/src/mesa/swrast/s_logic.h',
            'src/src/mesa/swrast/s_masking.c',
            'src/src/mesa/swrast/s_masking.h',
            'src/src/mesa/swrast/s_points.c',
            'src/src/mesa/swrast/s_points.h',
            'src/src/mesa/swrast/s_readpix.c',
            'src/src/mesa/swrast/s_span.c',
            'src/src/mesa/swrast/s_span.h',
            'src/src/mesa/swrast/s_spantemp.h',
            'src/src/mesa/swrast/s_stencil.c',
            'src/src/mesa/swrast/s_stencil.h',
            'src/src/mesa/swrast/s_texcombine.c',
            'src/src/mesa/swrast/s_texcombine.h',
            'src/src/mesa/swrast/s_texfilter.c',
            'src/src/mesa/swrast/s_texfilter.h',
            'src/src/mesa/swrast/s_triangle.c',
            'src/src/mesa/swrast/s_triangle.h',
            'src/src/mesa/swrast/s_trispan.h',
            'src/src/mesa/swrast/s_tritemp.h',
            'src/src/mesa/swrast/s_zoom.c',
            'src/src/mesa/swrast/s_zoom.h',
            'src/src/mesa/swrast/swrast.h',
            'src/src/mesa/swrast_setup/ss_context.c',
            'src/src/mesa/swrast_setup/ss_context.h',
            'src/src/mesa/swrast_setup/ss_triangle.c',
            'src/src/mesa/swrast_setup/ss_triangle.h',
            'src/src/mesa/swrast_setup/ss_tritmp.h',
            'src/src/mesa/swrast_setup/ss_vb.h',
            'src/src/mesa/swrast_setup/swrast_setup.h',
            'src/src/mesa/tnl/t_context.c',
            'src/src/mesa/tnl/t_context.h',
            'src/src/mesa/tnl/t_draw.c',
            'src/src/mesa/tnl/t_pipeline.c',
            'src/src/mesa/tnl/t_pipeline.h',
            'src/src/mesa/tnl/t_rasterpos.c',
            'src/src/mesa/tnl/t_vb_cliptmp.h',
            'src/src/mesa/tnl/t_vb_cull.c',
            'src/src/mesa/tnl/t_vb_fog.c',
            'src/src/mesa/tnl/t_vb_light.c',
            'src/src/mesa/tnl/t_vb_lighttmp.h',
            'src/src/mesa/tnl/t_vb_normals.c',
            'src/src/mesa/tnl/t_vb_points.c',
            'src/src/mesa/tnl/t_vb_program.c',
            'src/src/mesa/tnl/t_vb_render.c',
            'src/src/mesa/tnl/t_vb_rendertmp.h',
            'src/src/mesa/tnl/t_vb_texgen.c',
            'src/src/mesa/tnl/t_vb_texmat.c',
            'src/src/mesa/tnl/t_vb_vertex.c',
            'src/src/mesa/tnl/t_vertex.c',
            'src/src/mesa/tnl/t_vertex.h',
            'src/src/mesa/tnl/t_vertex_generic.c',
            'src/src/mesa/tnl/t_vertex_sse.c',
            'src/src/mesa/tnl/t_vp_build.c',
            'src/src/mesa/tnl/t_vp_build.h',
            'src/src/mesa/tnl/tnl.h',
            'src/src/mesa/vbo/vbo.h',
            'src/src/mesa/vbo/vbo_attrib.h',
            'src/src/mesa/vbo/vbo_attrib_tmp.h',
            'src/src/mesa/vbo/vbo_context.c',
            'src/src/mesa/vbo/vbo_context.h',
            'src/src/mesa/vbo/vbo_exec.c',
            'src/src/mesa/vbo/vbo_exec.h',
            'src/src/mesa/vbo/vbo_exec_api.c',
            'src/src/mesa/vbo/vbo_exec_array.c',
            'src/src/mesa/vbo/vbo_exec_draw.c',
            'src/src/mesa/vbo/vbo_exec_eval.c',
            'src/src/mesa/vbo/vbo_rebase.c',
            'src/src/mesa/vbo/vbo_save.c',
            'src/src/mesa/vbo/vbo_save.h',
            'src/src/mesa/vbo/vbo_save_api.c',
            'src/src/mesa/vbo/vbo_save_draw.c',
            'src/src/mesa/vbo/vbo_save_loopback.c',
            'src/src/mesa/vbo/vbo_split.c',
            'src/src/mesa/vbo/vbo_split.h',
            'src/src/mesa/vbo/vbo_split_copy.c',
            'src/src/mesa/vbo/vbo_split_inplace.c',
          ],
          'conditions': [
            ['clang == 1', {
              'xcode_settings': {
                'WARNING_CFLAGS': [
                  # Several functions ignore the result of talloc_steal().
                  '-Wno-unused-value',
                  # texenvprogram.c converts '~0' to a bitfield, which causes clang
                  # to warn that -1 is implicitly converted to 255.
                  '-Wno-constant-conversion',
                ],
                'WARNING_CFLAGS!': [
                  # Don't warn about string->bool used in asserts.
                  '-Wstring-conversion',
                ],
              },
              'cflags': [
                '-Wno-unused-value',
                '-Wno-constant-conversion',
              ],
              'cflags!': [
                '-Wstring-conversion',
              ],
            }],
            ['OS=="android" and clang==0', {
              # Disable sincos() optimization to avoid a linker error
              # since Android's math library doesn't have sincos().
              # Either -fno-builtin-sin or -fno-builtin-cos works.
              'cflags': [
                '-fno-builtin-sin',
              ],
            }],
          ],
        },
        # Building this target will hide the native OpenGL shared library and
        # replace it with a slow software renderer.
        {
          'target_name': 'osmesa',
          'type': 'loadable_module',
          'mac_bundle': 0,
          'dependencies': [
            'mesa_headers',
            'mesa',
          ],
          # Fixes link problems on Mac OS X with missing __cxa_pure_virtual.
          'conditions': [
            ['OS=="mac"', {
              'sources': [
                'src/src/mesa/drivers/osmesa/empty.cpp',
              ],
            }],
          ],
          'include_dirs': [
            'src/src/mapi',
            'src/src/mesa',
            'src/src/mesa/drivers',
          ],
          # TODO(scottmg): http://crbug.com/143877 These should be removed if
          # Mesa is ever rolled and the warnings are fixed.
          'msvs_disabled_warnings': [
              4005, 4133, 4267,
          ],
          'sources': [
            'src/src/mesa/drivers/common/driverfuncs.c',
            'src/src/mesa/drivers/common/driverfuncs.h',
            'src/src/mesa/drivers/common/meta.c',
            'src/src/mesa/drivers/common/meta.h',
            'src/src/mesa/drivers/osmesa/osmesa.c',
            'src/src/mesa/drivers/osmesa/osmesa.def',
          ],
        },
      ],
    }, { # use_system_mesa==1
      'targets': [
        # TODO(phajdan.jr): Make this work, http://crbug.com/161389 .
        {
          'target_name': 'mesa_headers',
          'type': 'none',
          'variables': {
            'headers_root_path': 'src/include',
            # This list can easily be updated using the command below:
            # find third_party/mesa/src/include -iname '*.h' \
            # -printf "'%p',\n" | grep -v internal | sed -e \
            # 's|third_party/mesa/src/include/||' | sort -u
            'header_filenames': [
              'GL/glext.h',
              'GL/glfbdev.h',
              'GL/gl.h',
              'GL/gl_mangle.h',
              'GL/glu.h',
              'GL/glu_mangle.h',
              'GL/glxext.h',
              'GL/glx.h',
              'GL/glx_mangle.h',
              'GL/mesa_wgl.h',
              'GL/osmesa.h',
              'GL/vms_x_fix.h',
              'GL/wglext.h',
              'GL/wmesa.h',
            ],
          },
          'includes': [
            '../../build/shim_headers.gypi',
          ],
        },
        {
          'target_name': 'mesa',
          'type': 'none',
        },
        {
          'target_name': 'osmesa',
          'type': 'none',
        },
      ],
    }],
  ],
}
