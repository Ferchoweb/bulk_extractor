/*
 *
 */

#ifndef _BULK_EXTRACTOR_H_
#define _BULK_EXTRACTOR_H_

#include "be20_api/scanner_set.h"
#include "be20_api/aftimer.h"
#include "notify_thread.h"

#include <ostream>

#define BE_PHASE_1 1 // reading input
#define BE_PHASE_2 2 // cleaning up
#define BE_PHASE_3 3 // shutdown

[[noreturn]] void debug_help();
void validate_path(const std::filesystem::path fn);
void bulk_extractor_set_debug();
extern bool RUNNING_UNDER_CATCH;
int bulk_extractor_main(std::ostream &cout, std::ostream &cerr, int argc,char * const *argv);

#endif
