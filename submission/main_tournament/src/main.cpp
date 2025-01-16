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
        if (canWinByFolding(gameState, roundState, active)) {
            cerr << "I can win by folding" << endl;
            return Action{Action::Type::FOLD};
        }
        auto legalActions = roundState->legalActions();
        auto rounds_left = NUM_ROUNDS - gameState->roundNum;
        auto duration = chrono::nanoseconds((long long)(1e9 * (gameState->gameClock-1) / rounds_left));

#define pip(player) (100-roundState->stacks[player])
        double foldEv = -pip(active);
        double eq = equity(roundState->hands[active], roundState->deck, 3.0/4*duration);
        auto [minRaise, maxRaise] = roundState->raiseBounds();
        if (roundState->street == 0 && eq > 0.45) {
            if (pip(active) < 5) {
                int raise = clamp(5 - pip(active), minRaise, maxRaise);
                return Action{Action::Type::RAISE, raise};
            }
        }
        int opip = pip(1 - active);
        if (roundState->street == 3) opip *= 3, opip /= 2;
        if (roundState->street >= 3) opip *= 2;
        opip = min(opip, 100);
        eq -= 0.03;
        double callEv = eq * opip - (1 - eq) * opip;
        if (callEv - foldEv > 5 && roundState->street > 0) {
            double threshold = 1.0;
            if (roundState->street == 3) threshold = 0.7;
            if (roundState->street == 4) threshold = 0.65;
            if (roundState->street >= 5) threshold = 0.6;
            int want = pow(max(0.0, (eq - threshold) / (1-threshold)), 2)*min(100, 2*pip(1 - active)); // Don't look for meaning, there is none.
            if (legalActions.contains(Action::Type::RAISE) && pip(1 - active) < want && pip(active) + minRaise <= want) {
                int raise = clamp(want - pip(active), minRaise, maxRaise);
                for (auto card : roundState->deck) cerr << card << "";
                cerr << ": " << roundState->hands[active][0] << roundState->hands[active][1] << " " << pip(active) << " -> " << pip(active) + raise << endl;
                return Action{Action::Type::RAISE, raise};
            }
            return checkCall(legalActions);
        }
        if (legalActions.contains(Action::Type::CHECK)) return Action{Action::Type::CHECK};
        cerr << pip(active) << " " << pip(1-active) << " folded" << endl;
        return Action{Action::Type::FOLD};
    }
};

int main(int argc, char *argv[]) {
    auto [host, port] = parseArgs(argc, argv);
    runBot<Bot>(host, port);
    return 0;
}