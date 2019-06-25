// Copyright (c) 2018-2019 The NavCoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/dao.h"
#include "base58.h"
#include "main.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

void GetVersionMask(uint64_t &nProposalMask, uint64_t &nPaymentRequestMask, uint64_t &nConsultationMask, uint64_t &nConsultationAnswerMask)
{
    bool fReducedQuorum = IsReducedCFundQuorumEnabled(chainActive.Tip(), Params().GetConsensus());
    bool fAbstainVote = IsAbstainVoteEnabled(chainActive.Tip(), Params().GetConsensus());

    nProposalMask = Params().GetConsensus().nProposalMaxVersion;
    nPaymentRequestMask = Params().GetConsensus().nPaymentRequestMaxVersion;
    nConsultationMask = Params().GetConsensus().nConsultationMaxVersion;
    nConsultationAnswerMask = Params().GetConsensus().nConsultationAnswerMaxVersion;

    if (!fReducedQuorum)
    {
        nProposalMask &= ~CProposal::REDUCED_QUORUM_VERSION;
        nPaymentRequestMask &= ~CPaymentRequest::REDUCED_QUORUM_VERSION;
    }

    if (!fAbstainVote)
    {
        nProposalMask &= ~CProposal::ABSTAIN_VOTE_VERSION;
        nPaymentRequestMask &= ~CPaymentRequest::ABSTAIN_VOTE_VERSION;
    }
}

void SetScriptForCommunityFundContribution(CScript &script)
{
    script.resize(2);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
}

void SetScriptForProposalVote(CScript &script, uint256 proposalhash, int64_t vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PROP;
    script[3] = vote == -2 ? OP_REMOVE : (vote == -1 ? OP_ABSTAIN : (vote == 1 ? OP_YES : OP_NO));
    script[4] = 0x20;
    memcpy(&script[5], proposalhash.begin(), 32);
}

void SetScriptForPaymentRequestVote(CScript &script, uint256 prequesthash, int64_t vote)
{
    script.resize(37);
    script[0] = OP_RETURN;
    script[1] = OP_CFUND;
    script[2] = OP_PREQ;
    script[3] = vote == -2 ? OP_REMOVE : (vote == -1 ? OP_ABSTAIN : (vote == 1 ? OP_YES : OP_NO));
    script[4] = 0x20;
    memcpy(&script[5], prequesthash.begin(), 32);
}

bool Vote(uint256 hash, int64_t vote, bool &duplicate)
{
    AssertLockHeld(cs_main);

    std::string str = hash.ToString();

    if (mapAddedVotes.count(hash) > 0 && mapAddedVotes[hash] == vote)
        duplicate = true;

    RemoveConfigFile("voteyes", str);
    RemoveConfigFile("voteabs", str);
    RemoveConfigFile("voteno", str);
    RemoveConfigFile("addproposalvoteyes", str);
    RemoveConfigFile("addproposalvoteabs", str);
    RemoveConfigFile("addproposalvoteno", str);

    std::string strCmd = "voteno";

    if (vote == -1)
        strCmd = "voteabs";

    if (vote == 1)
        strCmd = "voteyes";

    WriteConfigFile(strCmd, str);

    mapAddedVotes[hash] = vote;

    return true;
}

bool RemoveVote(string str)
{

    RemoveConfigFile("voteyes", str);
    RemoveConfigFile("voteabs", str);
    RemoveConfigFile("voteno", str);
    RemoveConfigFile("addproposalvoteyes", str);
    RemoveConfigFile("addproposalvoteabs", str);
    RemoveConfigFile("addproposalvoteno", str);
    RemoveConfigFile("addpaymentrequestvoteyes", str);
    RemoveConfigFile("addpaymentrequestvoteabs", str);
    RemoveConfigFile("addpaymentrequestvoteno", str);
    if (mapAddedVotes.count(uint256S(str)) > 0)
        mapAddedVotes.erase(uint256S(str));
    else
        return false;
    return true;
}

bool RemoveVote(uint256 hash)
{
    return RemoveVote(hash.ToString());
}

bool IsBeginningCycle(const CBlockIndex* pindex, CChainParams params)
{
    return pindex->nHeight % params.GetConsensus().nBlocksPerVotingCycle == 0;
}

bool IsEndCycle(const CBlockIndex* pindex, CChainParams params)
{
    return (pindex->nHeight+1) % params.GetConsensus().nBlocksPerVotingCycle == 0;
}

