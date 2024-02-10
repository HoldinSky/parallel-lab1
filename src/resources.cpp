#include "resources.h"

typedef struct resource_usage {
    uint64_t total_memory_gb;
    uint64_t total_memory_used_gb;
    uint64_t memory_mb;
    double cpu_percentage;
    double total_cpu_percentage;
} ResUsage;

struct CPUStat {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guest_nice;
};

std::vector<std::string> split(const std::string& txt, char ch) {
    std::vector<std::string> strs;

    size_t pos = txt.find(ch);
    size_t initialPos = 0;

    // Decompose statement
    while (pos != std::string::npos) {
        auto value = txt.substr(initialPos, pos - initialPos);

        if (!value.empty())
            strs.push_back(value);

        initialPos = pos + 1;
        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

    return strs;
}

void fill_cpu_stat_from_vec(const std::vector<std::string>& src, CPUStat* dst) {
    auto ptr = reinterpret_cast<uint64_t*>(dst);
    auto end_ptr = ptr + sizeof(CPUStat);

    for (size_t i = 0; i < src.size() && ptr != end_ptr; i++, ptr++)
        *ptr = static_cast<uint64_t>(std::strtoull(src[i].c_str(), nullptr, 0));
}

CPUStat get_cpu_stat() {
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        std::cerr << "Error opening /proc/stat" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    std::getline(stat_file, line);

    std::string cpu_label;
    CPUStat stat{};

    auto time_values = split(line.substr(5), ' ');
    fill_cpu_stat_from_vec(time_values, &stat);

    stat_file.close();
    return stat;
}

ResUsage capture_resource_usage() {
    struct rusage start_usage{}, usage{};

    ResUsage res_usage{ 0, 0, 0, -1.0, -1.0 };

    if (getrusage(RUSAGE_SELF, &start_usage) != 0) {
        std::cerr << "Error getting resource usage information." << std::endl;
        return res_usage;
    }

    auto start_stat = get_cpu_stat();

    usleep(1'000'000);

    auto end_stat = get_cpu_stat();

    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        std::cerr << "Error getting resource usage information." << std::endl;
        return res_usage;
    }

    double elapsed = 1.0;
    double cpu_time_diff = (usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec);

    res_usage.cpu_percentage = (cpu_time_diff / elapsed / static_cast<double>(cpu_threads_number)) * 100.0;
    res_usage.memory_mb = usage.ru_maxrss / 1024;

    struct sysinfo mem_info{};
    sysinfo(&mem_info);

    res_usage.total_memory_gb = mem_info.totalram / 1024 / 1024;
    res_usage.total_memory_used_gb = res_usage.total_memory_gb - mem_info.freeram / 1024 / 1024;

    uint64_t start_total_time = 0;
    uint64_t end_total_time = 0;

    auto begin_ptr = reinterpret_cast<uint64_t*>(&start_stat);
    auto end_ptr = begin_ptr + sizeof(start_stat) / sizeof(uint64_t);

    for (uint64_t* i = begin_ptr; i != end_ptr; i++)
        start_total_time += *i;

    begin_ptr = reinterpret_cast<uint64_t*>(&end_stat);
    end_ptr = begin_ptr + sizeof(end_stat) / sizeof(uint64_t);

    for (uint64_t* i = begin_ptr; i != end_ptr; i++)
        end_total_time += *i;

    uint64_t start_work_time = start_total_time - start_stat.idle;
    uint64_t end_work_time = end_total_time - end_stat.idle;

    double total_diff = end_total_time - start_total_time;
    double work_diff = end_work_time - start_work_time;

    res_usage.total_cpu_percentage = work_diff / total_diff * 100.0;

    return res_usage;
}

std::atomic<unsigned int> percent_done(0);

void set_percent_done(unsigned int new_percent_done) {
    percent_done = new_percent_done;
    if (percent_done >= 98) percent_done = 100;
}

void add_percent_done(unsigned int delta) {
    percent_done += delta;
    if (percent_done >= 98) percent_done = 100;
}

std::atomic<bool> resource_usage_terminate_flag(false);

void monitor_resource_usage(uint32_t threads_used) {
    while (!resource_usage_terminate_flag) {
        const ResUsage usage = capture_resource_usage();

        if (usage.cpu_percentage < 0 || usage.memory_mb < 0) continue;

         std::cout << "\033[2J\033[1;1H";

         std::cout << "Threads used: " << threads_used << "\n\n";

         std::cout << "Total memory used: " << usage.total_memory_used_gb << " / " << usage.total_memory_gb << " Gb\n";
         printf("Total CPU usage: %.2f%%\n", usage.total_cpu_percentage);

         std::cout << "Process memory usage: " << usage.memory_mb << " Mb\n";
         printf("Process CPU used: %.2f%%\n\n", usage.cpu_percentage);

         std::cout << "Percent done: " << percent_done << "%\n";
    }
}

void terminate_resource_monitor() {
    resource_usage_terminate_flag = true;
}

void test() {
    std::string line = "cpu  376589 2129 99210 7415704 10041 0 61485 0 0 0";
    CPUStat stat{};

    auto time_values = split(line.substr(3), ' ');

    fill_cpu_stat_from_vec(time_values, &stat);

    std::cout << "get_cpu_stat :: CpuStat: " << stat.user << " " << stat.nice << " " << stat.system << " " << stat.idle << " " << stat.iowait
        << " " << stat.irq << " " << stat.softirq << " " << stat.steal << " " << stat.guest << " " << stat.guest_nice << "\n\n";
}