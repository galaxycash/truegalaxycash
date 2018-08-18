// Copyright (c) 2017-2018 The TrueGalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRUEGALAXYCASH_COINS_PARAMS_H
#define TRUEGALAXYCASH_COINS_PARAMS_H

#include "base58.h"
#include "core.h"
#include "leveldbwrapper.h"

class CCoins : public CLevelDBWrapper {
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    CCoins(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    void GetSpendableCoins(const uint160 &address, std::vector <COutPoint> &coins);
    bool CheckHash(const uint160 &address, const uint256 &hash);
    uint256 GetHash(const uint160 &address);

    void ConnectBlock(const CBlock *pblock);
    void DisconnectBlock(const CBlock *pblock);



    bool IsSpendable(const COutPoint &id);
    bool IsUnspendable(const COutPoint &id);
};
extern CCoins *pCoins;

#endif