std::map<uint256, std::pair<std::pair<int, int>, int>> vCacheProposalsToUpdate;
std::map<uint256, std::pair<std::pair<int, int>, int>> vCachePaymentRequestToUpdate;

void VoteStep(const CValidationState& state, CBlockIndex *pindexNew, const bool fUndo, CStateViewCache& view)
{
    AssertLockHeld(cs_main);

    const CBlockIndex* pindexDelete;
    if (fUndo)
    {
        pindexDelete = pindexNew;
        pindexNew = pindexNew->pprev;
        assert(pindexNew);
    }

    int64_t nTimeStart = GetTimeMicros();
    int nBlocks = (pindexNew->nHeight % Params().GetConsensus().nBlocksPerVotingCycle) + 1;
    const CBlockIndex* pindexblock = pindexNew;

    std::map<uint256, bool> vSeen;

    if (fUndo || nBlocks == 1 || vCacheProposalsToUpdate.empty() || vCachePaymentRequestToUpdate.empty()) {
        vCacheProposalsToUpdate.clear();
        vCachePaymentRequestToUpdate.clear();
    } else {
        nBlocks = 1;
    }

    int64_t nTimeStart2 = GetTimeMicros();

    while(nBlocks > 0 && pindexblock != NULL)
    {
        CProposal proposal;
        CPaymentRequest prequest;

        vSeen.clear();

        for(unsigned int i = 0; i < pindexblock->vProposalVotes.size(); i++)
        {
            if(!view.GetProposal(pindexblock->vProposalVotes[i].first, proposal))
                continue;

            if(vSeen.count(pindexblock->vProposalVotes[i].first) == 0)
            {
                if(vCacheProposalsToUpdate.count(pindexblock->vProposalVotes[i].first) == 0)
                    vCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vProposalVotes[i].second == 1)
                    vCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.first += 1;
                else if(pindexblock->vProposalVotes[i].second == -1)
                    vCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].second += 1;
                else if(pindexblock->vProposalVotes[i].second == 0)
                    vCacheProposalsToUpdate[pindexblock->vProposalVotes[i].first].first.second += 1;

                vSeen[pindexblock->vProposalVotes[i].first]=true;
            }
        }

        for(unsigned int i = 0; i < pindexblock->vPaymentRequestVotes.size(); i++)
        {
            if(!view.GetPaymentRequest(pindexblock->vPaymentRequestVotes[i].first, prequest))
                continue;

            if(!view.GetProposal(prequest.proposalhash, proposal))
                continue;

            if (mapBlockIndex.count(proposal.blockhash) == 0)
                continue;

            CBlockIndex* pindexblockparent = mapBlockIndex[proposal.blockhash];

            if(pindexblockparent == NULL)
                continue;

            if(vSeen.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
            {
                if(vCachePaymentRequestToUpdate.count(pindexblock->vPaymentRequestVotes[i].first) == 0)
                    vCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first] = make_pair(make_pair(0, 0), 0);

                if(pindexblock->vPaymentRequestVotes[i].second == 1)
                    vCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.first += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == -1)
                    vCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].second += 1;
                else if(pindexblock->vPaymentRequestVotes[i].second == 0)
                    vCachePaymentRequestToUpdate[pindexblock->vPaymentRequestVotes[i].first].first.second += 1;

                vSeen[pindexblock->vPaymentRequestVotes[i].first]=true;
            }
        }
        pindexblock = pindexblock->pprev;
        nBlocks--;
    }

    vSeen.clear();

    int64_t nTimeEnd2 = GetTimeMicros();
    LogPrint("bench", "   - CFund count votes from headers: %.2fms\n", (nTimeEnd2 - nTimeStart2) * 0.001);

    int64_t nTimeStart3 = GetTimeMicros();
    std::map<uint256, std::pair<std::pair<int, int>, int>>::iterator it;
    std::vector<std::pair<uint256, CProposal>> vecProposalsToUpdate;
    std::vector<std::pair<uint256, CPaymentRequest>> vecPaymentRequestsToUpdate;

    for(it = vCacheProposalsToUpdate.begin(); it != vCacheProposalsToUpdate.end(); it++)
    {
        if (view.HaveProposal(it->first))
        {
            CProposalModifier proposal = view.ModifyProposal(it->first);
            proposal->nVotesYes = it->second.first.first;
            proposal->nVotesAbs = it->second.second;
            proposal->nVotesNo = it->second.first.second;
            vSeen[proposal->hash]=true;
        }
    }

    for(it = vCachePaymentRequestToUpdate.begin(); it != vCachePaymentRequestToUpdate.end(); it++)
    {
        if(view.HavePaymentRequest(it->first))
        {
            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it->first);
            prequest->nVotesYes = it->second.first.first;
            prequest->nVotesAbs = it->second.second;
            prequest->nVotesNo = it->second.first.second;
            vSeen[prequest->hash]=true;
        }
    }

    int64_t nTimeEnd3 = GetTimeMicros();
    LogPrint("bench", "   - CFund update votes: %.2fms\n", (nTimeEnd3 - nTimeStart3) * 0.001);

    std::vector<CPaymentRequest> vecPaymentRequest;

    int64_t nTimeStart4 = GetTimeMicros();
    CPaymentRequestMap mapPaymentRequests;

    if(view.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it = mapPaymentRequests.begin(); it != mapPaymentRequests.end(); it++)
        {
            if (!view.HavePaymentRequest(it->first))
                continue;

            bool fUpdate = false;

            CPaymentRequestModifier prequest = view.ModifyPaymentRequest(it->first);

            if(fUndo && prequest->paymenthash == pindexDelete->GetBlockHash())
            {
                prequest->paymenthash = uint256();
                fUpdate = true;
            }

            if(fUndo && prequest->blockhash == pindexDelete->GetBlockHash())
            {
                prequest->blockhash = uint256();
                prequest->fState = NIL;
                fUpdate = true;
            }

            if (prequest->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(prequest->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of payment request %s\n",
                          __func__, prequest->txblockhash.ToString(), prequest->hash.ToString());
                continue;
            }

            CBlockIndex* pblockindex = mapBlockIndex[prequest->txblockhash];

            CProposal proposal;

            if (!view.GetProposal(prequest->proposalhash, proposal))
                continue;

            auto nCreatedOnCycle = (unsigned )(pblockindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nCurrentCycle = (unsigned )(pindexNew->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min(nElapsedCycles, Params().GetConsensus().nCyclesPaymentRequestVoting + 1);

            auto oldState = prequest->fState;
            auto oldCycle = prequest->nVotingCycle;

            if((prequest->fState == NIL || fUndo) && nVotingCycles != prequest->nVotingCycle)
            {
                prequest->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if((!prequest->IsExpired() && prequest->fState == EXPIRED) ||
                        (!prequest->IsRejected() && prequest->fState == REJECTED))
                {
                    prequest->fState = NIL;
                    prequest->blockhash = uint256();
                    fUpdate = true;
                }

                if(!prequest->IsAccepted() && prequest->fState == ACCEPTED)
                {
                    prequest->fState = NIL;
                    prequest->blockhash = uint256();
                    fUpdate = true;
                }

                if(prequest->IsExpired())
                {
                    if (prequest->fState != EXPIRED)
                    {
                        prequest->fState = EXPIRED;
                        prequest->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
                else if(prequest->IsRejected())
                {
                    if (prequest->fState != REJECTED)
                    {
                        prequest->fState = REJECTED;
                        prequest->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                }
                else if(prequest->fState == NIL)
                {
                    if((proposal.fState == ACCEPTED || proposal.fState == PENDING_VOTING_PREQ) && prequest->IsAccepted())
                    {
                        if(prequest->nAmount <= pindexNew->nCFLocked && prequest->nAmount <= proposal.GetAvailable(view))
                        {
                            pindexNew->nCFLocked -= prequest->nAmount;
                            prequest->fState = ACCEPTED;
                            prequest->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                    }
                }
            }

            if (fUndo && fUpdate && prequest->fState == oldState && prequest->fState != NIL
                    && prequest->nVotingCycle != oldCycle)
                prequest->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if (!vSeen.count(prequest->hash) && prequest->fState == NIL &&
                    !((proposal.fState == ACCEPTED || proposal.fState == PENDING_VOTING_PREQ) && prequest->IsAccepted())){
                    prequest->nVotesYes = 0;
                    prequest->nVotesNo = 0;
                }
            }
        }
    }

    int64_t nTimeEnd4 = GetTimeMicros();
    LogPrint("bench", "   - CFund update payment request status: %.2fms\n", (nTimeEnd4 - nTimeStart4) * 0.001);

    std::vector<CProposal> vecProposal;

    int64_t nTimeStart5 = GetTimeMicros();
    CProposalMap mapProposals;

    if(view.GetAllProposals(mapProposals))
    {
        for (CProposalMap::iterator it = mapProposals.begin(); it != mapProposals.end(); it++)
        {
            if (!view.HaveProposal(it->first))
                continue;

            bool fUpdate = false;

            CProposalModifier proposal = view.ModifyProposal(it->first);

            if (proposal->txblockhash == uint256())
                continue;

            if (mapBlockIndex.count(proposal->txblockhash) == 0)
            {
                LogPrintf("%s: Can't find block %s of proposal %s\n",
                          __func__, proposal->txblockhash.ToString(), proposal->hash.ToString());
                continue;
            }

            if(fUndo && proposal->blockhash == pindexDelete->GetBlockHash())
            {
                proposal->blockhash = uint256();
                proposal->fState = NIL;
                fUpdate = true;
            }

            CBlockIndex* pblockindex = mapBlockIndex[proposal->txblockhash];

            auto nCreatedOnCycle = (unsigned int)(pblockindex->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nCurrentCycle = (unsigned int)(pindexNew->nHeight / Params().GetConsensus().nBlocksPerVotingCycle);
            auto nElapsedCycles = nCurrentCycle - nCreatedOnCycle;
            auto nVotingCycles = std::min(nElapsedCycles, Params().GetConsensus().nCyclesProposalVoting + 1);

            auto oldState = proposal->fState;
            auto oldCycle = proposal->nVotingCycle;

            if((proposal->fState == NIL || fUndo) && nVotingCycles != proposal->nVotingCycle)
            {
                proposal->nVotingCycle = nVotingCycles;
                fUpdate = true;
            }

            if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if((!proposal->IsExpired(pindexNew->GetBlockTime()) && (proposal->fState == EXPIRED || proposal->fState == PENDING_VOTING_PREQ)) ||
                   (!proposal->IsRejected() && proposal->fState == REJECTED))
                {
                    proposal->fState = NIL;
                    proposal->blockhash = uint256();
                    fUpdate = true;
                }

                if(!proposal->IsAccepted() && (proposal->fState == ACCEPTED || proposal->fState == PENDING_FUNDS))
                {
                    proposal->fState = NIL;
                    proposal->blockhash = uint256();
                    fUpdate = true;
                }

                if(proposal->IsExpired(pindexNew->GetBlockTime()))
                {
                    if (proposal->fState != EXPIRED)
                    {
                        if (proposal->HasPendingPaymentRequests(view))
                        {
                            proposal->fState = PENDING_VOTING_PREQ;
                            fUpdate = true;
                        }
                        else
                        {
                            if(proposal->fState == ACCEPTED || proposal->fState == PENDING_VOTING_PREQ)
                            {
                                pindexNew->nCFSupply += proposal->GetAvailable(view);
                                pindexNew->nCFLocked -= proposal->GetAvailable(view);
                            }
                            proposal->fState = EXPIRED;
                            proposal->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                    }
                }
                else if(proposal->IsRejected())
                {
                    if(proposal->fState != REJECTED)
                    {
                        proposal->fState = REJECTED;
                        proposal->blockhash = pindexNew->GetBlockHash();
                        fUpdate = true;
                    }
                } else if(proposal->IsAccepted())
                {
                    if((proposal->fState == NIL || proposal->fState == PENDING_FUNDS))
                    {
                        if(pindexNew->nCFSupply >= proposal->GetAvailable(view))
                        {
                            pindexNew->nCFSupply -= proposal->GetAvailable(view);
                            pindexNew->nCFLocked += proposal->GetAvailable(view);
                            proposal->fState = ACCEPTED;
                            proposal->blockhash = pindexNew->GetBlockHash();
                            fUpdate = true;
                        }
                        else if(proposal->fState != PENDING_FUNDS)
                        {
                            proposal->fState = PENDING_FUNDS;
                            proposal->blockhash = uint256();
                            fUpdate = true;
                        }
                    }
                }
            }

            if (fUndo && fUpdate && proposal->fState == oldState && proposal->fState != NIL && proposal->nVotingCycle != oldCycle)
                proposal->nVotingCycle = oldCycle;

            if((pindexNew->nHeight) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
            {
                if (!vSeen.count(proposal->hash) && proposal->fState == NIL)
                {
                    proposal->nVotesYes = 0;
                    proposal->nVotesAbs = 0;
                    proposal->nVotesNo = 0;
                }
            }
        }
    }

    int64_t nTimeEnd5 = GetTimeMicros();

    LogPrint("bench", "   - CFund update proposal status: %.2fms\n", (nTimeEnd5 - nTimeStart5) * 0.001);

    if((pindexNew->nHeight + 1) % Params().GetConsensus().nBlocksPerVotingCycle == 0)
    {
        CVoteMap mapVoters;
        view.GetAllVotes(mapVoters);

        for (auto& it: mapVoters)
        {
            view.RemoveCachedVoter(it.first);
        }
    }

    int64_t nTimeEnd = GetTimeMicros();
    LogPrint("bench", "  - CFund total VoteStep() function: %.2fms\n", (nTimeEnd - nTimeStart) * 0.001);
}


// CONSULTATIONS

std::string CConsultation::GetState() const {
    std::string sFlags = "waiting for support";
    if(IsSupported()) {
        sFlags = "found support";
        if(fState != ACCEPTED)
            sFlags += " waiting for end of voting period";
    }
    if(IsExpired())
        sFlags = "expired";
    return sFlags;
}

std::string CConsultation::ToString() const {
    std::string sRet = strprintf("CConsultation(hash=%s, nVersion=%d, strDZeel=\"%s\", fState=%s, vAnswers=[",
                                 hash.ToString(), nVersion, strDZeel, fState);

    for (auto &it: vAnswers)
    {
        sRet += it.ToString();
    }

    sRet += strprintf("], nVotingCycle=%u, nDeadline=%u, blockhash=%s)",
                     nVotingCycle, nDuration, blockhash.ToString().substr(0,10));
    return sRet;
}

void CConsultation::ToJson(UniValue& ret, CStateViewCache& view) const
{
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockhash", txblockhash.ToString()));
    ret.push_back(Pair("question", strDZeel));
    ret.push_back(Pair("support", nSupport));
    UniValue answers(UniValue::VARR);
    for (auto &it: vAnswers)
    {
        CConsultationAnswer answer;
        if (view.GetConsultationAnswer(it, answer))
        {
            UniValue ret;
            answer.ToJson(ret);
            answers.push_back(ret);
        }
    }
    ret.push_back(Pair("answers", answers));
    ret.push_back(Pair("min", nMin));
    ret.push_back(Pair("max", nMax));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesConsultationVoting)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.push_back(Pair("duration", nDuration));
    ret.push_back(Pair("status", GetState()));
    ret.push_back(Pair("state", (uint64_t)fState));
    ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
}

bool CConsultation::IsAccepted() const
{
    return false;
};

bool CConsultation::IsExpired() const
{
    return false;
};

bool CConsultation::ExceededMaxVotingCycles() const
{
    return nVotingCycle > Params().GetConsensus().nCyclesConsultationVoting;
};

void CConsultationAnswer::Vote() {
    nVotes++;
}

void CConsultationAnswer::DecVote() {
    nVotes = std::max(0,nVotes-1);
}

void CConsultationAnswer::ClearVotes() {
    nVotes=0;
}

int CConsultationAnswer::GetVotes() const {
    return nVotes;
}

void CConsultationAnswer::SetState(int fStateIn) {
    fState = fStateIn;
}

int CConsultationAnswer::GetState() const {
    return fState;
}

std::string CConsultationAnswer::GetText() const {
    return sAnswer;
}

bool CConsultationAnswer::CanBeVoted(CStateViewCache* view) const {
    CConsultation consultation;

    if (!view->HaveConsultation(parent))
        return false;

    view->GetConsultation(parent, consultation);

    if (consultation.fState != ACCEPTED)
        return false;

    return fState == ACCEPTED;
}

std::string CConsultationAnswer::ToString() const {
    return strprintf("CConsultationAnswer(fState=%u, sAnswer=\"%s\", nVotes=%u)", fState, sAnswer, nVotes);
}

void CConsultationAnswer::ToJson(UniValue& ret) const {
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("string", sAnswer));
    ret.push_back(Pair("votes", nVotes));
    ret.push_back(Pair("state", fState));
    ret.push_back(Pair("parent", parent.ToString()));
    if (hash != uint256())
        ret.push_back(Pair("hash", hash.ToString()));
}

// CFUND

bool IsValidPaymentRequest(CTransaction tx, CStateViewCache& coins, uint64_t nMaskVersion)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
            metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for payment request %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() && find_value(metadata, "s").isStr() && find_value(metadata, "h").isStr() && find_value(metadata, "i").isStr()))
        return error("%s: Wrong strdzeel for payment request %s", __func__, tx.GetHash().ToString());

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Signature = find_value(metadata, "s").get_str();
    std::string Hash = find_value(metadata, "h").get_str();
    std::string strDZeel = find_value(metadata, "i").get_str();
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int() : 1;

    if (nAmount < 0) {
         return error("%s: Payment Request cannot have amount less than 0: %s", __func__, tx.GetHash().ToString());
    }

    CProposal proposal;

    if(!coins.GetProposal(uint256S(Hash), proposal) || proposal.fState != ACCEPTED)
        return error("%s: Could not find parent proposal %s for payment request %s", __func__, Hash.c_str(),tx.GetHash().ToString());

    std::string sRandom = "";

    if (nVersion >= CPaymentRequest::BASE_VERSION && find_value(metadata, "r").isStr())
        sRandom = find_value(metadata, "r").get_str();

    std::string Secret = sRandom + "I kindly ask to withdraw " +
            std::to_string(nAmount) + "NAV from the proposal " +
            proposal.hash.ToString() + ". Payment request id: " + strDZeel;

    CNavCoinAddress addr(proposal.Address);
    if (!addr.IsValid())
        return error("%s: Address %s is not valid for payment request %s", __func__, proposal.Address, Hash.c_str(), tx.GetHash().ToString());

    CKeyID keyID;
    addr.GetKeyID(keyID);

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(Signature.c_str(), &fInvalid);

    if (fInvalid)
        return error("%s: Invalid signature for payment request  %s", __func__, tx.GetHash().ToString());

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << Secret;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig) || pubkey.GetID() != keyID)
        return error("%s: Invalid signature for payment request %s", __func__, tx.GetHash().ToString());

    if(nAmount > proposal.GetAvailable(coins, true))
        return error("%s: Invalid requested amount for payment request %s (%d vs %d available)",
                     __func__, tx.GetHash().ToString(), nAmount, proposal.GetAvailable(coins, true));

    bool ret = (nVersion & ~nMaskVersion) == 0;

    if(!ret)
        return error("%s: Invalid version for payment request %s", __func__, tx.GetHash().ToString());

    return nVersion <= Params().GetConsensus().nPaymentRequestMaxVersion;

}

