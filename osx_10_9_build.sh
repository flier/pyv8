#!/bin/bash
export CXXFLAGS='-std=c++11 -stdlib=libc++ -mmacosx-version-min=10.8'
export LDFLAGS=-lc++
python setup.py build
