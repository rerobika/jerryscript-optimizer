/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef INST_H
#define INST_H

#include "common.h"

class Argument {
  enum Type {

  };

public:
  Argument() {}

private:
};

class Inst {
public:
  Inst() {}

private:
  std::vector<std::unique_ptr<Argument>> args_;
  Inst *dest_;
};

#endif // INST_H
