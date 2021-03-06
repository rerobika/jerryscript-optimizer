/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#ifndef SNAPSHOT_READWRITER_H
#define SNAPSHOT_READWRITER_H

#include "bytecode.h"
#include "common.h"

namespace optimizer {

class SnapshotReadResult {
public:
  SnapshotReadResult(BytecodeList &list) : list_(std::move(list)), error_("") {}
  SnapshotReadResult(std::string error) : error_(error) {}
  ~SnapshotReadResult();

  auto &list() { return list_; }
  auto error() const { return error_; }

  bool failed() { return error_.length() != 0; }

private:
  BytecodeList list_;
  std::string error_;
};

class SnapshotWriteResult {
public:
  SnapshotWriteResult(std::string error) : error_(error) {}
  SnapshotWriteResult() : error_("") {}

  bool failed() const { return error_.size() != 0; }
  auto error() const { return error_; }

private:
  std::string error_;
};

class SnapshotReadWriter {
public:
  SnapshotReadWriter(std::string &snapshot);
  ~SnapshotReadWriter();

  SnapshotReadResult read();
  SnapshotWriteResult write(std::string &path, BytecodeList &list);

  auto snapshot() const { return snapshot_; }
  auto bytecode() const { return bytecode_; }

private:
  uint32_t writeSnapshot(Bytecode *bytecode, uint32_t generate_snapshot_opts,
                         uint32_t *buffer_p, uint32_t buffer_size);
  std::string snapshot_;
  std::shared_ptr<Bytecode> bytecode_;
};

} // namespace optimizer
#endif // SNAPSHOT_READWRITER_H
