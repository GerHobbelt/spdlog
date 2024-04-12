
#define BUILD_MONOLITHIC 1
#include "monolithic_examples.h"

#define USAGE_NAME   "threadpool"

#include "monolithic_main_internal_defs.h"

MONOLITHIC_CMD_TABLE_START()
{ "async_bench", {.fpp = spdlog_async_bench_main } },
{ "bench", { .fpp = spdlog_bench_main } },
{ "formatter_bench", { .fpp = spdlog_formatter_bench_main } },
{ "latency_bench", { .fpp = spdlog_latency_bench_main } },
{ "example", { .fpp = spdlog_example_main } },
{ "ex2", { .fpp = spdlog_example2_main } },
MONOLITHIC_CMD_TABLE_END();

#include "monolithic_main_tpl.h"
