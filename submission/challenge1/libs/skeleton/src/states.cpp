#include "../include/skeleton/states.h"

#include <algorithm>
#include <numeric>

#include <fmt/ranges.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "../include/skeleton/util.h"

namespace pokerbots::skeleton {

StatePtr RoundState::showdown() const {
  return std::make_shared<TerminalState>(std::array<int, 2>{0, 0}, getShared());
}

std::unordered_set<Action::Type> RoundState::legalActions() const {
  auto active = getActive(button);
  auto continueCost = pips[1-active] - pips[active];
  if (continueCost == 0) {
    // we can only raise the stakes if both players can afford it
    auto betsForbidden = stacks[0] == 0 || stacks[1] == 0;
    return betsForbidden ? std::unordered_set<Action::Type>{Action::Type::CHECK}
                         : std::unordered_set<Action::Type>{
                               Action::Type::CHECK, Action::Type::RAISE};
  }
  // continueCost > 0
  // similarly, re-raising is only allowed if both players can afford it
  auto raisesForbidden = continueCost == stacks[active] || stacks[1-active] == 0;
  return raisesForbidden
             ? std::unordered_set<Action::Type>{Action::Type::FOLD,
                                                Action::Type::CALL}
             : std::unordered_set<Action::Type>{
                   Action::Type::FOLD, Action::Type::CALL, Action::Type::RAISE};
}

std::array<int, 2> RoundState::raiseBounds() const {
  auto active = getActive(button);
  auto continueCost = pips[1-active] - pips[active];
  auto maxContribution = std::min(stacks[active], stacks[1-active] + continueCost);
  auto minContribution = std::min(maxContribution, continueCost + std::max(continueCost, BIG_BLIND));
  return {pips[active] + minContribution, pips[active] + maxContribution};
}

StatePtr RoundState::proceedStreet() const {
  auto newStreet = street == 0 ? 3 : street + 1;
  return std::make_shared<RoundState>(1, newStreet, std::array<int, 2>{0, 0}, stacks, hands, deck, getShared());
}

StatePtr RoundState::proceed(Action action) const {
  auto active = getActive(button);
  switch (action.actionType) {
    case Action::Type::FOLD: {
      auto delta = active == 0 ? stacks[0] - STARTING_STACK : STARTING_STACK - stacks[1];
      return std::make_shared<TerminalState>(std::array<int, 2>{delta, -1 * delta}, getShared());
    }
    case Action::Type::CALL: {
      if (button == 0) {  // sb calls bb
        return std::make_shared<RoundState>(
            1, 0, std::array<int, 2>{BIG_BLIND, BIG_BLIND},
            std::array<int, 2>{STARTING_STACK - BIG_BLIND,
                               STARTING_STACK - BIG_BLIND},
            hands, deck, getShared());
      }
      // both players acted
      auto newPips = pips;
      auto newStacks = stacks;
      auto contribution = newPips[1-active] - newPips[active];
      newStacks[active] = newStacks[active] - contribution;
      newPips[active] = newPips[active] + contribution;
      auto state = std::make_shared<RoundState>(button + 1, street, std::move(newPips), std::move(newStacks),
                                                hands, deck, getShared());
      return state->proceedStreet();
    }
    case Action::Type::CHECK: {
      if ((street == 0 && button > 0) || button > 1) {
        return this->proceedStreet();
      }
      // let opponent act
      return std::make_shared<RoundState>(button + 1, street, pips, stacks, hands, deck, getShared());
    }
    default: {  // Action::Type::RAISE
      auto newPips = pips;
      auto newStacks = stacks;
      auto contribution = action.amount - newPips[active];
      newStacks[active] = newStacks[active] - contribution;
      newPips[active] = newPips[active] + contribution;
      return std::make_shared<RoundState>(button + 1, street, std::move(newPips), std::move(newStacks), hands, deck, getShared());
    }
  }
}

std::ostream &RoundState::doFormat(std::ostream &os) const {
  std::array<std::string, 2> formattedHands = {
      fmt::format(FMT_STRING("{}"),
                  fmt::join(hands[0].begin(), hands[0].end(), "")),
      fmt::format(FMT_STRING("{}"),
                  fmt::join(hands[1].begin(), hands[1].end(), ""))};

  fmt::print(os,
             FMT_STRING("round(button={}, street={}, pips=[{}], stacks=[{}], hands=[{}], "
                        "deck=[{}])"),
             button, street, fmt::join(pips.begin(), pips.end(), ", "),
             fmt::join(stacks.begin(), stacks.end(), ", "),
             fmt::join(formattedHands.begin(), formattedHands.end(), ","),
             fmt::join(deck.begin(), deck.end(), ", "));
  return os;
}

std::ostream &TerminalState::doFormat(std::ostream &os) const {
  fmt::print(os, FMT_STRING("terminal(deltas=[{}])"),
             fmt::join(deltas.begin(), deltas.end(), ", "));
  return os;
}

} // namespace pokerbots::skeleton
