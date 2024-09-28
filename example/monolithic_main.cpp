
#define BUILD_MONOLITHIC 1
#include "monolithic_examples.h"

#define USAGE_NAME   "threadpool"

#include "monolithic_main_internal_defs.h"

MONOLITHIC_CMD_TABLE_START()
{ "async_bench", {.fa = spdlog_async_bench_main } },
{ "bench", { .fa = spdlog_bench_main } },
{ "formatter_bench", { .fa = spdlog_formatter_bench_main } },
{ "latency_bench", { .fa = spdlog_latency_bench_main } },
{ "example", { .fa = spdlog_example_main } },
{ "ex2", { .fa = spdlog_example2_main } },
MONOLITHIC_CMD_TABLE_END();

#include "monolithic_main_tpl.h"
