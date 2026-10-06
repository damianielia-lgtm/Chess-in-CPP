#include <chrono>
#include <memory>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include <random>
#include <vector>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace std::chrono;

inline void const volatile* volatile escape_sink = nullptr;

template <typename T>
inline void do_not_optimize(T& value) noexcept {
    escape_sink = static_cast<void const volatile*>(std::addressof(value));
    std::atomic_signal_fence(std::memory_order_seq_cst);
}

struct ScoredMove {
    std::uint16_t move;
    std::int16_t score;
};

static constexpr std::size_t kMaxCapacity = 256;

class ScoredMoveList {
public:
    ScoredMoveList() noexcept = default;

    void push(ScoredMove move) noexcept {
        if (count_ < moves_.size()) {
            moves_[count_++] = move;
        }
    }

    using iterator = std::array<ScoredMove, kMaxCapacity>::iterator;
    using const_iterator = std::array<ScoredMove, kMaxCapacity>::const_iterator;

    iterator begin() noexcept { return moves_.begin(); }
    iterator end() noexcept { return moves_.begin() + count_; }
    const_iterator begin() const noexcept { return moves_.begin(); }
    const_iterator end() const noexcept { return moves_.begin() + count_; }

    std::size_t size() const noexcept { return count_; }

    // Fast O(1) removal by swapping the target with the last element
    void remove_at(iterator it) noexcept {
        if (count_ > 0 && it >= begin() && it < end()) {
            *it = moves_[--count_];
        }
    }

    void pop_back() noexcept { count_--; }

private:
    std::array<ScoredMove, kMaxCapacity> moves_{};
    std::size_t count_ = 0;
};

class Presort {
public:
    explicit Presort(const ScoredMoveList& moves) : list_(moves) {
        std::sort(
            list_.begin(), list_.end(), [](const ScoredMove& a, const ScoredMove& b) {
                return a.score < b.score; // ascending: best move ends up last
            }
        );
    }

    std::uint16_t next_move() {
        ScoredMove best_move = *(list_.end() - 1);
        list_.pop_back();
        return best_move.move;
    }

private:
    ScoredMoveList list_;
};

class LazySelect {
public:
    explicit LazySelect(const ScoredMoveList& moves) : list_(moves) {}

    std::uint16_t next_move() {
        auto best_it = std::max_element(list_.begin(), list_.end(),
            [](const ScoredMove& a, const ScoredMove& b) {
                return a.score < b.score;
            }
        );

        ScoredMove best_move = *best_it;
        list_.remove_at(best_it);

        return best_move.move;
    }

private:
    ScoredMoveList list_;
};

// Benchmark configuration
constexpr int MaxMoves = 100; // Sweep move-list size N = 1..128
constexpr int PoolSize = 500; // distinct pre-generated boards per N
constexpr int BatchSize = 128; // trials timed together per sample
constexpr int Samples = 32; // timed samples per algorithm per (N, k)
constexpr int WarmupBatches = 2; // untimed batches to prime cache/branch predictor
constexpr int TotalTrials = Samples * BatchSize;
constexpr int FullPasses = 3; // re-run the whole thing, to find minimal external overhead

std::mt19937 g_rng(1337);

ScoredMoveList generate_test_case(int N) {
    static std::uniform_int_distribution<std::uint16_t> moves_distr(0, 65535);
    static std::uniform_int_distribution<std::int16_t> score_distr(-32768, 32767);

    ScoredMoveList test_case;
    for (int i = 0; i < N; ++i) {
        test_case.push({moves_distr(g_rng), score_distr(g_rng)});
    }

    return test_case;
}

std::vector<ScoredMoveList> build_pool(int N) {
    std::vector<ScoredMoveList> pool;
    pool.reserve(PoolSize);
    for (int i = 0; i < PoolSize; ++i) {
        pool.push_back(generate_test_case(N));
    }

    return pool;
}

template <typename Algo>
double run_batch(
    const std::vector<ScoredMoveList>& board,
    int k,
    std::uint64_t& checksum
) {
    auto start = steady_clock::now();

    for (int t = 0; t < BatchSize; t++) {
        Algo algo(board[t]);

        if (k == 0) {
            do_not_optimize(algo);
        } else {
            for (int i = 0; i < k; i++) {
                checksum += algo.next_move();
            }
        }
    }

    auto end = steady_clock::now();

    return duration_cast<nanoseconds>(end - start).count();
}

