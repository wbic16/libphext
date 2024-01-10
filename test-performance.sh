#!/bin/bash
if [ ! -f ./phextio ]; then
  make
fi
time ./phextio 1.1.1/1.1.1/1.1.2 small.phext
time ./phextio 5.5.5/4.3.2/5.1.4 medium.phext
time ./phextio 1.1.1/1.1.1/3.1.38 the-odyssey.phext
