// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ismine.h"

#include "chainparams.h"
#include "key.h"
#include "keystore.h"
#include "script/script.h"
#include "script/standard.h"
#include "script/sign.h"
#include "wallet/wallet.h"

#include <boost/foreach.hpp>

using namespace std;

typedef vector<unsigned char> valtype;

std::vector<uint256> vMyMints;

unsigned int HaveKeys(const vector<valtype>& pubkeys, const CKeyStore& keystore)
{
    unsigned int nResult = 0;
    BOOST_FOREACH(const valtype& pubkey, pubkeys)
    {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (keystore.HaveKey(keyID))
            ++nResult;
    }
    return nResult;
}

isminetype IsMine(const CKeyStore &keystore, const CTxDestination& dest)
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(keystore, script);
}

isminetype IsMine(const CKeyStore &keystore, const CScript& scriptPubKey)
{
    vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions)) {
        if (keystore.HaveWatchOnly(scriptPubKey))
            return ISMINE_WATCH_UNSOLVABLE;
        return ISMINE_NO;
    }

    CKeyID keyID;
    CKeyID keyID2;
    switch (whichType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_CONTRIBUTION:
    case TX_PROPOSALNOVOTE:
    case TX_PROPOSALYESVOTE:
    case TX_PAYMENTREQUESTNOVOTE:
    case TX_PAYMENTREQUESTYESVOTE:
        break;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (keystore.HaveKey(keyID))
            return ISMINE_SPENDABLE;
        break;
    case TX_PUBKEYHASH:
    case TX_WITNESS_V0_KEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (keystore.HaveKey(keyID))
            return ISMINE_SPENDABLE;
        break;
    case TX_COLDSTAKING: {
        keyID = CKeyID(uint160(vSolutions[1]));
        keyID2 = CKeyID(uint160(vSolutions[0]));
        bool fSpendable = keystore.HaveKey(keyID);
        bool fStakable = keystore.HaveKey(keyID2);
        if (fSpendable && fStakable)
            return ISMINE_SPENDABLE_STAKABLE;
        else if (fSpendable)
            return ISMINE_SPENDABLE;
        else if (fStakable)
            return ISMINE_STAKABLE;
        break;
    }
    case TX_ZEROCOIN:
    {
        CPubKey p(vSolutions[0]); CBigNum c(vSolutions[1]);  CBigNum a(vSolutions[2]); CBigNum ac(vSolutions[3]);
        CKey zk; libzeroct::BlindingCommitment bc;
        uint256 scriptHash = Hash(scriptPubKey.begin(), scriptPubKey.end());
        if(std::find(vMyMints.begin(), vMyMints.end(), scriptHash) != vMyMints.end())
            return ISMINE_SPENDABLE_PRIVATE;
        if(!keystore.GetZeroKey(zk))
            break;
        if(!keystore.GetBlindingCommitment(bc))
            break;
        if(libzeroct::PrivateCoin::QuickCheckIsMine(&Params().GetConsensus().ZeroCT_Params, zk, p, bc, c, a, ac)) {
            vMyMints.push_back(scriptHash);
            return ISMINE_SPENDABLE_PRIVATE;
        }
        break;
    }
    case TX_SCRIPTHASH:
    {
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            isminetype ret = IsMine(keystore, subscript);
            if (ret == ISMINE_SPENDABLE)
                return ret;
        }
        break;
    }
    case TX_WITNESS_V0_SCRIPTHASH:
    {
        uint160 hash;
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(hash.begin());
        CScriptID scriptID = CScriptID(hash);
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            isminetype ret = IsMine(keystore, subscript);
            if (ret == ISMINE_SPENDABLE)
                return ret;
        }
        break;
    }

    case TX_MULTISIG:
    {
        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (HaveKeys(keys, keystore) == keys.size())
            return ISMINE_SPENDABLE;
        break;
    }
    }

    if (keystore.HaveWatchOnly(scriptPubKey)) {
        // TODO: This could be optimized some by doing some work after the above solver
        SignatureData sigs;
        return ProduceSignature(DummySignatureCreator(&keystore), scriptPubKey, sigs) ? ISMINE_WATCH_SOLVABLE : ISMINE_WATCH_UNSOLVABLE;
    }
    return ISMINE_NO;
}
