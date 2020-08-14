#!/bin/bash
echo "[top before run]"
top -bn1
echo "[Running]"
time ./core
echo "[top after run]"
top -bn2