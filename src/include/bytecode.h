/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef BYTECODE_H
#define BYTECODE_H

#include "common.h"
#ifdef __cplusplus
extern "C" {
#endif
#define this this_value
#include "ecma-function-object.h"
#undef this
#ifdef __cplusplus
}
#endif
namespace optimizer {

class Bytecode {
public:
  Bytecode(ecma_value_t function);
  ~Bytecode();

  auto compiled_code() const { return compiled_code_; }
  auto function() const { return function_; }

private:
  ecma_object_t *function_;
  ecma_compiled_code_t *compiled_code_;
};

} // namespace optimizer
#endif // BYTECODE_H
