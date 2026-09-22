#!/bin/bash
size=$(((RANDOM % 997 + 1)))
head -c ${size}m /dev/zero | tail > /dev/null