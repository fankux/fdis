#!/bin/bash

rm *.cc *.h

cd ..

protoc --cpp_out=. ./proto/meta.proto
protoc --cpp_out=. ./proto/heartbeat.proto
protoc --cpp_out=. ./proto/ArrangerService.proto
protoc --cpp_out=. ./proto/modelService.proto
