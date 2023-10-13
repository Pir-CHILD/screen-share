#include <iostream>

extern "C"
{
#include "libavutil/avutil.h"
}

int main()
{
    std::cout << avutil_version() << std::endl;

    return 0;
}