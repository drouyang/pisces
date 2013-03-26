#!/bin/bash
find "/sys/devices/system/memory/" -iname "removable" -exec cat {} \;
