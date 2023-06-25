#include "catch.hpp"
#include "console.hpp"

TEST_CASE("Test console output", "[console]") {
    vkd::console << "woah";
    CHECK(vkd::console.log() == "");
    vkd::console << std::endl;
    CHECK(vkd::console.log() == "woah\n");
}