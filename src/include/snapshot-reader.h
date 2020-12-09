/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef SNAPSHOT_READER_H
#define SNAPSHOT_READER_H

#include "bytecode.h"
#include "common.h"

namespace optimizer {

class SnapshotReaderResult {
public:
  SnapshotReaderResult(Bytecode *bytecode) : bytecode_(bytecode) {}
  SnapshotReaderResult(std::string error) : error_(error) {}

  auto bytecode() const { return bytecode_; }
  auto error() const { return error_; }

private:
  Bytecode *bytecode_;
  std::string error_;
};

class SnapshotReader {
public:
  SnapshotReader(std::string &snapshot) : snapshot_(snapshot) {}

  SnapshotReaderResult read();

  auto snapshot() const { return snapshot_; }

private:
  std::string snapshot_;
};

} // namespace optimizer
#endif // SNAPSHOT_READER_H
