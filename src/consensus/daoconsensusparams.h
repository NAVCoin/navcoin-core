// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DAOCONSENSUSPARAMS_H
#define DAOCONSENSUSPARAMS_H

namespace Consensus
{
enum ConsensusParamType
{
    TYPE_NUMBER,
    TYPE_PERCENT,
    TYPE_NAV
};

enum ConsensusParamsPos
{
    CONSENSUS_PARAM_VOTING_CYCLE_LENGTH,

    CONSENSUS_PARAM_CONSULTATION_MIN_SUPPORT,
    CONSENSUS_PARAM_CONSULTATION_MIN_CYCLES,
    CONSENSUS_PARAM_CONSULTATION_MAX_VOTING_CYCLES,
    CONSENSUS_PARAM_CONSULTATION_MAX_SUPPORT_CYCLES,
    CONSENSUS_PARAM_CONSULTATION_REFLECTION_LENGTH,
    CONSENSUS_PARAM_CONSULTATION_MIN_FEE,

    CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_SUPPORT,
    CONSENSUS_PARAM_CONSULTATION_ANSWER_MIN_FEE,

    CONSENSUS_PARAM_PROPOSAL_MIN_QUORUM,
    CONSENSUS_PARAM_PROPOSAL_MIN_ACCEPT,
    CONSENSUS_PARAM_PROPOSAL_MIN_REJECT,
    CONSENSUS_PARAM_PROPOSAL_MIN_FEE,
    CONSENSUS_PARAM_PROPOSAL_MAX_VOTING_CYCLES,

    CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_QUORUM,
    CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_ACCEPT,
    CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_REJECT,
    CONSENSUS_PARAM_PAYMENT_REQUEST_MIN_FEE,
    CONSENSUS_PARAM_PAYMENT_REQUEST_MAX_VOTING_CYCLES,

    CONSENSUS_PARAM_FUND_SPREAD_ACCUMULATION,
    CONSENSUS_PARAM_FUND_AMOUNT_PER_BLOCK,

    CONSENSUS_PARAM_STAKING_STATIC_REWARD,

    CONSENSUS_PARAM_NAVNS_FEE,

    MAX_CONSENSUS_PARAMS
};

static std::string sConsensusParamsDesc[Consensus::MAX_CONSENSUS_PARAMS] = {
    "Length in blocks of a voting cycle",

    "Minimum of support needed for starting a consultation",
    "Earliest cycle when a consultation can get in confirmation phase",
    "Length in cycles for consultation votings",
    "Maximum of voting cycles for a consultation to gain support",
    "Length in cycles for the reflection phase of consultations",
    "Minimum fee to submit a consultation",

    "Minimum of support needed for a consultation answer proposal",
    "Minimum fee to submit a consultation answer proposal",

    "Minimum of quorum for fund proposal votings",
    "Minimum of positive votes for a fund proposal to be accepted",
    "Minimum of negative votes for a fund proposal to be rejected",
    "Minimum fee to submit a fund proposal",
    "Maximum of voting cycles for fund proposal votings",

    "Minimum of quorum for payment request votings",
    "Minimum of positive votes for a payment request to be accepted",
    "Minimum of negative votes for a payment request to be rejected",
    "Minimum fee to submit a payment request",
    "Maximum of voting cycles for fund proposal votings",

    "Frequency of the fund accumulation transaction",
    "Amount contributed to the fund per block",

    "Staking reward per block",

    "Yearly fee for registering a name in NavNS"
};

static ConsensusParamType vConsensusParamsType[MAX_CONSENSUS_PARAMS] =
{
    TYPE_NUMBER,

    TYPE_PERCENT,
    TYPE_NUMBER,
    TYPE_NUMBER,
    TYPE_NUMBER,
    TYPE_NUMBER,
    TYPE_NAV,

    TYPE_PERCENT,
    TYPE_NAV,

    TYPE_PERCENT,
    TYPE_PERCENT,
    TYPE_PERCENT,
    TYPE_NAV,
    TYPE_NUMBER,

    TYPE_PERCENT,
    TYPE_PERCENT,
    TYPE_PERCENT,
    TYPE_NAV,
    TYPE_NUMBER,

    TYPE_NUMBER,
    TYPE_NAV,

    TYPE_NAV,
    TYPE_NAV
};
}

#endif // DAOCONSENSUSPARAMS_H
