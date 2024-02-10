#ifndef LAB_RESOURCES_H
#define LAB_RESOURCES_H

#include "sys/resource.h"
#include "sys/sysinfo.h"
#include "unistd.h"

#include <iostream>
#include <thread>
#include <fstream>

#include <vector>
#include <atomic>
#include <string>

static const unsigned int cpu_threads_number = std::thread::hardware_concurrency();

void monitor_resource_usage(uint32_t threads_used, uint32_t matrix_size);
void terminate_resource_monitor();

void set_percent_done(unsigned int new_percent_done);
void add_percent_done(unsigned int delta);

void test();

#endif // LAB_RESOURCES_H