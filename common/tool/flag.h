//
// Created by ByteDance on 2025/2/7.
//

#ifndef TOOL_FLAG_H
#define TOOL_FLAG_H
#include "../types/basic.h"

static size_t parse_flag(int argc, char *argv[], char **result) {
    size_t flag_count = 0;
    for (int i = 1; i < argc-1;) {
        if (argv[i][0] == '-') {
            result[flag_count*2+0] = argv[i]+1; // move right a char, skip the '-'
            result[flag_count*2+1] = argv[i+1];
            i += 2;
            flag_count++;
            continue;
        }
        i++;
    }
    return flag_count;
}

#endif //TOOL_FLAG_H
