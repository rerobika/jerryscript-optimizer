/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "bytecode.h"

namespace optimizer {
Bytecode::Bytecode(ecma_value_t function) {
  assert(ecma_is_value_object(function));

  function_ = ecma_get_object_from_value(function);
  assert(ecma_get_object_type(function_) == ECMA_OBJECT_TYPE_FUNCTION);
  auto ext_func = reinterpret_cast<ecma_extended_object_t *>(function_);
  compiled_code_ = const_cast<ecma_compiled_code_t *>(
      ecma_op_function_get_compiled_code(ext_func));
};

Bytecode::~Bytecode() { ecma_deref_object(function_); };

} // namespace optimizer
