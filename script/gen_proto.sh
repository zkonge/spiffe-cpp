#!/bin/bash

root=$(dirname $(dirname $(realpath $0)))

protoc --cpp_out=$root/src/proto --proto_path=$root/proto $root/proto/helloworld.proto
protoc --cpp_out=$root/src/proto --proto_path=$root/proto $root/proto/workloadapi.proto
