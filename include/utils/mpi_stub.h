#pragma once

// MPI stub header for development without MPI
// This provides basic MPI types and functions for compilation

#include <cstdint>

// MPI constants
#define MPI_COMM_WORLD 0
#define MPI_SUCCESS 0
#define MPI_ANY_SOURCE -1
#define MPI_ANY_TAG -1
#define MPI_THREAD_SINGLE 0
#define MPI_THREAD_FUNNELED 1
#define MPI_THREAD_SERIALIZED 2
#define MPI_THREAD_MULTIPLE 3

// MPI data types
typedef int MPI_Datatype;
typedef int MPI_Comm;
typedef int MPI_Status;
typedef int MPI_Request;
typedef int MPI_Op;
typedef int MPI_Info;

// MPI predefined types
#define MPI_INT 1
#define MPI_DOUBLE 2
#define MPI_CHAR 3
#define MPI_BYTE 4
#define MPI_FLOAT 5
#define MPI_LONG 6
#define MPI_LONG_LONG 7
#define MPI_UNSIGNED_LONG 8
#define MPI_UNSIGNED_LONG_LONG 9

// MPI operations
#define MPI_SUM 1
#define MPI_MAX 2
#define MPI_MIN 3
#define MPI_PROD 4

// MPI functions (stubs that do nothing)
inline int MPI_Init(int* argc, char*** argv) { return MPI_SUCCESS; }
inline int MPI_Init_thread(int* argc, char*** argv, int required, int* provided) { *provided = required; return MPI_SUCCESS; }
inline int MPI_Finalize() { return MPI_SUCCESS; }
inline int MPI_Comm_rank(MPI_Comm comm, int* rank) { *rank = 0; return MPI_SUCCESS; }
inline int MPI_Comm_size(MPI_Comm comm, int* size) { *size = 1; return MPI_SUCCESS; }
inline int MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status) { return MPI_SUCCESS; }
inline int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Barrier(MPI_Comm comm) { return MPI_SUCCESS; }
inline int MPI_Abort(MPI_Comm comm, int errorcode) { return MPI_SUCCESS; } 