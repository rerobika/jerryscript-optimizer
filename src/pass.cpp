/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "pass.h"

namespace optimizer {

Pass::Pass() {}
Pass::~Pass() {}

bool Pass::run(Optimizer* optimizer, Bytecode *byte_code) { return false; }

} // namespace optimizer
