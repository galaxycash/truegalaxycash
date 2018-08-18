// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRUEGALAXYCASH_CHAIN_PARAMS_H
#define TRUEGALAXYCASH_CHAIN_PARAMS_H

#include "arith_uint256.h"
#include "util.h"
#include "script.h"

#include <vector>

using namespace std;

#define MESSAGE_START_SIZE 4
typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

class CAddress;
class CBlock;

struct CDNSSeedData {
    string name, host;
    CDNSSeedData(const string &strName, const string &strHost) : name(strName), host(strHost) {}
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * TrueGalaxyCash system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Network {
        MAIN,
        EASY,
        TEST,
        MAX_NETWORK_TYPES
    };

    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const uint256& HashGenesisBlock() const { return hashGenesisBlock; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    int GetDefaultPort() const { return nDefaultPort; }
    const uint256& ProofOfWorkLimit() const { return powLimit; }
    const uint256& ProofOfStakeLimit() const { return stakeLimit; }
    virtual const CBlock& GenesisBlock() const = 0;
    virtual bool RequireRPCPassword() const { return true; }
    const string& DataDir() const { return strDataDir; }
    virtual Network NetworkID() const = 0;
    virtual string NetworkIDString() const = 0;
    const vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char> &Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    virtual const vector<CAddress>& FixedSeeds() const = 0;
    int RPCPort() const { return nRPCPort; }
    bool POWNoRetargeting() const { return fPOWNoRetargeting; }
    bool IsProtocolV1(int32_t nHeight) const { return true; }
    int64_t PowTargetTimespan(int32_t nHeight) const { return nPowTargetTimespan; }
    int64_t PowTargetSpacing(int32_t nHeight) const { return nPowTargetSpacing; }
    int64_t DifficultyAdjustmentInterval(int32_t nHeight) const { return (nPowTargetTimespan / nPowTargetSpacing); }
    int64_t POSStart() const { return nPOSFirstBlock; }
    int32_t LastPowBlock() const { return nLastPowBlock; }
protected:
    CChainParams() {};

    uint256 hashGenesisBlock;
    MessageStartChars pchMessageStart;
    int nDefaultPort;
    int nRPCPort;
    int64_t nPOSFirstBlock;
    int64_t nPowTargetTimespan;
    int64_t nPowTargetSpacing;
    int32_t nLastPowBlock;
    bool fPOWNoRetargeting;
    uint256 stakeLimit;
    uint256 powLimit;
    string strDataDir;
    vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
};

/**
 * Return the currently selected parameters. This won't change after app startup
 * outside of the unit tests.
 */
const CChainParams &Params();

/** Sets the params returned by Params() to those for the given network. */
void SelectParams(CChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectParamsFromCommandLine();

inline bool TestNet() {
    // Note: it's deliberate that this returns "false" for regression test mode.
    return Params().NetworkID() == CChainParams::TEST;
}

inline bool EasyNet() {
    // Note: it's deliberate that this returns "false" for regression test mode.
    return Params().NetworkID() == CChainParams::EASY;
}

#endif

