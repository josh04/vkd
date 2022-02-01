#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# For OS X
export DYLD_LIBRARY_PATH="/Users/josh04/code/OpenColorIO/build/bundle/release/lib:${DYLD_LIBRARY_PATH}"

# For Linux
export LD_LIBRARY_PATH="/Users/josh04/code/OpenColorIO/build/bundle/release/lib:${LD_LIBRARY_PATH}"

export PATH="/Users/josh04/code/OpenColorIO/build/bundle/release/bin:${PATH}"
export PYTHONPATH="/Users/josh04/code/OpenColorIO/build/bundle/release/:${PYTHONPATH}"
