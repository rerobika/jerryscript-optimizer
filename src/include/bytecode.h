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

namespace optimizer {

class Bytecode {
public:
  Bytecode() : buffer_(nullptr), size_(0) {}
  Bytecode(uint8_t *buffer, size_t size) : buffer_(buffer), size_(size) {}

  auto buffer() const {
    assert(buffer_ != nullptr);
    return buffer_;
  }

  auto size() const {
    assert(size_ != 0);
    return size_;
  }

private:
  uint8_t *buffer_;
  size_t size_;
};

} // namespace optimizer
#endif // BYTECODE_H
