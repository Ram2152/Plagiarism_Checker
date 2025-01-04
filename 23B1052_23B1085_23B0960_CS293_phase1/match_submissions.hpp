#include <array>
#include <iostream>
#include <span>
#include <vector>
#include <cmath>
#include <algorithm>

int levenshteinDist(std::vector<int> &submission1, std::vector<int> &submission2)
{

    const int size1 = int(submission1.size());
    const int size2 = int(submission2.size());

    std::vector<std::vector<int>> dp(size1 + 1, std::vector<int>(size2 + 1));

    // Initialise the matrix
    for (int i = 0; i <= size1; i++)
    {
        dp[i][0] = i;
    }

    for (int j = 0; j <= size2; j++)
    {
        dp[0][j] = j;
    }

    for (int i = 1; i <= size1; i++)
    {
        for (int j = 1; j <= size2; j++)
        {
            int cost = 0;
            if (submission1[i - 1] != submission2[j - 1])
                cost++;
            dp[i][j] = std::min({dp[i - 1][j - 1] + cost, dp[i][j - 1] + 1, dp[i - 1][j] + 1});
        }
    }

    return dp[size1][size2];
}

void findApproxMatch(std::vector<int> &submission1, std::vector<int> &submission2, int start_row, int &match_size, int &start_1, int &start_2)
{

    // Credits for algorithm: https://stackoverflow.com/questions/8139958/algorithm-to-find-edit-distance-to-all-substrings
    // https://repositorio.uchile.cl/bitstream/handle/2250/126168/Navarro_Gonzalo_Guided_tour.pdf

    const int size1 = int(submission1.size()) - start_row;
    const int size2 = int(submission2.size());

    int dp[size1 + 1][size2 + 1];

    // Initialise the matrix

    for (int i = 0; i <= size1; i++)
    {
        dp[i][0] = i;
    }

    // this is for disregarding all mismatches before the index we need which allows us to take all suffixes of submission 2

    for (int j = 0; j <= size2; j++)
    {
        dp[0][j] = 0;
    }

    for (int i = 1; i <= size1; i++)
    {
        for (int j = 1; j <= size2; j++)
        {
            int cost = 0;
            if (submission1[i + start_row - 1] != submission2[j - 1])
                cost++;
            dp[i][j] = std::min({dp[i - 1][j - 1] + cost, dp[i][j - 1] + 1, dp[i - 1][j] + 1});
        }
    }

    // Checking for the minimum in a particular row

    for (int i = size1; i >= 15; i--)
    {
        int min_so_far = size1, min_index = 0;

        // We start the iteration from i because the values of j from 1 to i-1 will have lesser number of tokens compared to the other substring
        // Accuracy gets a hit, but this speeds it up

        for (int j = i; j <= size2; j++)
        {
            if (dp[i][j] <= min_so_far)
            {
                min_so_far = dp[i][j];
                min_index = j;
            }
        }

        // Accuracy 96% match 

        if (double(min_so_far) / i <= 0.04)
        {
            match_size = i;
            start_1 = start_row;
            start_2 = min_index - i;

            // break because we want the latest min value

            break;
        }
    }
}

// Finds The total length of all matchings >= size (length = 10) between submission1 and submission2

int findExactMatchLength(std::vector<int> &submission1, std::vector<int> &submission2, int length = 10)
{
    int total_length = 0;

    int n = submission1.size();
    int m = submission2.size();

    // matched vector to keep track of elements that occur in a match
    // This ensures that we don't count the same match twice

    std::vector<bool> matched1(n, false);
    std::vector<bool> matched2(m, false);

    // i,j are starting indices for submission1 and submission2 respectively

    for (int i = 0; i <= n - length; i++)
    {
        for (int j = 0; j <= m - length; j++)
        {

            // We find the match length between submission1 starting from index i to
            // submission 2 starting from index j

            int curr_match_length = 0;

            while ((i + curr_match_length < n) and (j + curr_match_length < m) and
                   (submission1[i + curr_match_length] == submission2[j + curr_match_length]) and
                   !matched1[i + curr_match_length] and !matched2[j + curr_match_length])
            {
                curr_match_length++;
            }

            if (curr_match_length >= length)
            {
                total_length += curr_match_length;

                // We mark the matched elements to true to avoid repetitions

                for (int k = 0; k < curr_match_length; k++)
                {
                    matched1[i + k] = true;
                    matched2[j + k] = true;
                }

                i += curr_match_length;
                break;
            }
        }
    }
    return total_length;
}

int longestCommonSubvector(const std::vector<int> &submission1, const std::vector<int> &submission2)
{
    // Credits: https://en.wikipedia.org/wiki/Longest_common_subsequence

    int m = submission1.size();
    int n = submission2.size();

    std::vector<std::vector<int>> common(m + 1, std::vector<int>(n + 1, 0));

    // Initialising

    for (int i = 0; i <= m; ++i)
    {
        common[i][0] = 0;
    }
    for (int j = 0; j <= n; ++j)
    {
        common[0][j] = 0;
    }

    // Usign LCS algorithm, we fill in the matrix

    for (int i = 1; i <= m; ++i)
    {
        for (int j = 1; j <= n; ++j)
        {
            if (submission1[i - 1] == submission2[j - 1])
            {
                common[i][j] = common[i - 1][j - 1] + 1;
            }
            else
            {
                common[i][j] = std::max(common[i][j - 1], common[i - 1][j]);
            }
        }
    }

    return common[m][n]; // Return the length of LCS
}

std::array<int, 5> match_submissions(std::vector<int> &submission1,
                                     std::vector<int> &submission2)
{
    // TODO: Write your code here
    std::array<int, 5> result = {0, 0, 0, 0, 0};

    // Fuzzy match assumes that submission 1 size < submission 2 size

    if (submission1.size() > submission2.size())
    {
        auto temp = submission1;
        submission1 = submission2;
        submission2 = temp;
    }

    result[1] = findExactMatchLength(submission1, submission2);

    int ld = levenshteinDist(submission1, submission2);

    int max_match_len = 0, istart_1 = 0, istart_2 = 0;

    int max_so_far = 29, final_start_1 = 0, final_start_2 = 0;

    for (int i = 0; i < submission1.size() - 15; i++)
    {
        findApproxMatch(submission1, submission2, i, max_match_len, istart_1, istart_2);
        if (max_match_len > max_so_far)
        {
            max_so_far = max_match_len;
            final_start_1 = istart_1;
            final_start_2 = istart_2;
        }

        max_match_len = 0;
        istart_1 = 0;
        istart_2 = 0;
    }

    // Calculate lcs to determine plag condition

    int lcs = longestCommonSubvector(submission1, submission2);

    if ( max_so_far != 29 ) result[2] = max_so_far;
    result[3] = final_start_1;
    result[4] = final_start_2;

    // determines plag based on decision from https://ieeexplore.ieee.org/document/8054419

    double decision = double(lcs) / (ld + lcs);

    if (decision > 0.5)
        result[0] = 1;

    return result;
}
