#include <cstdlib>
#include <chrono>
#include <ctime>

#include "resources.h"

template<typename FT>
std::chrono::duration<int64_t, std::milli>
measure_execution_time(FT func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

void fill_matrix(std::vector<std::vector<uint32_t>> &matrix, uint32_t max_value) {
    std::srand(std::time(nullptr));

    const size_t one_hundreds = matrix.size() / 100;

    uint32_t progress_tracker = 0;

    for (uint32_t i = 0; i < matrix.capacity(); i++, progress_tracker++) {
        if (progress_tracker > one_hundreds) {
            add_percent_done(progress_tracker / one_hundreds);
            progress_tracker = 0;
        }

        long long column_sum = 0;
        for (uint32_t j = 0; j < matrix.capacity(); j++) {
            matrix[i].emplace_back();
            if (i == j) continue;

            matrix[i][j] = std::rand() % max_value;
            column_sum += matrix[i][j];
        }

        matrix[i][i] = column_sum;
    }
}

void main_application(uint32_t threads_used, uint32_t matrix_size, uint32_t max_value) {
    std::vector<std::vector<uint32_t>> matrix;
    matrix.reserve(matrix_size);

    for (uint32_t i = 0; i < matrix_size; i++) {
        matrix.emplace_back();
        matrix.back().reserve(matrix_size);
    }

    std::thread resources_monitor(monitor_resource_usage, threads_used);

    auto execution_time = measure_execution_time([&]() { fill_matrix(matrix, max_value); });

    terminate_resource_monitor();
    resources_monitor.join();

    std::cout << "\n\nFunction executed in: " << execution_time.count() << "ms\n";
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << R"(
            Illegal arguments list:
            Threads for process as $1 argument (min 1).
            Matrix size as $2 argument (> 0).
            Max value of matrix element as $3 argument (> 0).
        )";
        return -1;
    }

    const auto threads_used = static_cast<int32_t>(std::strtol(argv[1], nullptr, 0));
    const auto matrix_size = static_cast<int32_t>(std::strtol(argv[2], nullptr, 0));
    const auto max_value = static_cast<int32_t>(std::strtol(argv[3], nullptr, 0));

    if (matrix_size <= 0 || max_value <= 0 || threads_used <= 0) {
        std::cout << R"(
            Wrong arguments specified.
            Threads for process as $1 argument (min 1).
            Matrix size as $2 argument (> 0).
            Max value of matrix element as $3 argument (> 0).
        )";
        return -1;
    }

    main_application(threads_used, matrix_size, max_value);

    return 0;
}