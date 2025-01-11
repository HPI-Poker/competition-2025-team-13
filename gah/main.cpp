#include <omp/EquityCalculator.h>
#include <omp/Hand.h>
#include <omp/Constants.h>
#include <iostream>
#include <map>
#include <random>

using namespace omp;
using namespace std;

const uint8_t JACK = 9;
const uint8_t QUEEN = 10;
const uint8_t KING = 11;

bool are_cards_suited(const array<uint8_t,2>& cards) {
    return cards[0] % SUIT_COUNT == cards[1] % SUIT_COUNT;
}

bool is_pair(const array<uint8_t,2>& cards) {
    return cards[0] / SUIT_COUNT == cards[1] / SUIT_COUNT;
}

bool are_cards_connected(const array<uint8_t,2>& cards) {
    return abs(cards[0] / (int)SUIT_COUNT - cards[1] / (int)SUIT_COUNT) == 1;
}

bool has_high_card(const array<uint8_t,2>& cards) {
    return cards[0] / SUIT_COUNT >= QUEEN || cards[1] / SUIT_COUNT >= QUEEN;
}

bool is_blind_bandit_good(const array<uint8_t,2>& hand) {
    return are_cards_suited(hand) || is_pair(hand) || are_cards_connected(hand) || has_high_card(hand);
}

mt19937 rng(0);
auto random_bit = uniform_int_distribution<uint64_t>(0, CARD_COUNT - 1);

uint64_t draw(uint64_t used, int n) {
    uint64_t result = 0;
    for (int i = 0; i < n; ++i) {
        uint64_t bit = 1ull << random_bit(rng);
        while (used & bit)
            bit = 1ull << random_bit(rng);
        result |= bit, used |= bit;
        n += i == n-1 && countTrailingZeros(bit) / SUIT_COUNT >= JACK;
    }
    return result;
}

Hand from_bitmask(uint64_t mask) {
    Hand h = Hand::empty();
    while (mask) {
        uint64_t t = mask & -mask;
        h += countTrailingZeros(t);
        mask ^= t;
    }
    return h;
}

HandEvaluator eval;
uint16_t evaluate(uint64_t hand) {
    uint16_t best = 0;
    for (uint64_t subset = hand; subset; subset = (subset - 1) & hand)
        if (popcount(subset) == 7) // TODO: this makes no sense (performance-wise), do something better
            best = max(best, eval.evaluate(from_bitmask(subset)));
    return best;
}

const double err = 2e-3;
double monte_carlo(const array<uint8_t,2>& me, const vector<array<uint8_t,2>>& opponent, uint64_t board = 0, uint64_t dead = 0) {
    dead |= 1ull << me[0];
    dead |= 1ull << me[1];
    int n = 5 - popcount(board);
    uint64_t me_hand = (1ull << me[0]) | (1ull << me[1]) | board;
    uint64_t wins = 0, total = 0;
    double prev_avg = -1;
    for (uint64_t i = 1;; ++i) {
        uint64_t guess = draw(dead, n);
        uint64_t bad = dead | board;
        auto my_score = evaluate(me_hand | guess);
        for (const auto& opp : opponent) {
            auto opp_hand = (1ull << opp[0]) | (1ull << opp[1]);
            if (opp_hand & bad) continue;
            ++total;
            opp_hand |= board;
            auto opp_score = evaluate(opp_hand | guess);
            wins += (my_score >= opp_score) + (my_score > opp_score);
        }

        // TODO: better convergence criteria
        double avg = wins / 2.0 / total;
        if (i > 100 && abs(avg - prev_avg) < err)
            return avg;
        prev_avg = avg;
    }
}

int main()
{
    vector<array<uint8_t,2>> all_hands;
    for (uint8_t i = 0; i < CARD_COUNT; ++i)
        for (uint8_t j = i + 1; j < CARD_COUNT; ++j)
            all_hands.emplace_back(array<uint8_t,2>{i, j});

    vector<array<uint8_t,2>> good_hands, bad_hands;
    for (const auto& hand : all_hands) {
        (is_blind_bandit_good(hand) ? good_hands : bad_hands).emplace_back(hand);
    }

    map<array<uint8_t,2>,double> equity_good, equity_bad;
    for (const auto& hand : all_hands) {
        equity_good[hand] = monte_carlo(hand, good_hands);
        equity_bad[hand] = monte_carlo(hand, bad_hands);
        cerr << equity_good[hand] << " " << equity_bad[hand] << "\n";
    }
}