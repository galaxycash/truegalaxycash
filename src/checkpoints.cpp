// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "txdb.h"
#include "main.h"
#include "uint256.h"


static const int nCheckpointSpan = 0;

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
         ( 0,       uint256S("0x576c1b6eb8af58d39dcb6b26ec3452ee67ea59b40caba04f7c6f393d1756eeed") )
         ( 1,       uint256S("0x4e8f7a8e796b613d85b2ec4537f14dcbf88a9dcb83ce216b6db6267907b36b8b") )
         ( 1000,    uint256S("0xe833e60c26611de62b8491770553b533b78560f995a35c8914bddb4b923aa14f") )
    ;
    static MapCheckpoints mapTestCheckpoints =
        boost::assign::map_list_of
         ( 0,    uint256S("0x576c1b6eb8af58d39dcb6b26ec3452ee67ea59b40caba04f7c6f393d1756eeed") )
    ;


    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

        if (checkpoints.empty())
            return 0;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = TestNet() ? mapTestCheckpoints : mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

    // Automatically select a suitable sync-checkpoint
    const CBlockIndex* AutoSelectSyncCheckpoint()
    {
        const CBlockIndex *pindex = pindexBest;
        // Search backward for a block within max span and maturity window
        while (pindex->pprev && pindex->nHeight + nCheckpointSpan > pindexBest->nHeight)
            pindex = pindex->pprev;
        return pindex;
    }

    // Check against synchronized checkpoint
    bool CheckSync(int nHeight)
    {
        const CBlockIndex* pindexSync = AutoSelectSyncCheckpoint();

        const int nSync = std::max(0, pindexSync->nHeight - (int)(GetArg("-maxreorganize", int64_t(1024))));
        if (nHeight < nSync)
            return false;
        return true;
    }
}

