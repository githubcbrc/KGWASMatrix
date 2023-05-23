#!/bin/bash
/usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-10 -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++-10 -S/project/src -B/project/build -G Ninja
/usr/bin/cmake --build /project/build --config Release --target all

