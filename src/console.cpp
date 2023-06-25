#include "console.hpp"

namespace vkd {

    namespace {
        thread_local std::stringstream _buf;
    }
    std::stringstream& Console::buf() { return _buf; }
    Console console;
}