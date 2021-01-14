/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "snapshot-readwriter.h"
extern "C" {
#include "jerryscript.h"
}

namespace optimizer {

SnapshotReadWriter::SnapshotReadWriter(std::string &snapshot)
    : snapshot_(snapshot) {
  jerry_init(JERRY_INIT_SHOW_OPCODES);
}

SnapshotReadWriter::~SnapshotReadWriter() { jerry_cleanup(); }

SnapshotReadResult SnapshotReadWriter::read() {
  jerry_value_t function = jerry_load_function_snapshot(
      reinterpret_cast<const uint32_t *>(snapshot_.c_str()), snapshot_.size(),
      0, JERRY_SNAPSHOT_EXEC_COPY_DATA);

  if (jerry_value_is_error(function)) {
    jerry_value_t error = jerry_get_value_from_error(function, true);
    jerry_value_t to_string = jerry_value_to_string(error);
    jerry_release_value(error);

    size_t str_size = jerry_get_string_size(to_string);
    uint8_t *buff = new uint8_t[str_size + 1];

    jerry_string_to_char_buffer(to_string, buff, str_size);
    buff[str_size] = 0;

    std::string error_str(reinterpret_cast<const char *>(buff), str_size);

    delete[] buff;
    jerry_release_value(to_string);
    return {error_str.substr(error_str.find(": ") + 2)};
  }

  return {std::make_shared<Bytecode>(function)};
}

SnapshotWriteResult SnapshotReadWriter::write(std::string &path) {
  return {"unimplemented"};
}

} // namespace optimizer
