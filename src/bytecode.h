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
#include "stack.h"

extern "C" {
#include "ecma-function-object.h"
}

namespace optimizer {

class Ins;
class BasicBlock;
class LiveInterval;

using RegList = std::vector<uint32_t>;
using RegSet = std::unordered_set<uint32_t>;

using BasicBlockList = std::vector<BasicBlock *>;
using BasicBlockSet = std::unordered_set<BasicBlock *>;
using BasicBlockOrderedSet = std::set<BasicBlock *>;
using InsList = std::vector<Ins *>;
using OffsetMap = std::unordered_map<int32_t, Ins *>;
using LiteralIndex = uint16_t;
using BasicBlockID = uint32_t;

using LiveIntervalList = std::vector<LiveInterval *>;
using RegLiveInterval =
    std::pair<std::pair<uint32_t, uint32_t>, LiveInterval *>;
using RegLiveIntervalList = std::vector<RegLiveInterval>;
using LiveRangeMap = std::unordered_map<uint32_t, std::vector<LiveInterval *>>;

class BytecodeFlags {
public:
  BytecodeFlags(){};

  auto flags() const { return flags_; }
  void setFlags(uint16_t flags) { flags_ = flags; }

  bool isFunction() const { return CBC_IS_FUNCTION(flags()) != 0; }

  bool hasExtendedInfo() const {
    return (flags() & CBC_CODE_FLAGS_HAS_EXTENDED_INFO) != 0;
  }

  bool hasTaggedTemplateLiterals() const {
    return (flags() & CBC_CODE_FLAGS_HAS_TAGGED_LITERALS) != 0;
  }

  bool hasName() const {
    return CBC_FUNCTION_GET_TYPE(flags()) != CBC_FUNCTION_CONSTRUCTOR;
  }

  bool uint16Arguments() const {
    return (flags() & CBC_CODE_FLAGS_UINT16_ARGUMENTS) != 0;
  }

  bool fullLiteralEncoding() const {
    return (flags() & CBC_CODE_FLAGS_FULL_LITERAL_ENCODING) != 0;
  }

  bool mappedArgumentsNeeded() const {
    return (flags() & CBC_CODE_FLAGS_MAPPED_ARGUMENTS_NEEDED) != 0;
  }

private:
  uint16_t flags_;
};

class BytecodeArguments {
public:
  BytecodeArguments() = default;

  void setArguments(cbc_uint16_arguments_t *args) {
    argument_end_ = args->argument_end;
    register_end_ = args->register_end;
    ident_end_ = args->ident_end;
    const_literal_end_ = args->const_literal_end;
    literal_end_ = args->literal_end;
    stack_limit_ = args->stack_limit;
#if ENABLED(JERRY_BUILTIN_REALMS)
    realm_value_ = realm_value_;
#endif
    size_ = static_cast<uint16_t>(sizeof(cbc_uint16_arguments_t));
  }

  void setArguments(cbc_uint8_arguments_t *args) {
    argument_end_ = static_cast<uint16_t>(args->argument_end);
    register_end_ = static_cast<uint16_t>(args->register_end);
    ident_end_ = static_cast<uint16_t>(args->ident_end);
    const_literal_end_ = static_cast<uint16_t>(args->const_literal_end);
    literal_end_ = static_cast<uint16_t>(args->literal_end);
    stack_limit_ = static_cast<uint16_t>(args->stack_limit);
#if ENABLED(JERRY_BUILTIN_REALMS)
    realm_value_ = realm_value_;
#endif
    size_ = static_cast<uint16_t>(sizeof(cbc_uint8_arguments_t));
  }

  cbc_uint8_arguments_t toU8args() {
    cbc_uint8_arguments_t args;
    args.argument_end = static_cast<uint8_t>(argument_end_);
    args.register_end = static_cast<uint8_t>(register_end_);
    args.ident_end = static_cast<uint8_t>(ident_end_);
    args.const_literal_end = static_cast<uint8_t>(const_literal_end_);
    args.literal_end = static_cast<uint8_t>(literal_end_);
    args.stack_limit = static_cast<uint8_t>(stack_limit_);
#if ENABLED(JERRY_BUILTIN_REALMS)
    args.realm_value = realm_value_;
#endif

    return args;
  }

  cbc_uint16_arguments_t toU16args() {
    cbc_uint16_arguments_t args;
    args.argument_end = argument_end_;
    args.register_end = register_end_;
    args.ident_end = ident_end_;
    args.const_literal_end = const_literal_end_;
    args.literal_end = literal_end_;
    args.stack_limit = stack_limit_;
    args.padding = 0;
#if ENABLED(JERRY_BUILTIN_REALMS)
    args.realm_value = realm_value_;
#endif

    return args;
  }

  void moveRegIndex(int32_t offset) {
    register_end_ = static_cast<uint16_t>(register_end_ + offset);
    ident_end_ = static_cast<uint16_t>(ident_end_ + offset);
    const_literal_end_ = static_cast<uint16_t>(const_literal_end_ + offset);
    literal_end_ = static_cast<uint16_t>(literal_end_ + offset);
  }

