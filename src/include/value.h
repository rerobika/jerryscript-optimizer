/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef VALUE_H
#define VALUE_H

#include "common.h"
#include "ecma-helpers.h"

namespace optimizer {

class Literal;

enum class ValueType {
  ANY,
  OBJECT,
  NUMBER,
  STRING,
  BOOLEAN,
  PRIMITIVE,
};

class Value {
public:
  Value();
  Value(ValueType type);
  Value(ecma_value_t value);
  Value(ecma_value_t number, ValueType type);

  auto value() const { return value_; }
  auto type() const { return type_; }

  ~Value();

  static ecma_value_t _undefined() { return ECMA_VALUE_UNDEFINED; }

  static ecma_value_t _null() { return ECMA_VALUE_NULL; }

  static ecma_value_t _true() { return ECMA_VALUE_TRUE; }

  static ecma_value_t _false() { return ECMA_VALUE_FALSE; }

private:
  ecma_value_t value_;
  ValueType type_;
};
} // namespace optimizer
#endif // VALUE_H
