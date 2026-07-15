#! /bin/bash
find src tests -type f -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i -style=file
