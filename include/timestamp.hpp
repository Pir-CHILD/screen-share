#include <chrono>
#include <utility>
#include <cstring>

const char *time_now()
{
    auto now = std::chrono::system_clock::now();
    const std::time_t c_t = std::chrono::system_clock::to_time_t(std::move(now));
    // remove newline
    char *t = std::ctime(&c_t);
    if (t[strlen(t) - 1] == '\n')
        t[strlen(t) - 1] = '\0';

    return t;
}