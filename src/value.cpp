/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "value.h"

namespace optimizer {

Value::Value(ecma_value_t value) {
  value_ = value;

  switch (value) {
  case ECMA_VALUE_UNDEFINED:
  case ECMA_VALUE_NULL: {
    type_ = ValueType::PRIMITIVE;
    break;
  }
  case ECMA_VALUE_TRUE:
  case ECMA_VALUE_FALSE: {
    type_ = ValueType::BOOLEAN;
    break;
  }
  default: {
    if (ecma_is_value_object(value)) {
      type_ = ValueType::OBJECT;
    } else if (ecma_is_value_string(value)) {
      type_ = ValueType::STRING;
    } else {
      assert(ecma_is_value_number(value));
      value_ = ecma_copy_value(value);
      type_ = ValueType::NUMBER;
    }
  }
  }
}

Value::Value(ecma_value_t number, ValueType type)
    : value_(number), type_(type) {}

Value::~Value() {
  if (type() == ValueType::NUMBER) {
    ecma_free_value(value());
  }
}
} // namespace optimizer
