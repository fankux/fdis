#!/bin/bash

rm *.cc *.h

cd ..

protoc --cpp_out=. ./proto/modelService.proto
protoc --cpp_out=. ./proto/meta.proto
