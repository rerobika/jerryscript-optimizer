/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "snapshot-readwriter.h"
extern "C" {
#include "jerryscript-port-default.h"
#include "jerryscript.h"
}

namespace optimizer {

SnapshotReadResult::~SnapshotReadResult() {
  for (auto it : list_) {
    delete it;
  }
}

SnapshotReadWriter::SnapshotReadWriter(std::string &snapshot)
    : snapshot_(snapshot) {
  jerry_init(JERRY_INIT_SHOW_OPCODES);
  jerry_port_default_set_log_level(JERRY_LOG_LEVEL_DEBUG);
}

SnapshotReadWriter::~SnapshotReadWriter() { jerry_cleanup(); }

SnapshotReadResult SnapshotReadWriter::read() {
  size_t number_of_funcs = Bytecode::countFunctions(snapshot());
  BytecodeList function_table;

  for (size_t i = 0; i < number_of_funcs; i++) {
    jerry_value_t function = jerry_load_function_snapshot(
        reinterpret_cast<const uint32_t *>(snapshot().c_str()),
        snapshot().size(), 0, JERRY_SNAPSHOT_EXEC_COPY_DATA);

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

    auto function_list = Bytecode::readFunctions(function);
    function_table.insert(function_table.end(), function_list.begin(),
                          function_list.end());
  }

  return {function_table};
}

SnapshotWriteResult SnapshotReadWriter::write(std::string &path) {
  return {"unimplemented"};
}

} // namespace optimizer
