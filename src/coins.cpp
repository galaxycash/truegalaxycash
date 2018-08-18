// Copyright (c) 2017-2018 The TrueGalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "masternodeman.h"
#include "main.h"
#include "init.h"
#include "util.h"
#include "base58.h"
#include "coins.h"
#include "script.h"

CCoins *pCoins = nullptr;

class CKeyIDVisitor : public boost::static_visitor<bool>
{
private:
    CKeyID *key;
public:
    CKeyIDVisitor(CKeyID *keyIn) : key(keyIn) { }
    bool operator()(const CKeyID &id) const {
        *key = id;
        return true;
    }
    bool operator()(const CScriptID &id) const {
        return false;
    }
    bool operator()(const CNoDestination &no) const {
        return false;
    }
};


bool GetScriptHolder(const CScript &script, uint160 &address) {
    CTxDestination destination; CKeyID key;
    if (ExtractDestination(script, destination)) {
        if (boost::apply_visitor(CKeyIDVisitor(&key), destination)) {
            address = key;
            return true;
        }
    }
    return false;
}

bool GetCoinHolder(const COutPoint &coin, uint160 &address) {

    uint256 hash;
    CTransaction tx;
    if (GetTransaction(coin.hash, tx, hash)) {
        if (tx.vout.size() <= coin.n)
            return false;

        CTxOut &vout = tx.vout[coin.n];
        return GetScriptHolder(vout.scriptPubKey, address);
    }
    return false;
}

CCoins::CCoins(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "coins", nCacheSize, fMemory, fWipe) {}

bool CCoins::IsSpendable(const COutPoint &coin) {
    LOCK(cs);

    if (Exists(coin)) {

        bool status = false;
        if (Read(coin, status))
            return status;
    }
    return false;
}

bool CCoins::IsUnspendable(const COutPoint &coin) {
    LOCK(cs);

    if (Exists(coin)) {

        bool status = false;
        if (Read(coin, status))
            return !status;
    }
    return true;
}

void CCoins::GetSpendableCoins(const uint160 &address, std::vector <COutPoint> &coins) {
    LOCK(cs);

    if (Exists(address))
        Read(address, coins);
}

bool CCoins::CheckHash(const uint160 &address, const uint256 &hash) {
    uint256 dbhash;

    if (Read(make_pair(string("hsh"), address), dbhash))
        return (dbhash == hash);

    return false;
}

uint256 CCoins::GetHash(const uint160 &address) {
    uint256 dbhash;

    if (Read(make_pair(string("hsh"), address), dbhash))
        return dbhash;

    return uint256();
}

void CCoins::ConnectBlock(const CBlock *pblock) {
    LOCK(cs);

    for (int i = 0; i < pblock->vtx.size(); i++) {
        const CTransaction &tx = pblock->vtx[i];

        // Process inputs
        for (int j = 0; j < tx.vin.size(); j++) {
            const CTxIn &vin = tx.vin[j];
            if (vin.prevout.IsNull())
                continue;

            if (Exists(vin.prevout)) {
                Write(vin.prevout, false);

                uint160 address;
                if (Read(make_pair(string("hld"), vin.prevout), address)) {

                    std::vector <COutPoint> coins;
                    if (Read(address, coins)) {

                        std::vector <COutPoint>::iterator it = std::find(coins.begin(), coins.end(), vin.prevout);
                        if (it != coins.end()) {
                            coins.erase(it);
                            uint256 hash = SerializeHash(coins);
                            Write(address, coins);
                            Write(make_pair(string("hsh"), address), hash);
                        }
                    }
                }
            }
        }

        // Process outputs
        for (int j = 0; j < tx.vout.size(); j++) {
            const CTxOut &vout = tx.vout[j];
            if (vout.IsEmpty())
                continue;

            COutPoint coin(tx.GetHash(), j);
            Write(coin, true);

            uint160 address;
            if (GetScriptHolder(vout.scriptPubKey, address)) {
                Write(make_pair(string("hld"), coin), address);

                std::vector <COutPoint> coins;
                if (Read(address, coins)) {

                    std::vector <COutPoint>::iterator it = std::find(coins.begin(), coins.end(), coin);
                    if (it == coins.end()) {
                        coins.push_back(coin);
                        uint256 hash = SerializeHash(coins);
                        Write(address, coins);
                        Write(make_pair(string("hsh"), address), hash);
                    }
                }
                else {
                    coins.push_back(coin);
                    uint256 hash = SerializeHash(coins);
                    Write(address, coins);
                    Write(make_pair(string("hsh"), address), hash);
                }
            }

        }
    }
}

void CCoins::DisconnectBlock(const CBlock *pblock) {
    LOCK(cs);

    for (int i = 0; i < pblock->vtx.size(); i++) {
        const CTransaction &tx = pblock->vtx[i];

        // Process inputs
        for (int j = 0; j < tx.vin.size(); j++) {
            const CTxIn &vin = tx.vin[j];
            if (vin.prevout.IsNull())
                continue;

            if (Exists(vin.prevout)) {
                Write(vin.prevout, true);

                uint160 address;
                if (Read(make_pair(string("hld"), vin.prevout), address)) {

                    std::vector <COutPoint> coins;
                    if (Read(address, coins)) {

                        std::vector <COutPoint>::iterator it = std::find(coins.begin(), coins.end(), vin.prevout);
                        if (it == coins.end()) {
                            coins.push_back(vin.prevout);
                            uint256 hash = SerializeHash(coins);

                            Write(address, coins);
                            Write(make_pair(string("hsh"), address), hash);
                        }
                    }
                }
            }
        }

        // Process outputs
        for (int j = 0; j < tx.vout.size(); j++) {
            const CTxOut &vout = tx.vout[j];
            if (vout.IsEmpty())
                continue;

            COutPoint coin(tx.GetHash(), j);

            if (Exists(coin)) {
                Erase(coin);

                uint160 address;
                if (GetScriptHolder(vout.scriptPubKey, address)) {
                    Erase(make_pair(string("hld"), coin));

                    std::vector <COutPoint> coins;
                    if (Read(address, coins)) {

                        std::vector <COutPoint>::iterator it = std::find(coins.begin(), coins.end(), coin);
                        if (it != coins.end()) {
                            coins.erase(it);
                            uint256 hash = SerializeHash(coins);

                            Write(address, coins);
                            Write(make_pair(string("hsh"), address), hash);
                        }
                    }
                }
            }

        }
    }
}