bool CPaymentRequest::CanVote(CStateViewCache& coins) const
{
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    CProposal proposal;
    if(!coins.GetProposal(proposalhash, proposal))
        return false;

    return nAmount <= proposal.GetAvailable(coins) && fState != ACCEPTED && fState != REJECTED && fState != EXPIRED && !ExceededMaxVotingCycles();
}

bool CPaymentRequest::IsExpired() const {
    if(nVersion >= BASE_VERSION)
        return (ExceededMaxVotingCycles() && fState != ACCEPTED && fState != REJECTED);
    return false;
}

bool IsValidProposal(CTransaction tx, uint64_t nMaskVersion)
{
    if(tx.strDZeel.length() > 1024)
        return error("%s: Too long strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    UniValue metadata(UniValue::VOBJ);
    try {
        UniValue valRequest;
        if (!valRequest.read(tx.strDZeel))
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

        if (valRequest.isObject())
          metadata = valRequest.get_obj();
        else
            return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());

    } catch (const UniValue& objError) {
        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    } catch (const std::exception& e) {
        return error("%s: Wrong strdzeel for proposal %s: %s", __func__, tx.GetHash().ToString(), e.what());
    }

    if(!(find_value(metadata, "n").isNum() &&
            find_value(metadata, "a").isStr() &&
            find_value(metadata, "d").isNum() &&
            find_value(metadata, "s").isStr()))
    {

        return error("%s: Wrong strdzeel for proposal %s", __func__, tx.GetHash().ToString());
    }

    CAmount nAmount = find_value(metadata, "n").get_int64();
    std::string Address = find_value(metadata, "a").get_str();
    int64_t nDeadline = find_value(metadata, "d").get_int64();
    CAmount nContribution = 0;
    int nVersion = find_value(metadata, "v").isNum() ? find_value(metadata, "v").get_int() : 1;

    CNavCoinAddress address(Address);
    if (!address.IsValid())
        return error("%s: Wrong address %s for proposal %s", __func__, Address.c_str(), tx.GetHash().ToString());

    for(unsigned int i=0;i<tx.vout.size();i++)
        if(tx.vout[i].IsCommunityFundContribution())
            nContribution +=tx.vout[i].nValue;

    bool ret = (nContribution >= Params().GetConsensus().nProposalMinimalFee &&
            Address != "" &&
            nAmount < MAX_MONEY &&
            nAmount > 0 &&
            nDeadline > 0 &&
            (nVersion & ~nMaskVersion) == 0);

    if (!ret)
        return error("%s: Wrong strdzeel %s for proposal %s", __func__, tx.strDZeel.c_str(), tx.GetHash().ToString());

    return true;

}

