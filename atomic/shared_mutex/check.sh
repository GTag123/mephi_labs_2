#!/bin/bash

defines=$(grep -c '#define' task.h)
undefs=$(grep -c '#undef' task.h)
includes=$(grep -E -c '#include ["<](shared_)?mutex[">]' task.h)

[[ $undefs = 0 ]] && [[ $defines = 0 ]] && [[ $includes = 0 ]]
