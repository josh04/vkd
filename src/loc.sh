#!/bin/bash
find . -path ./imgui -prune -o -name '*.cpp' -or -name '*.hpp' | xargs wc -l