bool CPaymentRequest::IsAccepted() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptPaymentRequest));
}

bool CPaymentRequest::IsRejected() const {
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectPaymentRequest));
}

bool CPaymentRequest::ExceededMaxVotingCycles() const {
    return nVotingCycle > Params().GetConsensus().nCyclesPaymentRequestVoting;
}

bool CProposal::IsAccepted() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesYes > ((float)(nTotalVotes) * Params().GetConsensus().nVotesAcceptProposal));
}

bool CProposal::IsRejected() const
{
    int nTotalVotes = nVotesYes + nVotesNo;
    float nMinimumQuorum = Params().GetConsensus().nMinimumQuorum;

    if (nVersion & REDUCED_QUORUM_VERSION)
        nMinimumQuorum = nVotingCycle > Params().GetConsensus().nCyclesProposalVoting / 2 ? Params().GetConsensus().nMinimumQuorumSecondHalf : Params().GetConsensus().nMinimumQuorumFirstHalf;

    if (nVersion & ABSTAIN_VOTE_VERSION)
        nTotalVotes += nVotesAbs;

    return nTotalVotes > Params().GetConsensus().nBlocksPerVotingCycle * nMinimumQuorum
           && ((float)nVotesNo > ((float)(nTotalVotes) * Params().GetConsensus().nVotesRejectProposal));
}

