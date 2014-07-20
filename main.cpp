#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

#include "asf.hpp"


int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        std::ifstream asf_str(argv[i]);
        if (!asf_str.is_open()) {
            fprintf(stderr, "%s: Could not open %s: %s\n", argv[0], argv[i], strerror(errno));
            return 1;
        }

        ASF bone_data(asf_str);
    }

    return 0;
}
