#!/bin/bash
find "/sys/devices/system/memory/" -iname "state" -exec cat {} \;