  void setEncoding(uint16_t limit, uint16_t delta, uint16_t one_byte_limit) {
    encoding_limit_ = limit;
    encoding_delta_ = delta;
    one_byte_limit_ = one_byte_limit;
  }

  auto literalCount() { return literal_end_ - register_end_; }

  auto argumentEnd() const { return argument_end_; }
  auto registerEnd() const { return register_end_; }
  auto identEnd() const { return ident_end_; }
  auto constLiteralEnd() const { return const_literal_end_; }
  auto literalEnd() const { return literal_end_; }
  auto stackLimit() const { return stack_limit_; }
  auto encodingLimit() const { return encoding_limit_; }
  auto encodingDelta() const { return encoding_delta_; }
  auto oneByteLimit() const { return one_byte_limit_; }
  auto realmValue() const { return realm_value_; }
  auto size() const { return size_; }

private:
  uint16_t argument_end_;
  uint16_t register_end_;
  uint16_t ident_end_;
  uint16_t const_literal_end_;
  uint16_t literal_end_;
  uint16_t encoding_limit_;
  uint16_t encoding_delta_;
  uint16_t one_byte_limit_;
  uint16_t stack_limit_;
#if ENABLED(JERRY_BUILTIN_REALMS)
  ecma_value_t realm_value_;
#endif
  uint16_t size_;
};

class LiteralPool {
public:
  LiteralPool() {}

  auto literalStart() const { return literal_start_; }
  auto end() const { return end_; }
  auto size() const { return size_; }

  ValueRef getLiteral(LiteralIndex index) {
    assert(index < end_);
    return Value::_value(literalStart()[end_ - index]);
  }

  void movePoolStart(int32_t offset) {
    end_ = static_cast<uint16_t>(end_ + offset);
  }

  uint8_t *setLiteralPool(void *literalStart, BytecodeArguments &args) {
    literal_start_ = reinterpret_cast<ecma_value_t *>(literalStart);
    end_ = args.literalEnd();
    size_ = end_ - args.registerEnd();

    /* Bytecode start */
    return reinterpret_cast<uint8_t *>(literal_start_ + size_);
  }

private:
  ecma_value_t *literal_start_;
  uint16_t size_;
  uint16_t end_;
};

class Bytecode;
using BytecodeList = std::vector<Bytecode *>;

class Bytecode {
public:
  Bytecode(ecma_value_t function);
  Bytecode(ecma_compiled_code_t *compiled_code, Bytecode *parent,
           uint32_t parent_literal_pool_index);
  ~Bytecode();

  auto compiledCode() const { return compiled_code_; }
  auto &args() { return args_; }

  auto function() const { return function_; }
  auto &byteCodeStart() const { return byte_code_start_; }
  auto &byteCodeCurrent() { return byte_code_; }
  auto flags() const { return flags_; }
  auto literalPool() const { return literal_pool_; }
  auto &stack() { return stack_; }
  auto &instructions() { return instructions_; }
  auto &offsetToInst() { return offset_to_ins_; }
  auto &basicBlockList() { return bb_list_; }

  auto &liveRanges() { return live_ranges_; }

  Ins *insAt(int32_t offset) { return offsetToInst().find(offset)->second; }

  size_t compiledCodesize() const {
    return compiledCode()->size << JMEM_ALIGNMENT_LOG;
  }

  void setArguments(cbc_uint16_arguments_t *args);
  void setArguments(cbc_uint8_arguments_t *args);
  void setEncoding();
  void setBytecodeEnd();

  uint32_t toRegisterIndex(LiteralIndex index) {
    return index; // - args().argumentEnd();
  };

  static size_t countFunctions(std::string snapshot);
  static BytecodeList readFunctions(ecma_value_t function);
  static void readSubFunctions(BytecodeList &functions,
                               Bytecode *parent_byte_code);

  uint32_t offset() { return byte_code_ - byte_code_start_; }

  uint8_t next() {
    assert(hasNext());
    return *byte_code_++;
  };
  uint8_t current() { return *byte_code_; };
  bool hasNext() { return byte_code_ < byte_code_end_; };

  void emit();

private:
  void decodeHeader();
  void buildInstructions();

  void emitHeader(std::vector<uint8_t> &buffer);
  void emitInstructions(std::vector<uint8_t> &buffer);

  ecma_value_t function_;
  ecma_compiled_code_t *compiled_code_;
  Bytecode *parent_;
  uint32_t parent_literal_pool_index_;

  uint8_t *byte_code_;
  uint8_t *byte_code_start_;
  uint8_t *byte_code_end_;
  size_t end_info_;
  BytecodeFlags flags_;

  BytecodeArguments args_;
  LiteralPool literal_pool_;
  Stack stack_;
  InsList instructions_;
  OffsetMap offset_to_ins_;
  BasicBlockList bb_list_;

  // Live Ranges
  LiveRangeMap live_ranges_;
};

} // namespace optimizer
#endif // BYTECODE_H