bool CProposal::CanVote() const {
    AssertLockHeld(cs_main);

    CBlockIndex* pindex;
    if(txblockhash == uint256() || !mapBlockIndex.count(txblockhash))
        return false;

    pindex = mapBlockIndex[txblockhash];
    if(!chainActive.Contains(pindex))
        return false;

    return (fState == NIL) && (!ExceededMaxVotingCycles());
}

uint64_t CProposal::getTimeTillExpired(uint32_t currentTime) const
{
    if(nVersion & BASE_VERSION) {
        if (mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pblockindex = mapBlockIndex[blockhash];
            return currentTime - (pblockindex->GetBlockTime() + nDeadline);
        }
    }
    return 0;
}

bool CProposal::IsExpired(uint32_t currentTime) const {
    if(nVersion & BASE_VERSION) {
        if (fState == ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            return (pBlockIndex->GetBlockTime() + nDeadline < currentTime);
        }
        return (fState == EXPIRED) || (fState == PENDING_VOTING_PREQ) || (ExceededMaxVotingCycles() && fState == NIL);
    } else {
        return (nDeadline < currentTime);
    }
}

bool CProposal::ExceededMaxVotingCycles() const {
    return nVotingCycle > Params().GetConsensus().nCyclesProposalVoting;
}

CAmount CProposal::GetAvailable(CStateViewCache& coins, bool fIncludeRequests) const
{
    AssertLockHeld(cs_main);

    CAmount initial = nAmount;
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!coins.GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            if (!coins.HaveCoins(prequest.hash))
            {
                CBlockIndex* pindex;
                if(prequest.txblockhash == uint256() || !mapBlockIndex.count(prequest.txblockhash))
                    continue;
                pindex = mapBlockIndex[prequest.txblockhash];
                if(!chainActive.Contains(pindex))
                    continue;
            }
            if((fIncludeRequests && prequest.fState != REJECTED && prequest.fState != EXPIRED) || (!fIncludeRequests && prequest.fState == ACCEPTED))
                initial -= prequest.nAmount;
        }
    }
    return initial;
}

