# SPIFFE-CPP
Experimental C++ implementation of SPIFFE Workload API with cURL backend.

**WITHOUT STUPID FAT GRPC DEPENDENCIES.**

Still under development, do not use it.

## Under the hood
- Uses cURL for HTTP/2 communication.
- Uses hand-written protobuf parser for SPIFFE data structures.
- Simulates gRPC-like interface for SPIFFE Workload API.
- Won't support `ValidateJWTSVID` because the `google.protobuf.Struct` is stupid.
- Most design and types copied from [zkonge/spiffe-rs](https://github.com/zkonge/spiffe-rs).

## Implemented
- [x] `FetchJWTSVID`.
- [x] `FetchJWTBundles`.
- [x] `FetchX509SVID`.
- [x] `FetchX509Bundles`.
- [ ] `SPIFFE ID` validator.
- [ ] user guide.
