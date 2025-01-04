#include "plagiarism_checker.hpp"

// Constructor for BloomFilter

template <size_t SIZE>
BloomFilter<SIZE>::BloomFilter(int numHashes) : numHashes(numHashes)
{
    bitArray.reset();
    for (auto prime : primes)
    {
        int big_exp = 1;
        for (int i = 0; i < 15; i++)
        {
            big_exp = (big_exp * prime) % SIZE;
        }
        prime_raised.push_back(big_exp);
    }
}

template <size_t SIZE>
size_t BloomFilter<SIZE>::hashFunction(const std::vector<int> &item, int start, int end, size_t prev_hash, int seed) const
{

    size_t sum = 0;
    size_t exp = 1;
    if (start == 0 || prev_hash == 0)
    {
        for (int i = start; i < end; i++)
        {
            size_t x = item[i];
            sum = (sum + x * exp) % SIZE;
            exp *= primes[seed];
            exp %= SIZE;
        }
    }
    else
    {
        sum = (prev_hash * primes[seed]) % SIZE;
        sum = (sum - (item[start - 1] * prime_raised[seed]) % SIZE + SIZE) % SIZE;
        sum = (sum + item[end - 1]) % SIZE;
    }
    return (sum + SIZE) % SIZE;
}

// Set the bits corresponding to hashes of the subsequence to 1

template <size_t SIZE>
void BloomFilter<SIZE>::insert(const std::vector<int> &item, int start, int end)
{
    for (int i = 0; i < numHashes; i++)
    {
        size_t hashValue = hashFunction(item, start, end, 0, i);
        bitArray.set(hashValue);
    }
}

// Checks if all bits of the subsequence correspond to 1

template <size_t SIZE>
bool BloomFilter<SIZE>::contains(const std::vector<int> &item, int start, int end) const

{
    for (int i = 0; i < numHashes; i++)
    {
        size_t hashValue = hashFunction(item, start, end, 0, i);
        if (!bitArray[hashValue])
        {
            return false;
        }
    }
    return true;
}

// Takes tasks from queue and puts it in avaiable threads

void plagiarism_checker_t::worker_function()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_condition.wait(lock, [&]
                                 { return stop_flag || !task_queue.empty(); });

            if (stop_flag && task_queue.empty())
            {
                break; // Exit if stopping and no more tasks
            }

            task = std::move(task_queue.front());
            task_queue.pop();
        }

        // Execute the task
        task();
    }
}

// Default constructor

plagiarism_checker_t::plagiarism_checker_t(void)
    : stop_flag(false), worker_thread(&plagiarism_checker_t::worker_function, this)
{

    next_id = 0;
    auto now = std::chrono::system_clock::now();
    startTime = std::chrono::system_clock::to_time_t(now);
}

// Constructor when original submissions are given

plagiarism_checker_t::plagiarism_checker_t(std::vector<std::shared_ptr<submission_t>> __submissions)
    : stop_flag(false), worker_thread(&plagiarism_checker_t::worker_function, this)
{

    next_id = 0;
    auto now = std::chrono::system_clock::now();
    startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();

    for (auto submission : __submissions)
    {
        times.push_back(0);
        mapping[next_id] = submission;
        isPlag.push_back(false);
        std::vector<int> tokens;
        {
            tokenizer_t tokenizer(submission->codefile);
            tokens = tokenizer.get_tokens();
        }

        BloomFilter<SMALL_SIZE> *bloom15 = new BloomFilter<SMALL_SIZE>(NUM_HASH_FUNCTIONS);

        // Adding all files into their respective bloom filters

        for (int i = 15; i <= tokens.size(); i++)
        {
            bloom15->insert(tokens, i - 15, i);
        }
        for (int i = 75; i <= tokens.size(); i++)
        {
            Bloom75_old.insert(tokens, i - 75, i);
        }
        Blooms15.push_back({next_id, bloom15});
        next_id++;
    }
}

plagiarism_checker_t::~plagiarism_checker_t(void)
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop_flag = true; // Signal the worker thread to stop
    }

    if (worker_thread.joinable())
    {
        worker_thread.join(); // Wait for the worker thread to finish
    }

    queue_condition.notify_all(); // Notify the worker thread

    for (auto bloom : Blooms15)
    {
        delete bloom.second;
    }
    Blooms15.clear();
    for (auto bloom : Blooms75_new)
    {
        delete bloom.second;
    }
    Blooms75_new.clear();

    times.clear();
    mapping.clear();
    isPlag.clear();
}

