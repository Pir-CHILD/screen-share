#include <iostream>

#include "absl/strings/ascii.h"

int main()
{
    std::cout << absl::ascii_isspace(' ') << std::endl;

    return 0;
}