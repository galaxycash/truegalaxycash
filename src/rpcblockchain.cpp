// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "main.h"
#include "kernel.h"
#include "checkpoints.h"
#include "txdb.h"

using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

double GetDifficultyFromBits(unsigned int nBits)
{
    int nShift = (nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

double GetDifficulty(const CBlockIndex* blockindex)
{
    const CBlockIndex *pindex = blockindex;
    while (pindex && pindex->IsProofOfStake())
        pindex = pindex->pprev;

    if (!pindex)
        return UintToArith256(Params().ProofOfWorkLimit()).GetCompact();
    else
        return GetDifficultyFromBits(pindex->nBits);
}

double GetDifficultyForPOS()
{
    const CBlockIndex *pindex = pindexBest;
    while (pindex && pindex->IsProofOfWork())
        pindex = pindex->pprev;

    if (!pindex)
        return UintToArith256(Params().ProofOfStakeLimit()).GetCompact();
    else
        return GetDifficultyFromBits(pindex->nBits);
}


double GetPoWMHashPS()
{
    int nPoWInterval = Params().DifficultyAdjustmentInterval(nBestHeight);
    int64_t nTargetSpacingWorkMin = Params().PowTargetSpacing(nBestHeight), nTargetSpacingWork = 30;

    CBlockIndex* pindex = pindexGenesisBlock;
    CBlockIndex* pindexPrevWork = pindexGenesisBlock;

    while (pindex)
    {
        if (pindex->IsProofOfWork())
        {
            int64_t nActualSpacingWork = pindex->GetBlockTime() - pindexPrevWork->GetBlockTime();
            nTargetSpacingWork = ((nPoWInterval - 1) * nTargetSpacingWork + nActualSpacingWork + nActualSpacingWork) / (nPoWInterval + 1);
            nTargetSpacingWork = max(nTargetSpacingWork, nTargetSpacingWorkMin);
            pindexPrevWork = pindex;
        }

        pindex = pindex->pnext;
    }

    return GetDifficultyFromBits(GetLastBlockIndex(pindexBest, false)->nBits) * 4294.967296 / nTargetSpacingWork;
}

double GetPoSKernelPS()
{
    int nPoSInterval = Params().DifficultyAdjustmentInterval(nBestHeight);
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindex = pindexBest;
    CBlockIndex* pindexPrevStake = NULL;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            if (pindexPrevStake)
            {
                dStakeKernelsTriedAvg += GetDifficultyFromBits(pindexPrevStake->nBits) * 4294967296.0;
                nStakesTime += pindexPrevStake->nTime - pindex->nTime;
                nStakesHandled++;
            }
            pindexPrevStake = pindex;
        }

        pindex = pindex->pprev;
    }

    double result = 0;

    if (nStakesTime)
        result = dStakeKernelsTriedAvg / nStakesTime * STAKE_TIMESTAMP_MASK + 1;

    return result;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool fPrintTransactionDetail)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", (blockindex->GetBlockHash() != block.GetHash()) ? (blockindex->nHeight + 1) : blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", strprintf("%08x", block.nBits)));
    result.push_back(Pair("difficulty", GetDifficultyFromBits(block.nBits)));
    result.push_back(Pair("blocktrust", leftTrim(blockindex->GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(blockindex->nChainTrust.GetHex(), '0')));
    result.push_back(Pair("previousblockhash", block.hashPrevBlock.GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", block.IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->hashProof.GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016x", blockindex->nStakeModifier)));
    result.push_back(Pair("modifierv2", blockindex->bnStakeModifierV2.GetHex()));
    Array txinfo;
    BOOST_FOREACH (const CTransaction& tx, block.vtx)
    {
        if (fPrintTransactionDetail)
        {
            Object entry;

            entry.push_back(Pair("txid", tx.GetHash().GetHex()));
            TxToJSON(tx, 0, entry);

            txinfo.push_back(entry);
        }
        else
            txinfo.push_back(tx.GetHash().GetHex());
    }

    result.push_back(Pair("tx", txinfo));

    if (block.IsProofOfStake())
        result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "Returns the hash of the best block in the longest block chain.");

    return hashBestChain.GetHex();
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}


Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the difficulty as a multiple of the minimum difficulty.");

    Object obj;
    obj.push_back(Pair("proof-of-work",  GetDifficulty(pindexBest)));
    obj.push_back(Pair("proof-of-stake", GetDifficultyForPOS()));
    return obj;
}


Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->GetBlockHash().GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    //return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
    bool fTxinfo = false;
    if (params.size() > 2)
      fTxinfo = params[2].get_bool();

    // bitcoin-cli verbose=0 support
    bool fVerbose = true;
    if (params.size() > 1)
      fVerbose = params[1].get_bool();

    if (!fVerbose)
      {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
        return strHex;
      }

    return blockToJSON(block, pblockindex, fTxinfo);
}