void plagiarism_checker_t::add_submission(std::shared_ptr<submission_t> __submission)
{

    auto now = std::chrono::system_clock::now();
    double currentTime = (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - startTime) / 1000.0;

    int curr_id = next_id;
    next_id++;

    times.push_back(currentTime);
    mapping[curr_id] = __submission;
    isPlag.push_back(false);

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push([=, this]()
                        {
            // Asynchronous task
            std::vector<int> tokens;
            {
                tokenizer_t tokenizer(__submission->codefile);
                tokens = tokenizer.get_tokens();
            }

            // Pop out old submissions as we don't want to tag them as plag and put them in overall 75 filter

            while (Blooms75_new.empty() == false && currentTime - times[Blooms75_new[0].first] > 1)
            {
                std::vector<int> newtokens;
                int cur_id = Blooms75_new[0].first;

                {
                    tokenizer_t tokenizer(mapping[cur_id]->codefile);
                    newtokens = tokenizer.get_tokens();
                }

                for (int i = 75; i < newtokens.size(); i++)
                {
                    Bloom75_old.insert(newtokens, i - 75, i);
                }
                // write detructr for bloom
                delete Blooms75_new.begin()->second;
                Blooms75_new.erase(Blooms75_new.begin());
            }

            // check if the submission has plagged

            process_submission(__submission, tokens, curr_id);

            BloomFilter<SMALL_SIZE> *bloom15 = new BloomFilter<SMALL_SIZE>(NUM_HASH_FUNCTIONS);
            BloomFilter<SMALL_SIZE> *bloom_new_75 = new BloomFilter<SMALL_SIZE>(NUM_HASH_FUNCTIONS);

            for (int i = 15; i < tokens.size(); i++)
            {
                bloom15->insert(tokens, i - 15, i);
            }
            for (int i = 75; i < tokens.size(); i++)
            {
                bloom_new_75->insert(tokens, i - 75, i);
            }

            Blooms15.push_back({curr_id, bloom15});
            Blooms75_new.push_back({curr_id, bloom_new_75}); });
    }
    queue_condition.notify_all(); // Notify the worker thread
}

void plagiarism_checker_t::process_submission(std::shared_ptr<submission_t> __submission, std::vector<int> &tokens, int sub_id)
{
    // Tokenize the code file

    int patchwork = 0;

    // checks for the 75 condition with old files

    for (int i = 75; i <= tokens.size(); i++)
    {
        if (!isPlag[sub_id] && Bloom75_old.contains(tokens, i - 75, i))
        {
            if (not(isPlag[sub_id]))
            {

                flagPlag(__submission);
                isPlag[sub_id] = true;
            }
        }

        // check for new files for match length 75

        for (auto bnew : Blooms75_new)
        {

            if (bnew.second->contains(tokens, i - 75, i))
            {
                if (not(isPlag[sub_id]))
                {
                    flagPlag(__submission);
                    isPlag[sub_id] = true;
                }

                if (not(isPlag[bnew.first]))
                {
                    flagPlag(mapping[bnew.first]);
                    isPlag[bnew.first] = true;
                }
            }
        }
    }

    // check for matching of size 15

    for (auto item : Blooms15)
    {
        auto bloom = *(item.second);
        int bloom_id = item.first;
        int matches = 0;
        int i = 15;
        while (i <= tokens.size())
        {
            if (bloom.contains(tokens, i - 15, i))
            {
                matches++;
                // Check for patchwork submission

                if (patchwork >= 20)
                {
                    // Raise only if it hasn't been raise already

                    if (not(isPlag[sub_id]))
                    {

                        flagPlag(__submission);
                        isPlag[sub_id] = true;
                    }
                }

                // check for plagiarism with individual files

                if (matches >= 10)
                {
                    if (not(isPlag[sub_id]))
                    {
                        flagPlag(__submission);
                        isPlag[sub_id] = true;
                    }

                    // Raise for the file which is being compared if the time difference is less than 1 second

                    if (times[sub_id] - times[bloom_id] < 1 && isPlag[bloom_id] == false)
                    {

                        flagPlag(mapping[bloom_id]);
                        isPlag[bloom_id] = true;
                    }
                }
                patchwork += 1;

                i = i + 14;
            }
            i++;
        }
    }
}

void plagiarism_checker_t::flagPlag(std::shared_ptr<submission_t> __submission)
{
    if (__submission->student)
        __submission->student->flag_student(__submission);
    if (__submission->professor)
        __submission->professor->flag_professor(__submission);
}