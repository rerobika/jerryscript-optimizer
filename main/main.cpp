/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "argparse.h"
#include "optimizer.h"
#include "passes.h"
#include <fstream>
#include <iostream>

int main(int argc, char const *argv[]) {
  argparse::ArgumentParser argparser("argparser", "Argument parser");
  argparser.add_argument()
      .names({"-i", "--input"})
      .description("Input snapshot to optimize")
      .position(argparse::ArgumentParser::Argument::Position::LAST)
      .required(true);
  argparser.add_argument()
      .names({"-o", "--output"})
      .description("Optimized snapshot output")
      .required(false);

  argparser.enable_help();

  auto err = argparser.parse(argc, argv);
  if (err) {
    std::cerr << err << std::endl;
  }

  if (argparser.exists("help")) {
    argparser.print_help();
    return 0;
  }

  std::ostringstream buff;
  std::ifstream input_stream(argparser.get<std::string>("i").c_str());
  buff << input_stream.rdbuf();
  auto input_str = buff.str();

  optimizer::SnapshotReadWriter reader(input_str);
  auto res = reader.read();

  if (res.failed()) {
    std::cerr << "Snapshot parsing error: " << res.error() << std::endl;
    return 2;
  }

  optimizer::Optimizer optimizer(res.list());
  optimizer.addPass(new optimizer::IRBuilder());
  optimizer.addPass(new optimizer::Dominator());
  optimizer.run();

  return 0;
}