Value setbestblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setbestblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    CTxDB txdb;
    if (!txdb.TxnBegin())
        return "Failed to tx begin!";

    if (!block.SetBestChain(txdb, pblockindex))
    {
        txdb.TxnAbort();
        return "Failed to set bestchain!";
    }

    txdb.TxnCommit();

    return Value::null;
}

Value getblockbynumber(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblockbynumber <number> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-number.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || (nHeight > nBestHeight))
        throw runtime_error("Block number out of range.");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

// ppcoin: get information of sync-checkpoint
Value getcheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getcheckpoint\n"
            "Show info of synchronized checkpoint.\n");

    Object result;
    const CBlockIndex* pindexCheckpoint = Checkpoints::AutoSelectSyncCheckpoint();

    result.push_back(Pair("synccheckpoint", pindexCheckpoint->GetBlockHash().ToString().c_str()));
    result.push_back(Pair("height", pindexCheckpoint->nHeight));
    result.push_back(Pair("timestamp", DateTimeStrFormat(pindexCheckpoint->GetBlockTime()).c_str()));

    result.push_back(Pair("policy", "rolling"));

    return result;
}


Value getsupply(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getsupply\n"
            "Show current coin supply.\n");

    if (pindexBest)
        return ValueFromAmount(pindexBest->nMoneySupply);

    return 0;
}

Value getmaxmoney(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getmaxmoney\n"
            "Show max coin supply.\n");

    return ValueFromAmount(MAX_MONEY);
}

class CCoinInfo {
public:
    int32_t nHeight;
    bool fCoinbase;
    CTxOut out;
    uint256 block;

    CCoinInfo() :
        nHeight(0),
        fCoinbase(false)
    {}

    CCoinInfo(const CCoinInfo &info) :
        nHeight(info.nHeight),
        fCoinbase(info.fCoinbase),
        out(info.out),
        block(info.block)
    {}
};

static const uint32_t MEMPOOL_HEIGHT = 0x7FFFFFFF;


bool GetCoin(const COutPoint &coin, CCoinInfo &info) {
    CTransaction tx;
    uint256 block;

    if (GetTransaction(coin.hash, tx, block)) {
        if (coin.n >= tx.vout.size())
            return false;

        CBlockIndex *pblock = mapBlockIndex[block];
        if (!pblock)
            return false;

        info.block = block;
        info.nHeight = pblock->nHeight;
        info.fCoinbase = tx.IsCoinBase();
        info.out = tx.vout[coin.n];

        return true;
    }

    return false;
}

bool GetCoinMempool(const COutPoint &coin, CCoinInfo &info) {
    CTransaction tx;
    if (mempool.lookup(coin.hash, tx)) {
        if (coin.n < tx.vout.size()) {
            info.block = uint256(0);
            info.fCoinbase = false;
            info.nHeight = MEMPOOL_HEIGHT;
            info.out = tx.vout[coin.n];
            return true;
        } else {
            return false;
        }
    }
    return GetCoin(coin, info);
}