std::string CProposal::ToString(CStateViewCache& coins, uint32_t currentTime) const {
    std::string str;
    str += strprintf("CProposal(hash=%s, nVersion=%i, nAmount=%f, available=%f, nFee=%f, address=%s, nDeadline=%u, nVotesYes=%u, "
                     "nVotesAbs=%u, nVotesNo=%u, nVotingCycle=%u, fState=%s, strDZeel=%s, blockhash=%s)",
                     hash.ToString(), nVersion, (float)nAmount/COIN, (float)GetAvailable(coins)/COIN, (float)nFee/COIN, Address, nDeadline,
                     nVotesYes, nVotesAbs, nVotesNo, nVotingCycle, GetState(currentTime), strDZeel, blockhash.ToString().substr(0,10));
    CPaymentRequestMap mapPaymentRequests;

    if(coins.GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            str += "\n    " + prequest.ToString();
        }
    }
    return str + "\n";
}

bool CProposal::HasPendingPaymentRequests(CStateViewCache& coins) const {
    AssertLockHeld(cs_main);

    CPaymentRequestMap mapPaymentRequests;

    if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
    {
        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            if(prequest.CanVote(coins))
                return true;
        }
    }
    return false;
}

std::string CProposal::GetState(uint32_t currentTime) const {
    std::string sFlags = "pending";
    if(IsAccepted()) {
        sFlags = "accepted";
        if(fState == PENDING_FUNDS)
            sFlags += " waiting for enough coins in fund";
        else if(fState != ACCEPTED)
            sFlags += " waiting for end of voting period";
    }
    if(IsRejected()) {
        sFlags = "rejected";
        if(fState != REJECTED)
            sFlags += " waiting for end of voting period";
    }
    if(currentTime > 0 && IsExpired(currentTime)) {
        sFlags = "expired";
        if(fState != EXPIRED && !ExceededMaxVotingCycles())
            // This branch only occurs when a proposal expires due to exceeding its nDeadline during a voting cycle, not due to exceeding max voting cycles
            sFlags += " waiting for end of voting period";
    }
    if(fState == PENDING_VOTING_PREQ) {
        sFlags = "expired pending voting of payment requests";
    }
    return sFlags;
}

