/* Copyright (c) 2020 Robert Fancsik
 *
 * Licensed under the BSD 3-Clause License
 * <LICENSE or https://opensource.org/licenses/BSD-3-Clause>.
 * This file may not be copied, modified, or distributed except
 * according to those terms.
 */

#include "argparse.h"
#include <iostream>

int main(int argc, char const *argv[]) {
  argparse::ArgumentParser argparser("argparser", "Argument parser");
  argparser.add_argument()
      .names({"-i", "--input"})
      .description("The input file to parse")
      .position(argparse::ArgumentParser::Argument::Position::LAST)
      .required(true);

  argparser.enable_help();

  auto err = argparser.parse(argc, argv);
  if (err) {
    std::cout << err << std::endl;
    return 2;
  }

  if (argparser.exists("help")) {
    argparser.print_help();
    return 0;
  }

  return 0;
}