extern void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out, bool fIncludeHex);
struct CCoinsStats
{
    int nHeight;
    uint256 hashBlock;
    uint64_t nTransactions;
    uint64_t nTransactionOutputs;
    uint64_t nBogoSize;
    uint256 hashSerialized;
    uint64_t nDiskSize;
    uint64_t nTotalAmount;

    CCoinsStats() : nHeight(0), nTransactions(0), nTransactionOutputs(0), nBogoSize(0), nDiskSize(0), nTotalAmount(0) {}
};

static void ApplyStats(CCoinsStats &stats, CHashWriter& ss, const uint256& hash, const std::map<uint32_t, CCoinInfo>& outputs)
{
    assert(!outputs.empty());
    ss << hash;
    ss << VARINT(outputs.begin()->second.nHeight * 2 + outputs.begin()->second.fCoinbase ? 1u : 0u);
    stats.nTransactions++;
    for (const auto output : outputs) {
        ss << VARINT(output.first + 1);
        ss << output.second.out.scriptPubKey;
        ss << VARINT(output.second.out.nValue);
        stats.nTransactionOutputs++;
        stats.nTotalAmount += output.second.out.nValue;
        stats.nBogoSize += 32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ + 8 /* amount */ +
                           2 /* scriptPubKey len */ + output.second.out.scriptPubKey.size() /* scriptPubKey */;
    }
    ss << VARINT(0u);
}

//! Calculate statistics about the unspent transaction output set
static bool GetUTXOStats(CCoinsStats &stats)
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = pindexBest->GetBlockHash();
    stats.nHeight = pindexBest->nHeight;

    ss << stats.hashBlock;

    uint256 prevkey;
    std::map<uint32_t, CCoinInfo> outputs;

    CBlock block;
    if (!block.ReadFromDisk(pindexBest, true))
        return false;

    stats.nDiskSize = 0;
    for (size_t i = 0; i < block.vtx.size(); i++) {
        CTransaction &tx = block.vtx[i];

        for (size_t j = 0; j < tx.vout.size(); j++) {
            COutPoint key(tx.GetHash(), j);

            CCoinInfo coin;
            if (GetCoin(key, coin)) {
                if (!outputs.empty() && key.hash != prevkey) {
                    ApplyStats(stats, ss, prevkey, outputs);
                    outputs.clear();
                }
                prevkey = key.hash;
                outputs[key.n] = std::move(coin);


            } else {
                return error("%s: unable to read value", __func__);
            }
        }

        stats.nDiskSize += tx.GetSerializeSize(SER_DISK, CLIENT_VERSION);
    }
    if (!outputs.empty()) {
        ApplyStats(stats, ss, prevkey, outputs);
    }
    stats.hashSerialized = ss.GetHash();

    return true;
}

Value gettxoutsetinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "gettxoutsetinfo\n"
            "\nReturns statistics about the unspent transaction output set.\n"
            "Note this call may take some time.\n"
            "\nResult:\n"
            "{\n"
            "  \"height\":n,     (numeric) The current block height (index)\n"
            "  \"bestblock\": \"hex\",   (string) the best block hash hex\n"
            "  \"transactions\": n,      (numeric) The number of transactions\n"
            "  \"txouts\": n,            (numeric) The number of output transactions\n"
            "  \"bogosize\": n,          (numeric) A meaningless metric for UTXO set size\n"
            "  \"hash_serialized_2\": \"hash\", (string) The serialized hash\n"
            "  \"disk_size\": n,         (numeric) The estimated size of the chainstate on disk\n"
            "  \"total_amount\": x.xxx          (numeric) The total amount\n"
            "}\n"
        );

    Object obj;

    CCoinsStats stats;
    if (GetUTXOStats(stats)) {
        obj.push_back(Pair("height", (int64_t)stats.nHeight));
        obj.push_back(Pair("bestblock", stats.hashBlock.GetHex()));
        obj.push_back(Pair("transactions", (int64_t)stats.nTransactions));
        obj.push_back(Pair("txouts", (int64_t)stats.nTransactionOutputs));
        obj.push_back(Pair("bogosize", (int64_t)stats.nBogoSize));
        obj.push_back(Pair("hash_serialized_2", stats.hashSerialized.GetHex()));
        obj.push_back(Pair("disk_size", stats.nDiskSize));
        obj.push_back(Pair("total_amount", ValueFromAmount(stats.nTotalAmount)));
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
    }
    return obj;
}

