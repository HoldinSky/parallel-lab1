#include <cstdlib>
#include <chrono>
#include <random>
#include <mutex>

#include "resources.h"

static std::random_device seed;

template<typename FT>
std::chrono::duration<int64_t, std::milli>
measure_execution_time(FT func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

std::atomic<uint32_t> progress_tracker = 0;
std::mutex progress_mutex{};

void fill_matrix(
    std::vector<std::vector<uint32_t>>& matrix,
    uint32_t low_col_idx,
    uint32_t high_col_idx,
    uint32_t max_value
) {
    std::default_random_engine generator(seed());
    std::uniform_int_distribution<int> distribution(0, static_cast<int32_t>(max_value));

    auto random_value = [&] { return distribution(generator); };

    const uint32_t matrix_size = matrix.size();
    const size_t one_hundreds = matrix_size / 100;

    for (size_t i = low_col_idx; i <= high_col_idx; i++, progress_tracker++) {
        if (progress_tracker > one_hundreds) {
            progress_mutex.lock();

            add_percent_done(progress_tracker / one_hundreds);
            progress_tracker = 0;

            progress_mutex.unlock();
        }

        uint32_t column_sum = 0;
        for (size_t j = 0; j < matrix_size; j++) {
            if (i == j) continue;

            matrix[i][j] = random_value();
            column_sum += matrix[i][j];
        }

        matrix[i][i] = column_sum;
    }
}

void fill_matrix_multithreaded(
    std::vector<std::vector<uint32_t>>& matrix,
    uint32_t max_value,
    uint32_t threads_used
) {
    std::vector<std::thread> threads{};
    threads.reserve(threads_used);

    const uint32_t matrix_size = matrix.size();
    const uint32_t interval = matrix_size / threads_used;
    uint32_t remainder = matrix_size % threads_used;

    uint32_t interval_start = 0;
    uint32_t interval_end = interval_start + interval - 1;

    if (remainder) {
        interval_end++;
        remainder--;
    }

    for (size_t i = 0; i < threads_used; i++) {
        threads.emplace_back(fill_matrix, std::ref(matrix), interval_start, interval_end, max_value);

        interval_start = interval_end + 1;
        interval_end = interval_start + interval - 1;
        if (remainder) {
            interval_end++;
            remainder--;
        }
    }

    for (auto& thread : threads) thread.join();
}

void main_application(uint32_t threads_used, uint32_t matrix_size, uint32_t max_value) {
    std::vector<std::vector<uint32_t>> matrix(matrix_size, std::vector<uint32_t>(matrix_size));

    std::thread resources_monitor(monitor_resource_usage, threads_used, matrix_size);

    auto execution_time = measure_execution_time([&]() { fill_matrix_multithreaded(matrix, max_value, threads_used); });

    terminate_resource_monitor();
    resources_monitor.join();

    std::cout << "\n\nFunction executed in: " << execution_time.count() << "ms\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << R"(
            Illegal arguments list:
            Threads for process as $1 argument (min 1).
            Matrix size as $2 argument (> 0).
        )";
        return -1;
    }

    const auto threads_used = static_cast<int32_t>(std::strtol(argv[1], nullptr, 0));
    const auto matrix_size = static_cast<int32_t>(std::strtol(argv[2], nullptr, 0));
    const uint32_t max_value = 100'000;

    if (matrix_size <= 0 || threads_used <= 0) {
        std::cout << R"(
            Wrong arguments specified.
            Threads for process as $1 argument (min 1).
            Matrix size as $2 argument (> 0).
        )";
        return -1;
    }

    main_application(
        threads_used,
        static_cast<uint32_t>(matrix_size),
        static_cast<uint32_t>(max_value)
    );

    return 0;
}
