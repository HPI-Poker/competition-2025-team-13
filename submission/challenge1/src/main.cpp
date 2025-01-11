#include <skeleton/actions.h>
#include <skeleton/constants.h>
#include <skeleton/runner.h>
#include <skeleton/states.h>
#include <chrono>
#include "util.cpp"

using namespace pokerbots::skeleton;

struct Bot {
    double ev = 0;
    /*
      Called when a new round starts. Called NUM_ROUNDS times.

      @param gameState The GameState object.
      @param roundState The RoundState object.
      @param active Your player's index.
    */
    void handleNewRound(GameInfoPtr gameState, RoundStatePtr roundState, int active) {
        // int myBankroll = gameState->bankroll;  // the total number of chips you've gained or lost from the beginning of the game to the start of this round
        // float gameClock = gameState->gameClock;  // the total number of seconds your bot has left to play this game
        // int roundNum = gameState->roundNum;  // the round number from 1 to State.NUM_ROUNDS
        // auto myCards = roundState->hands[active];  // your cards
        // bool bigBlind = (active == 1);  // true if you are the big blind
    }

    /*
      Called when a round ends. Called NUM_ROUNDS times.

      @param gameState The GameState object.
      @param terminalState The TerminalState object.
      @param active Your player's index.
    */
    void handleRoundOver(GameInfoPtr gameState, TerminalStatePtr terminalState, int active) {
        cerr << gameState->roundNum << ": " << gameState->bankroll << " " << gameState->gameClock << " " << ev << endl;
    }

    Action checkCall(const auto &legalActions) {
        return legalActions.contains(Action::Type::CHECK) ? Action{Action::Type::CHECK} : Action{Action::Type::CALL};
    }

    static bool isFinalStreet(vector<string> board) {
        if (size(board) < 5) return false;
        char last_rank = board.back()[0];
        return last_rank != 'J' && last_rank != 'Q' && last_rank != 'K';
    }

    Action getAction(GameInfoPtr gameState, RoundStatePtr roundState, int active) {
        if (roundState->stacks[active] == 0) return Action{Action::Type::CHECK};
        auto legalActions = roundState->legalActions();
        if (roundState->stacks[1 - active] > 0) {
            if (isFinalStreet(roundState->deck))
                return Action{Action::Type::RAISE, 2};
            return checkCall(legalActions);
        }
        int rounds_wo_all_in;
        if (roundState->street >= 3) {
            rounds_wo_all_in = (1 - active) - 1;
            rounds_wo_all_in += roundState->street - 2;
        } else {
            rounds_wo_all_in = active - 1;
        }

        int rounds_left = NUM_ROUNDS - gameState->roundNum;
        auto duration = chrono::nanoseconds((long long)(1e9 * (gameState->gameClock-1) / rounds_left));

        double foldEv = -roundState->pips[active];
        double cEv = callEv(rounds_wo_all_in, roundState->hands[active], roundState->deck, duration);
        assert(cEv <= 100);
        ev += max(cEv, foldEv);
        const double risk = 0;
        if (cEv - foldEv > risk)
            return Action{Action::Type::CALL};
        return Action{Action::Type::FOLD};
    }
};

int main(int argc, char *argv[]) {
    auto [host, port] = parseArgs(argc, argv);
    runBot<Bot>(host, port);
    return 0;
}