Value gettxout(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw std::runtime_error(
            "gettxout \"txid\" n ( include_mempool )\n"
            "\nReturns details about an unspent transaction output.\n"
            "\nArguments:\n"
            "1. \"txid\"             (string, required) The transaction id\n"
            "2. \"n\"                (numeric, required) vout number\n"
            "3. \"include_mempool\"  (boolean, optional) Whether to include the mempool. Default: true."
            "     Note that an unspent output that is spent in the mempool won't appear.\n"
            "\nResult:\n"
            "{\n"
            "  \"bestblock\" : \"hash\",    (string) the block hash\n"
            "  \"confirmations\" : n,       (numeric) The number of confirmations\n"
            "  \"value\" : x.xxx,           (numeric) The transaction value\n"
            "  \"scriptPubKey\" : {         (json object)\n"
            "     \"asm\" : \"code\",       (string) \n"
            "     \"hex\" : \"hex\",        (string) \n"
            "     \"reqSigs\" : n,          (numeric) Number of required signatures\n"
            "     \"type\" : \"pubkeyhash\", (string) The type, eg pubkeyhash\n"
            "     \"addresses\" : [          (array of string) array of bitcoin addresses\n"
            "        \"address\"     (string) bitcoin address\n"
            "        ,...\n"
            "     ]\n"
            "  },\n"
            "  \"coinbase\" : true|false   (boolean) Coinbase or not\n"
            "}\n"
        );

    Object obj;

    std::string strHash = params[0].get_str();
    uint256 hash(uint256S(strHash));
    int n = params[1].get_int();
    COutPoint out(hash, n);
    bool fMempool = true;
    if (params.size() > 2 && !params[2].is_null())
        fMempool = params[2].get_bool();

    CCoinInfo coin;
    if (fMempool) {
        if (!GetCoinMempool(out, coin) || mempool.isSpent(out))
            return Value::null;
    } else {
        if (!GetCoin(out, coin))
            return Value::null;
    }

    obj.push_back(Pair("bestblock", coin.block.GetHex()));
    if (coin.nHeight == pindexBest->nHeight || coin.nHeight <= 0) {
        obj.push_back(Pair("confirmations", 0));
    } else {
        obj.push_back(Pair("confirmations", (int64_t)(pindexBest->nHeight - coin.nHeight + 1)));
    }
    obj.push_back(Pair("value", ValueFromAmount(coin.out.nValue)));
    Object o;
    ScriptPubKeyToJSON(coin.out.scriptPubKey, o, true);
    obj.push_back(Pair("scriptPubKey", o));
    obj.push_back(Pair("coinbase", (bool)coin.fCoinbase));

    return obj;
}

#include "coins.h"

Value getcoins(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1)
        throw std::runtime_error(
            "getcoins \"holder\"\n"
            "\nReturns spendable coins for holder address.\n"
        );


    Object obj;

    if (GetBoolArg("-coins", false))
        return obj;

    CTrueGalaxyCashAddress address;
    address.SetString(params[0].get_str());

    if (!address.IsValid())
        return obj;

    CKeyID key;
    if (!address.GetKeyID(key))
        return obj;

    std::vector <COutPoint> coins;
    pCoins->GetSpendableCoins(key, coins);


    for (int i = 0; i < coins.size(); i++)
        obj.push_back(Pair(coins[i].hash.ToString(), std::to_string(coins[i].n)));

    return obj;
}
