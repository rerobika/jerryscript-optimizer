/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "bytecode.h"
#include "basic-block.h"
#include "inst.h"
#include "liveness-analysis.h"

extern "C" {
#include "jerry-snapshot.h"
#include "jerryscript.h"
}

namespace optimizer {

Bytecode::Bytecode(ecma_value_t function)
    : parent_(nullptr), parent_literal_pool_index_(0) {
  assert(ecma_is_value_object(function));

  auto func = ecma_get_object_from_value(function);
  assert(ecma_get_object_type(func) == ECMA_OBJECT_TYPE_FUNCTION);
  auto ext_func = reinterpret_cast<ecma_extended_object_t *>(func);

  function_ = function;
  compiled_code_ = const_cast<ecma_compiled_code_t *>(
      ecma_op_function_get_compiled_code(ext_func));

  buildInstructions();
}

Bytecode::Bytecode(ecma_compiled_code_t *compiled_code, Bytecode *parent,
                   uint32_t parent_literal_pool_index)
    : function_(ECMA_VALUE_UNDEFINED), compiled_code_(compiled_code),
      parent_(parent), parent_literal_pool_index_(parent_literal_pool_index) {
  buildInstructions();
}

void Bytecode::readSubFunctions(BytecodeList &list,
                                Bytecode *parent_byte_code) {
  for (uint32_t i = parent_byte_code->args().constLiteralEnd();
       i < parent_byte_code->args().literalEnd(); i++) {
    ecma_compiled_code_t *bytecode_literal_p = ECMA_GET_INTERNAL_VALUE_POINTER(
        ecma_compiled_code_t,
        parent_byte_code->literalPool().literalStart()[i]);

    if (bytecode_literal_p != parent_byte_code->compiledCode() &&
        CBC_IS_FUNCTION(bytecode_literal_p->status_flags)) {
      Bytecode *sub_byte_code =
          new Bytecode(bytecode_literal_p, parent_byte_code, i);
      readSubFunctions(list, sub_byte_code);
      list.push_back(sub_byte_code);
    }
  }
}

size_t Bytecode::countFunctions(std::string snapshot) {
  const uint8_t *snapshot_data = (uint8_t *)snapshot.c_str();
  size_t snapshot_size = snapshot.size();

  if (snapshot_size <= sizeof(jerry_snapshot_header_t)) {
    return SIZE_MAX;
  }

  auto header_p =
      reinterpret_cast<const jerry_snapshot_header_t *>(snapshot_data);

  return header_p->number_of_funcs;
}

BytecodeList Bytecode::readFunctions(ecma_value_t function) {
  BytecodeList list;

  Bytecode *byte_code = new Bytecode(function);
  readSubFunctions(list, byte_code);
  list.push_back(byte_code);

  return list;
}

void Bytecode::setArguments(cbc_uint16_arguments_t *args) {
  args_.setArguments(args);
  byte_code_start_ = literal_pool_.setLiteralPool(args + 1, args_);
  byte_code_ = byte_code_start_;
}

void Bytecode::setArguments(cbc_uint8_arguments_t *args) {
  args_.setArguments(args);
  byte_code_start_ = literal_pool_.setLiteralPool(args + 1, args_);
  byte_code_ = byte_code_start_;
}

void Bytecode::setEncoding() {
  uint16_t limit = CBC_SMALL_LITERAL_ENCODING_LIMIT;
  uint16_t delta = CBC_SMALL_LITERAL_ENCODING_DELTA;
  uint16_t one_byte_limit = CBC_MAXIMUM_BYTE_VALUE - 1;

  if (flags_.fullLiteralEncoding()) {
    limit = CBC_FULL_LITERAL_ENCODING_LIMIT,
    delta = CBC_FULL_LITERAL_ENCODING_DELTA;
    one_byte_limit = CBC_LOWER_SEVEN_BIT_MASK;
  }
  args_.setEncoding(limit, delta, one_byte_limit);
}

void Bytecode::setBytecodeEnd() {
  size_t size = compiledCodesize();
  size_t end_info = 0;

  if (flags().mappedArgumentsNeeded()) {
    end_info += args().argumentEnd() * sizeof(ecma_value_t);
  }

  if (flags().hasExtendedInfo()) {
    end_info += sizeof(ecma_value_t);
  }

  if (flags().hasName()) {
    end_info += sizeof(ecma_value_t);
  }

  if (flags().hasTaggedTemplateLiterals()) {
    end_info += sizeof(ecma_value_t);
  }

  byte_code_end_ =
      reinterpret_cast<uint8_t *>(compiledCode()) + size - end_info;
  end_info_ = end_info;
}

void Bytecode::decodeHeader() {
  flags_.setFlags(compiled_code_->status_flags);

  if (flags_.uint16Arguments()) {
    auto args = reinterpret_cast<cbc_uint16_arguments_t *>(compiled_code_);
    setArguments(args);
  } else {
    auto args = reinterpret_cast<cbc_uint8_arguments_t *>(compiled_code_);
    setArguments(args);
  }

  setEncoding();
  setBytecodeEnd();
  stack_ = Stack(args().stackLimit(), args().registerEnd());
}

void Bytecode::buildInstructions() {
  decodeHeader();
  LOG("--------- function intructions start --------");

  while (hasNext()) {
    Ins *inst = new Ins(this);
    instructions().push_back(inst);

    if (!inst->decodeCBCOpcode()) {
      delete inst;
      instructions().pop_back();
      break;
    }
    inst->decodeArguments();
    inst->decodeGroupOpcode();
    offsetToInst().insert({inst->offset(), inst});
  }

  auto &inst = instructions().back();

  /* end of bytecode stream */
  if (inst->size() == 0) {
    instructions().back()->setSize(offset() - instructions().back()->offset());
  }

  LOG("--------- function intructions end --------");
}

Bytecode::~Bytecode() {
  for (auto iter : live_ranges_) {
    for (auto li : iter.second) {
      delete li;
    }
  }

  for (auto &bb : bb_list_) {
    delete bb;
  }

  for (auto ins : instructions_) {
    delete ins;
  }

  ecma_free_value(function_);
};

void Bytecode::emitHeader(std::vector<uint8_t> &buffer) {
  size_t lit_pool_size = literal_pool_.size() * sizeof(ecma_value_t);
  size_t total_size = args_.size() + lit_pool_size;

  buffer.resize(total_size);
  uint8_t *rbuffer = buffer.data();

  /* write cbc arguments */
  if (flags_.uint16Arguments()) {
    cbc_uint16_arguments_t args = args_.toU16args();
    args.header = *compiled_code_;
    memcpy(rbuffer, &args, sizeof(cbc_uint16_arguments_t));
  } else {
    cbc_uint8_arguments_t args = args_.toU8args();
    args.header = *compiled_code_;
    memcpy(rbuffer, &args, sizeof(cbc_uint8_arguments_t));
  }

  rbuffer += args_.size();

  /* write literal pool */
  memcpy(rbuffer, literal_pool_.literalStart(), lit_pool_size);
}

void Bytecode::emitInstructions(std::vector<uint8_t> &buffer) {
  for (auto &ins : instructions_) {
    /* write opcode */
    ins->emit(buffer);
  }
}

/**
 * Reconstruct the bytecode from the IR
 */
void Bytecode::emit() {
  std::vector<uint8_t> buffer;

  emitHeader(buffer);
  emitInstructions(buffer);

  size_t total_size = JERRY_ALIGNUP(buffer.size(), JMEM_ALIGNMENT);

  while (buffer.size() != total_size) {
    buffer.push_back(0);
  }

  if (end_info_ != 0) {
    buffer.resize(buffer.size() + end_info_);
    memcpy(buffer.data() + buffer.size(),
           reinterpret_cast<uint8_t *>(compiled_code_) + compiledCodesize() -
               end_info_,
           end_info_);

    total_size = JERRY_ALIGNUP(buffer.size(), JMEM_ALIGNMENT);
  }

  jmem_heap_free_block(compiled_code_,
                       compiled_code_->size << JMEM_ALIGNMENT_LOG);

  compiled_code_ = reinterpret_cast<ecma_compiled_code_t *>(
      jmem_heap_alloc_block(total_size));
  memcpy(compiled_code_, buffer.data(), total_size);
  compiled_code_->size = total_size >> JMEM_ALIGNMENT_LOG;

  if (parent_) {
    assert(parent_->flags().isFunction());
    ECMA_SET_INTERNAL_VALUE_POINTER(
        parent_->literalPool().literalStart()[parent_literal_pool_index_],
        compiled_code_);
  } else {
    auto func = ecma_get_object_from_value(function_);
    assert(ecma_get_object_type(func) == ECMA_OBJECT_TYPE_FUNCTION);
    auto ext_func = reinterpret_cast<ecma_extended_object_t *>(func);

#if ENABLED(JERRY_SNAPSHOT_EXEC)
    if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION) {
      ext_func->u.function.bytecode_cp = JMEM_CP_NULL;
      ((ecma_static_function_t *)func_p)->bytecode_p = compiled_code_;
    } else
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */
    {
      ECMA_SET_INTERNAL_VALUE_POINTER(ext_func->u.function.bytecode_cp,
                                      compiled_code_);
    }
  }
}

} // namespace optimizer
