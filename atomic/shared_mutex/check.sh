#!/bin/bash

defines=$(grep -c '#define' task.h)
undefs=$(grep -c '#undef' task.h)
includes=$(grep -E -c '#include *["<](shared_)?mutex[">]' task.h)
using_namespace_std=$(grep -E -c 'using *namespace *std' task.h)
std_mutex=$(grep -c 'std::mutex' task.h)
std_shared_mutex=$(grep -c 'std::shared_mutex' task.h)

[[ $undefs = 0 ]] && [[ $defines = 0 ]] && [[ $includes = 0 ]] && [[ $using_namespace_std = 0 ]] && [[ $std_shared_mutex = 0 ]] && [[ $std_mutex = 0 ]]