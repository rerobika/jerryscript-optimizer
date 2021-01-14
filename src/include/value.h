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
extern "C" {
#include "ecma-helpers.h"
}

namespace optimizer {

class Literal;
class Value;

enum class ValueType {
  ANY,
  OBJECT,
  NUMBER,
  STRING,
  BOOLEAN,
  PRIMITIVE,
};

using ValueRef = std::shared_ptr<Value>;
using ValueRefList = std::vector<ValueRef>;

class Value {
public:
  Value(ecma_value_t value);
  Value(ecma_value_t number, ValueType type);

  auto value() const { return value_; }
  auto type() const { return type_; }

  ~Value();

  static ValueRef _value(ecma_value_t value) {
    return std::make_shared<Value>(value);
  }
  static ValueRef _any() { return std::make_shared<Value>(0, ValueType::ANY); }

  static ValueRef _object() {
    return std::make_shared<Value>(0, ValueType::OBJECT);
  }

  static ValueRef _undefined() {
    return std::make_shared<Value>(ECMA_VALUE_UNDEFINED, ValueType::PRIMITIVE);
  }

  static ValueRef _null() {
    return std::make_shared<Value>(ECMA_VALUE_NULL, ValueType::PRIMITIVE);
  }

  static ValueRef _true() {
    return std::make_shared<Value>(ECMA_VALUE_TRUE, ValueType::PRIMITIVE);
  }

  static ValueRef _false() {
    return std::make_shared<Value>(ECMA_VALUE_FALSE, ValueType::PRIMITIVE);
  }

private:
  ecma_value_t value_;
  ValueType type_;
};

} // namespace optimizer
#endif // VALUE_H
