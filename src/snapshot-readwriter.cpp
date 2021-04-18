/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

extern "C" {
#include "jerryscript-config.h"
}

extern "C" {
#include "jerry-snapshot.h"
#include "jerryscript-port-default.h"
#include "jerryscript.h"
}

#include "snapshot-readwriter.h"

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

// TODO: improve this part in API level

/**
 * Maximum size for emitted snapshots
 */
static uint32_t output_buffer[1048576];

uint32_t SnapshotReadWriter::writeSnapshot(Bytecode *bytecode,
                                           uint32_t generate_snapshot_opts,
                                           uint32_t *buffer_p,
                                           uint32_t buffer_size) {

  snapshot_globals_t globals;
  const uint32_t aligned_header_size =
      JERRY_ALIGNUP(sizeof(jerry_snapshot_header_t), JMEM_ALIGNMENT);

  globals.snapshot_buffer_write_offset = aligned_header_size;
  globals.snapshot_error = ECMA_VALUE_EMPTY;
  globals.regex_found = false;
  globals.class_found = false;

  ecma_compiled_code_t *bytecode_data_p = bytecode->compiledCode();

  if (generate_snapshot_opts & JERRY_SNAPSHOT_SAVE_STATIC) {
    static_snapshot_add_compiled_code(bytecode_data_p, (uint8_t *)buffer_p,
                                      buffer_size, &globals);
  } else {
    snapshot_add_compiled_code(bytecode_data_p, (uint8_t *)buffer_p,
                               buffer_size, &globals);
  }

  if (!ecma_is_value_empty(globals.snapshot_error)) {
    return 0;
  }

  jerry_snapshot_header_t header;
  header.magic = JERRY_SNAPSHOT_MAGIC;
  header.version = JERRY_SNAPSHOT_VERSION;
  header.global_flags =
      snapshot_get_global_flags(globals.regex_found, globals.class_found);
  header.lit_table_offset = (uint32_t)globals.snapshot_buffer_write_offset;
  header.number_of_funcs = 1;
  header.func_offsets[0] = aligned_header_size;

  lit_mem_to_snapshot_id_map_entry_t *lit_map_p = NULL;
  uint32_t literals_num = 0;

  if (!(generate_snapshot_opts & JERRY_SNAPSHOT_SAVE_STATIC)) {
    ecma_collection_t *lit_pool_p = ecma_new_collection();

    ecma_save_literals_add_compiled_code(bytecode_data_p, lit_pool_p);

    if (!ecma_save_literals_for_snapshot(lit_pool_p, buffer_p, buffer_size,
                                         &globals.snapshot_buffer_write_offset,
                                         &lit_map_p, &literals_num)) {
      JERRY_ASSERT(lit_map_p == NULL);
      return 0;
    }

    jerry_snapshot_set_offsets(
        buffer_p + (aligned_header_size / sizeof(uint32_t)),
        (uint32_t)(header.lit_table_offset - aligned_header_size), lit_map_p);
  }

  size_t header_offset = 0;

  snapshot_write_to_buffer_by_offset((uint8_t *)buffer_p, buffer_size,
                                     &header_offset, &header, sizeof(header));

  if (lit_map_p != NULL) {
    jmem_heap_free_block(
        lit_map_p, literals_num * sizeof(lit_mem_to_snapshot_id_map_entry_t));
  }

  return globals.snapshot_buffer_write_offset;
}

SnapshotWriteResult SnapshotReadWriter::write(std::string &path,
                                              BytecodeList &list) {
  std::vector<size_t> snapshot_sizes;
  std::vector<uint32_t *> snapshot_buffers;

  uint32_t *buffer_p = output_buffer;
  uint32_t buffer_size = sizeof(output_buffer) / sizeof(uint32_t);

  for (auto bytecode : list) {
    bytecode->emit();
    if (!jerry_value_is_undefined(bytecode->function())) {
      uint32_t res = writeSnapshot(bytecode, 0, buffer_p, buffer_size);

      if (res == 0) {
        return {"snaphot genration error"};
      }

      snapshot_sizes.push_back(res);
      snapshot_buffers.push_back(buffer_p);

      buffer_p += res;
      buffer_size -= res;
    }
  }

  size_t final_size = snapshot_sizes[0];
  uint32_t *final_snapshot = snapshot_buffers[0];

  if (snapshot_sizes.size() > 1) {
    const char *error_p;
    final_size = jerry_merge_snapshots(
        const_cast<const uint32_t **>(snapshot_buffers.data()),
        snapshot_sizes.data(), snapshot_sizes.size(), buffer_p, buffer_size,
        &error_p);
    final_snapshot = buffer_p;
  }

  std::ofstream output;
  output.open(path, std::ios::out | std::ios::binary);
  output.write(reinterpret_cast<char *>(final_snapshot),
                            final_size);
  output.close();

  return {};
}

} // namespace optimizer
