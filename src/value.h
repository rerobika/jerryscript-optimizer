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
  INTERNAL,
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

  static ValueRef _internal() {
    return std::make_shared<Value>(0, ValueType::INTERNAL);
  }

  static ValueRef _undefined() {
    return std::make_shared<Value>(ECMA_VALUE_UNDEFINED, ValueType::PRIMITIVE);
  }

  static ValueRef _null() {
    return std::make_shared<Value>(ECMA_VALUE_NULL, ValueType::PRIMITIVE);
  }

  static ValueRef _true() {
    return std::make_shared<Value>(ECMA_VALUE_TRUE, ValueType::BOOLEAN);
  }

  static ValueRef _false() {
    return std::make_shared<Value>(ECMA_VALUE_FALSE, ValueType::BOOLEAN);
  }

  static ValueRef _boolean() {
    return std::make_shared<Value>(0, ValueType::BOOLEAN);
  }

  static ValueRef _number() {
    return std::make_shared<Value>(0, ValueType::NUMBER);
  }

  static ValueRef _number(double num) {
    return std::make_shared<Value>(ecma_make_number_value(num),
                                   ValueType::NUMBER);
  }

  static ValueRef _string() {
    return std::make_shared<Value>(0, ValueType::STRING);
  }

private:
  ecma_value_t value_;
  ValueType type_;
};

} // namespace optimizer
#endif // VALUE_H
