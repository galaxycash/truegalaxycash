// Copyright (c) 2017-2018 The TrueGalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRUEGALAXYCASH_SMARTCHAIN_PARAMS_H
#define TRUEGALAXYCASH_SMARTCHAIN_PARAMS_H

#include "base58.h"
#include "core.h"
#include "leveldbwrapper.h"

class CSmartContract {
public:
    std::string name;
    std::string author;
    int64_t     supply;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(name);
        READWRITE(author);
        READWRITE(supply);
    )

    CSmartContract() {
        SetNull();
    }

    CSmartContract(const CSmartContract &contract) :
        name(contract.name),
        author(contract.author),
        supply(contract.supply),
    {}

    void SetNull()
    {
        name.clear();
        author.clear();
        supply = 0;
    }

    bool IsNull() const
    {
        return (name.empty() && author.empty() && supply == 0);
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }
};


class CSmartChain : public CLevelDBWrapper {
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    CSmartChain(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool        AddContract(const CSmartContract &contract);
    bool        RemoveContract(const uint256 &id);
    bool        RemoveContract(const CSmartContract &contract);
    bool        GetContract(const uint256 &id, CSmartContract &contract);
    bool        GetContracts(std::vector <uint256> &contracts);
    bool        ExistsContract(const uint256 &id);
};

void            ThreadSmartChain();

#endif
