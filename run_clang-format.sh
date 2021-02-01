#!/bin/bash

# By Gabriel Staples
# For documentation on this file, see my answer here:
# https://stackoverflow.com/questions/28896909/how-to-call-clang-format-over-a-cpp-project-folder/65988393#65988393

THIS_PATH="$(realpath "$0")"
THIS_DIR="$(dirname "$THIS_PATH")"

# Find all files in THIS_DIR which end in .ino, .cpp, etc., as specified
# in the regular expression just below
FILE_LIST="$(find "$THIS_DIR" | grep -E ".*(\.ino|\.cpp|\.c|\.h|\.hpp|\.hh)$")"

echo -e "Files found to format = \n\"\"\"\n$FILE_LIST\n\"\"\""

# Format each file.
# - NB: do NOT put quotes around `$FILE_LIST` below or else the `clang-format` command will
#   mistakenly see the entire blob of newline-separated file names as a SINGLE file name instead
#   of as a new-line separated list of *many* file names!
clang-format --verbose -i --style=file $FILE_LIST
