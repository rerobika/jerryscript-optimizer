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

class BytecodeFlags {
public:
  BytecodeFlags(){};

  auto flags() const { return flags_; }
  void setFlags(uint16_t flags) { flags_ = flags; }

  bool uint16Arguments() {
    return (flags_ & CBC_CODE_FLAGS_UINT16_ARGUMENTS) != 0;
  }

  bool fullLiteralEncoding() {
    return (flags_ & CBC_CODE_FLAGS_FULL_LITERAL_ENCODING) != 0;
  }

private:
  uint16_t flags_;
};

class BytecodeArguments {
public:
  BytecodeArguments()
      : argument_end_(0), register_end_(0), ident_end_(0),
        const_literal_end_(0), literal_end_(0) {}

  void setArguments(cbc_uint16_arguments_t *args) {
    argument_end_ = args->argument_end;
    register_end_ = args->register_end;
    ident_end_ = args->ident_end;
    const_literal_end_ = args->const_literal_end;
    literal_end_ = args->literal_end;
  }

  void setArguments(cbc_uint8_arguments_t *args) {
    argument_end_ = static_cast<uint16_t>(args->argument_end);
    register_end_ = static_cast<uint16_t>(args->register_end);
    ident_end_ = static_cast<uint16_t>(args->ident_end);
    const_literal_end_ = static_cast<uint16_t>(args->const_literal_end);
    literal_end_ = static_cast<uint16_t>(args->literal_end);
  }

  void setEncoding(uint16_t limit, uint16_t delta) {
    encoding_limit_ = limit;
    encoding_delta_ = delta;
  }

  auto argument_end() const { return argument_end_; }
  auto register_end() const { return register_end_; }
  auto ident_end() const { return ident_end_; }
  auto const_literal_end() const { return const_literal_end_; }
  auto literal_end() const { return literal_end_; }
  auto encoding_limit() const { return encoding_limit_; }
  auto encoding_delta() const { return encoding_delta_; }

private:
  uint16_t argument_end_;
  uint16_t register_end_;
  uint16_t ident_end_;
  uint16_t const_literal_end_;
  uint16_t literal_end_;
  uint16_t encoding_limit_;
  uint16_t encoding_delta_;
};

class LiteralPool {
public:
  LiteralPool() {}

  auto literal_start() const { return literal_start_; }
  auto size() const { return size_; }

  uint8_t *setLiteralPool(void *literal_start, BytecodeArguments &args) {
    literal_start_ = reinterpret_cast<ecma_value_t *>(literal_start);
    size_ = args.literal_end();

    return reinterpret_cast<uint8_t *>(literal_start_ + size_);
  }

private:
  ecma_value_t *literal_start_;
  uint16_t size_;
};

class Bytecode {
public:
  Bytecode(ecma_value_t function);
  ~Bytecode();

  auto compiled_code() const { return compiled_code_; }
  auto function() const { return function_; }
  auto flags() const { return flags_; }
  auto literal_pool() const { return literal_pool_; }

  void setArguments(cbc_uint16_arguments_t *args);
  void setArguments(cbc_uint8_arguments_t *args);
  void setEncoding();

private:
  void decodeHeader();
  void buildInstructions();

  ecma_object_t *function_;
  ecma_compiled_code_t *compiled_code_;
  uint8_t *byte_code_start_;
  BytecodeFlags flags_;
  BytecodeArguments args_;
  LiteralPool literal_pool_;
};

} // namespace optimizer
#endif // BYTECODE_H