void CProposal::ToJson(UniValue& ret, CStateViewCache& coins) const {
    AssertLockHeld(cs_main);

    ret.push_back(Pair("version", (uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("notPaidYet", FormatMoney(GetAvailable(coins))));
    ret.push_back(Pair("userPaidFee", FormatMoney(nFee)));
    ret.push_back(Pair("paymentAddress", Address));
    if(nVersion & BASE_VERSION) {
        ret.push_back(Pair("proposalDuration", (uint64_t)nDeadline));
        if (fState == ACCEPTED && mapBlockIndex.count(blockhash) > 0) {
            CBlockIndex* pBlockIndex = mapBlockIndex[blockhash];
            ret.push_back(Pair("expiresOn", pBlockIndex->GetBlockTime() + (uint64_t)nDeadline));
        }
    } else {
        ret.push_back(Pair("expiresOn", (uint64_t)nDeadline));
    }
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesAbs", nVotesAbs));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesProposalVoting)));
    // votingCycle does not return higher than nCyclesProposalVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesProposalVoting
    ret.push_back(Pair("status", GetState(chainActive.Tip()->GetBlockTime())));
    ret.push_back(Pair("state", (uint64_t)fState));
    if(fState == ACCEPTED)
        ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
    CPaymentRequestMap mapPaymentRequests;

    if(pcoinsTip->GetAllPaymentRequests(mapPaymentRequests))
    {
        UniValue preq(UniValue::VOBJ);
        UniValue arr(UniValue::VARR);
        CPaymentRequest prequest;

        for (CPaymentRequestMap::iterator it_ = mapPaymentRequests.begin(); it_ != mapPaymentRequests.end(); it_++)
        {
            CPaymentRequest prequest;

            if (!pcoinsTip->GetPaymentRequest(it_->first, prequest))
                continue;

            if (prequest.proposalhash != hash)
                continue;

            prequest.ToJson(preq);
            arr.push_back(preq);
        }

        ret.push_back(Pair("paymentRequests", arr));
    }
}

void CPaymentRequest::ToJson(UniValue& ret) const {
    ret.push_back(Pair("version",(uint64_t)nVersion));
    ret.push_back(Pair("hash", hash.ToString()));
    ret.push_back(Pair("blockHash", txblockhash.ToString()));
    ret.push_back(Pair("description", strDZeel));
    ret.push_back(Pair("requestedAmount", FormatMoney(nAmount)));
    ret.push_back(Pair("votesYes", nVotesYes));
    ret.push_back(Pair("votesAbs", nVotesAbs));
    ret.push_back(Pair("votesNo", nVotesNo));
    ret.push_back(Pair("votingCycle", (uint64_t)std::min(nVotingCycle, Params().GetConsensus().nCyclesPaymentRequestVoting)));
    // votingCycle does not return higher than nCyclesPaymentRequestVoting to avoid reader confusion, since votes are not counted anyway when votingCycle > nCyclesPaymentRequestVoting
    ret.push_back(Pair("status", GetState()));
    ret.push_back(Pair("state", (uint64_t)fState));
    ret.push_back(Pair("stateChangedOnBlock", blockhash.ToString()));
    if(fState == ACCEPTED) {
        ret.push_back(Pair("paidOnBlock", paymenthash.ToString()));
    }
}
