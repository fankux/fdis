#!/bin/bash

rm *.cc *.h

cd ..

protoc --cpp_out=. ./proto/modelService.proto
