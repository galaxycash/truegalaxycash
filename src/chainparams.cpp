// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2017 The TrueGalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"
#include "base58.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

//
// Main network
//

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CMainParams : public CChainParams {
public:
    CMainParams() {

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 'G';
        pchMessageStart[1] = 'C';
        pchMessageStart[2] = 'H';
        pchMessageStart[3] = 'M';

        // POS
        stakeLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPOSFirstBlock = 0;


        // Last PoW block
        nLastPowBlock = 20000;

        // Ports
        nDefaultPort = 8604;
        nRPCPort = 5604;

        // POW params
        powLimit = uint256S("00000fffff000000000000000000000000000000000000000000000000000000");
        nPowTargetSpacing = 1 * 60;
        nPowTargetTimespan = 1 * 60;
        fPOWNoRetargeting = false;

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        const char* pszTimestamp = "The Era of energy-effecient minning and fast transactions.";

        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].SetEmpty();

        CTransaction txNew(1, 1534504461, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1534504461;
        genesis.nBits    = UintToArith256(powLimit).GetCompact();
        genesis.nNonce   = 36143;

        hashGenesisBlock = genesis.GetHash();

        /*int counter = 0;
        while (true) {

            if (genesis.GetPoWHash() <= powLimit)
                break;

            genesis.nNonce++;

            if (counter % 500)
                std::cout << "Current hash is " << genesis.GetPoWHash().ToString() << ", target is " << powLimit.ToString() << std::endl;

            counter++;
        }

        std::cout << "Nonce is " << genesis.nNonce << std::endl;
        std::cout << "Hash is " << genesis.GetHash().ToString() << std::endl;
        std::cout << "PoW Hash is " << genesis.GetPoWHash().ToString() << std::endl;
        std::cout << "Marktle is " << genesis.hashMerkleRoot.ToString() << std::endl;
        */

        assert(hashGenesisBlock == uint256S("0x576c1b6eb8af58d39dcb6b26ec3452ee67ea59b40caba04f7c6f393d1756eeed"));
        assert(genesis.hashMerkleRoot == uint256S("0xa96e9aedccdd0274da597eddfa26440b7495dcdfcbfe55d562adcb9ab7d9ef22"));

        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("node1.galaxycash.info","195.133.201.213:8604"));
        vSeeds.push_back(CDNSSeedData("node2.galaxycash.info","195.133.201.213:8605"));
        vSeeds.push_back(CDNSSeedData("node3.galaxycash.info","195.133.201.213:8606"));


        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,38);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,99);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,89);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x02)(0x2D)(0x25)(0x33).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x02)(0x21)(0x31)(0x2B).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds.clear();
        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }
    virtual string NetworkIDString() const { return "main"; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//

class CTestParams : public CMainParams {
public:
    CTestParams() {
        strDataDir = "test";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 'G';
        pchMessageStart[1] = 'C';
        pchMessageStart[2] = 'H';
        pchMessageStart[3] = 'T';

        // POW
        fPOWNoRetargeting = GetBoolArg("-pownoretargeting", true);

        // POS params
        nPOSFirstBlock = 0;

        // Ports
        nDefaultPort = 18604;
        nRPCPort = 15604;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,41);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,51);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,61);
    }
    virtual Network NetworkID() const { return CChainParams::TEST; }
    virtual string NetworkIDString() const { return "test"; }
};
static CTestParams testParams;

//
// Testnet
//

class CEasyParams : public CMainParams {
public:
    CEasyParams() {
        strDataDir = "easy";

        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 'G';
        pchMessageStart[1] = 'C';
        pchMessageStart[2] = 'H';
        pchMessageStart[3] = 'E';

        // POS params
        nPOSFirstBlock = 0;

        // POW params
        fPOWNoRetargeting = true;

        // Ports
        nDefaultPort = 19604;
        nRPCPort = 16604;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,40);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,50);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,60);
    }
    virtual Network NetworkID() const { return CChainParams::EASY; }
    virtual string NetworkIDString() const { return "easy"; }
};
static CEasyParams easyParams;


static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::EASY:
            pCurrentParams = &easyParams;
        break;
        case CChainParams::TEST:
            pCurrentParams = &testParams;
            break;

        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {

    bool fTestNet = GetBoolArg("-testnet", false);
    bool fEasyNet = GetBoolArg("-easynet", false);


    if (fTestNet)
        SelectParams(CChainParams::TEST);
    else if (fEasyNet)
        SelectParams(CChainParams::EASY);
    else
        SelectParams(CChainParams::MAIN);

    return true;
}

