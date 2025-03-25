#!/bin/bash

if [ ! -e tools/generate_cmakelists.py ]
then
    echo "This script must be run from the HikoGUI root"
    exit 2
fi

PYTHON3=python
if [ -e /usr/local/bin/python3 ]
then
    PYTHON3=/usr/local/bin/python3
fi

$PYTHON3 tools/generate_cmakelists.py
