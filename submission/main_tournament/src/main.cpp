#include <skeleton/actions.h>
#include <skeleton/constants.h>
#include <skeleton/runner.h>
#include <skeleton/states.h>
#include <chrono>
#include "util.cpp"

using namespace pokerbots::skeleton;

struct Bot {
    mt19937 rng{42};
    uniform_real_distribution<double> U{0, 1};
    static void handleRoundOver(const GameInfoPtr& gameState, const TerminalStatePtr&, int) {
        cerr << gameState->roundNum << ": " << gameState->bankroll << " " << gameState->gameClock << endl;
    }
    void handleNewRound(const GameInfoPtr&, const RoundStatePtr&, int) {}
    bool canWinByFolding(const GameInfoPtr& gameState, const RoundStatePtr& roundState, int active) {
        bool is_big_blind = active;
        auto rounds_left = NUM_ROUNDS - gameState->roundNum;
        auto loss = rounds_left / 2 * SMALL_BLIND + rounds_left / 2 * BIG_BLIND;
        if (is_big_blind) loss += SMALL_BLIND;
        else loss += BIG_BLIND;
        loss += roundState->pips[active];
        return gameState->bankroll - loss > 0;
    }


    Action checkCall(const auto &legalActions) {
        return legalActions.contains(Action::Type::CHECK) ? Action{Action::Type::CHECK} : Action{Action::Type::CALL};
    }

    Action getAction(const GameInfoPtr& gameState, const RoundStatePtr& roundState, int active) {
        if (roundState->stacks[active] == 0) return Action{Action::Type::CHECK};
        if (canWinByFolding(gameState, roundState, active)) return Action{Action::Type::FOLD};
        auto legalActions = roundState->legalActions();
        auto rounds_left = NUM_ROUNDS - gameState->roundNum;
        auto duration = chrono::nanoseconds((long long)(1e9 * (gameState->gameClock-1) / rounds_left));

        double foldEv = -roundState->pips[active];
        double eq = equity(roundState->hands[active], roundState->deck, duration);
        double callEv = eq * roundState->pips[1 - active] - (1 - eq) * roundState->pips[1 - active];
        // This is dumb.
        if (callEv - foldEv > 30) {
            double minRaiseEq = 0.9;
            if (legalActions.contains(Action::Type::RAISE) && eq > minRaiseEq) {
                auto [minRaise, maxRaise] = roundState->raiseBounds();
                int raise = (eq - minRaiseEq) * (maxRaise - minRaise) + minRaise;
                raise = clamp(raise, minRaise, maxRaise);
                return Action{Action::Type::RAISE, raise};
            }
            return checkCall(legalActions);
        }
        return Action{Action::Type::FOLD};
    }
};

int main(int argc, char *argv[]) {
    auto [host, port] = parseArgs(argc, argv);
    runBot<Bot>(host, port);
    return 0;
}