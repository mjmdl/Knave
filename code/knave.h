#ifndef KNAVE_H
#define KNAVE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define K_MILISECONDS_PER_SECOND (1000)
#define K_NANOSECONDS_PER_SECOND (1000 * 1000 * 1000)

#define K_FN(NAME) NAME##_Fn

#define KDEBUG_ENABLED 1
#define KTRACE_ENABLED 1

#define KFATAL(...) \
	do \
	{ \
		fprintf(stderr, "FATAL: " __VA_ARGS__); \
		fputc('\n', stderr); \
	} \
	while (0)

#define KERROR(...) \
	do \
	{ \
		fprintf(stderr, "ERROR: " __VA_ARGS__); \
		fputc('\n', stderr); \
	} \
	while (0)

#define KWARN(...) \
	do \
	{ \
		fprintf(stderr, "WARN: " __VA_ARGS__); \
		fputc('\n', stderr); \
	} \
	while (0)

#define KINFO(...) \
	do \
	{ \
		fprintf(stdout, "INFO: " __VA_ARGS__); \
		fputc('\n', stdout); \
	} \
	while (0)

#if KDEBUG_ENABLED == 1
#   define KDEBUG(...) \
	do \
	{ \
		fprintf(stdout, "DEBUG: " __VA_ARGS__); \
		fputc('\n', stdout); \
	} \
	while (0)
#else
#   define KDEBUG(...)
#endif

#if KTRACE_ENABLED == 1
#   define KTRACE(...) \
	do \
	{ \
		fprintf(stdout, "TRACE: " __VA_ARGS__); \
		fputc('\n', stdout); \
	} \
	while (0)
#else
#   define KTRACE(...)
#endif

typedef uint8_t Ku8;
typedef uint16_t Ku16;
typedef uint32_t Ku32;
typedef uint64_t Ku64;
typedef int8_t Ks8;
typedef int16_t Ks16;
typedef int32_t Ks32;
typedef int64_t Ks64;
typedef float Kf32;
typedef double Kf64;

typedef struct Kos_File
{
	const char *path;
	char *content;
	size_t size;
}
Kos_File;

bool kos_read_entire_file(Kos_File *file);
void kos_free_file(Kos_File *file);

#endif // KNAVE_H