struct BenchmarkResult {
    std::vector<double> presort_values;
    std::vector<double> lazyselect_values;
};

BenchmarkResult benchmark_point(
    std::vector<ScoredMoveList>& pool,
    int k,
    std::uint64_t& presort_checksum,
    std::uint64_t& lazyselect_checksum
) {
    std::vector<ScoredMoveList> warmup_board(pool.begin(), pool.begin() + BatchSize);

    for (int w = 0; w < WarmupBatches; w++) {
        run_batch<Presort>(warmup_board, k, presort_checksum);
        run_batch<LazySelect>(warmup_board, k, lazyselect_checksum);
    }
    
    BenchmarkResult result;

    for (int sample = 0; sample < Samples; sample++) {
        std::shuffle(pool.begin(), pool.end(), g_rng);
        std::vector<ScoredMoveList> sample_board(pool.begin(), pool.begin() + BatchSize);

        if (sample % 2 == 0) {
            result.presort_values.push_back(
                run_batch<Presort>(sample_board, k, presort_checksum) / BatchSize
            );

            result.lazyselect_values.push_back(
                run_batch<LazySelect>(sample_board, k, lazyselect_checksum) / BatchSize
            );
        } else {
            result.lazyselect_values.push_back(
                run_batch<LazySelect>(sample_board, k, lazyselect_checksum) / BatchSize
            );

            result.presort_values.push_back(
                run_batch<Presort>(sample_board, k, presort_checksum) / BatchSize
            );
        }
    }

    return result;
}

double find_median(std::vector<double>& v) {
    auto mid = v.begin() + v.size() / 2;
    std::nth_element(v.begin(), mid, v.end());

    if (v.size() % 2 != 0) {
        return *mid;
    } else {
        auto lower_mid = std::max_element(v.begin(), mid);
        return (*mid + *lower_mid) / 2.0;
    }
}

int main() {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadAffinityMask(GetCurrentThread(), 1ull << 2);

    std::vector<int> N_distr(MaxMoves);
    std::iota(N_distr.begin(), N_distr.end(), 1);

    std::uint64_t presort_checksum = 0;
    std::uint64_t lazyselect_checksum = 0;

    std::array<std::array<std::vector<double>, MaxMoves + 1>, MaxMoves + 1> presort_times{};
    std::array<std::array<std::vector<double>, MaxMoves + 1>, MaxMoves + 1> lazyselect_times{};

    for (int N = 1; N <= MaxMoves; N++) {
        for (int k = 0; k <= N; k++) {
            presort_times[N][k].reserve(FullPasses * Samples);
            lazyselect_times[N][k].reserve(FullPasses * Samples);
        }
    }

    std::cout << "Benchmarking...\n";

    for (int pass = 1; pass <= FullPasses; pass++) {
        std::cout << "Pass " << std::to_string(pass) << '/' << std::to_string(FullPasses) << '\n';

        std::shuffle(N_distr.begin(), N_distr.end(), g_rng);

        for (int N : N_distr) {
            std::vector<ScoredMoveList> pool = build_pool(N);

            std::vector<int> k_distr(N + 1);
            std::iota(k_distr.begin(), k_distr.end(), 0);
            std::shuffle(k_distr.begin(), k_distr.end(), g_rng);

            for (int k : k_distr) {
                BenchmarkResult Nk_point = benchmark_point(
                    pool,
                    k,
                    presort_checksum,
                    lazyselect_checksum
                );

                presort_times[N][k].insert(
                    presort_times[N][k].end(),
                    Nk_point.presort_values.begin(),
                    Nk_point.presort_values.end()
                );
                lazyselect_times[N][k].insert(
                    lazyselect_times[N][k].end(),
                    Nk_point.lazyselect_values.begin(),
                    Nk_point.lazyselect_values.end()
                );
            }

            std::cout << "N = " << std::left << std::setw(3) << N << ".... ";
        }

        std::cout << '\n';
    }

    std::ofstream output{"picker_bench.csv"};
    output << "N,k,presort,lazyselect\n";
    
    for (int N = 1; N <= MaxMoves; N++) {
        for (int k = 0; k <= N; k++) {
            output << std::to_string(N) << ','
                << std::to_string(k) << ','
                << std::to_string(find_median(presort_times[N][k])) << ','
                << std::to_string(find_median(lazyselect_times[N][k])) << '\n';
        }
    }

    return 0;
}
