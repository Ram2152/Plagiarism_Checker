#include "structures.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <algorithm>
#include <functional>
#include <queue>
#include <map>
#include <ctime>
#include <bitset>
#define NUM_HASH_FUNCTIONS 7
#define SMALL_SIZE 12289
#define LARGE_SIZE 98317
// -----------------------------------------------------------------------------

// You are free to add any STL includes above this comment, below the --line--.
// DO NOT add "using namespace std;" or include any other files/libraries.
// Also DO NOT add the include "bits/stdc++.h"

// OPTIONAL: Add your helper functions and classes here

// Class for Bloom filters for compactly storing information

template <size_t SIZE>
class BloomFilter
{
private:
    std::bitset<SIZE> bitArray;
    int numHashes;

public:
    BloomFilter(int numHashes);

    void insert(const std::vector<int> &item, int start, int end);

    // Check if an item might be in the Bloom filter

    bool contains(const std::vector<int> &item, int start, int end) const;

    // List of primes for 7 hash functions in the bloom filter

    std::vector<int> primes = {31, 37, 53, 73, 89, 97, 43};

    // Raising the prime to highest power for optimising the process of finding hashes

    std::vector<int> prime_raised;

    size_t hashFunction(const std::vector<int> &item, int start, int end, size_t prev_hash, int seed) const;
};

class plagiarism_checker_t
{
    // You should NOT modify the public interface of this class.
public:
    plagiarism_checker_t(void);
    plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>>
                             __submissions);
    ~plagiarism_checker_t(void);
    void add_submission(std::shared_ptr<submission_t> __submission);

protected:
    // Having a filter to check match len == 75 in all past submissions

    BloomFilter<LARGE_SIZE> Bloom75_old = BloomFilter<LARGE_SIZE>(NUM_HASH_FUNCTIONS);

    // Having filters for 15 and new submissions with 75 respectively

    std::vector<std::pair<int, BloomFilter<SMALL_SIZE> *>> Blooms15;
    std::vector<std::pair<int, BloomFilter<SMALL_SIZE> *>> Blooms75_new;

    // storing timings
    std::vector<double> times;

    // stores index to submission pointer mapping
    std::map<int, std::shared_ptr<submission_t>> mapping;

    // Keeps track of whether something has been raised as plag so far
    std::vector<bool> isPlag;

    long long startTime;
    int next_id;

    void flagPlag(std::shared_ptr<submission_t> __submission);
    void process_submission(std::shared_ptr<submission_t> __submission, std::vector<int> &tokens, int sub_id);

    // Task queue for asynchronous processing
    std::queue<std::function<void()>> task_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_condition;
    bool stop_flag;

    std::thread worker_thread; // Worker thread for processing tasks

    // Private methods
    void worker_function();

    // End TODO
};
