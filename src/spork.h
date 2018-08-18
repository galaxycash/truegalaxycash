// Copyright (c) 2009-2012 The Anoncoin developers
// Copyright (c) 2017-2018 The TrueGalaxyCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef TRUEGALAXYCASH_SPORK_H
#define TRUEGALAXYCASH_SPORK_H

#include "script.h"
#include "arith_uint256.h"
#include "uint256.h"
#include "sync.h"
#include "net.h"
#include "key.h"

#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"

#include "protocol.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

// Don't ever reuse these IDs for other sporks
#define SPORK_1_MASTERNODE_PAYMENTS_ENFORCEMENT               10000
#define SPORK_2_FASTTX                                        10001
#define SPORK_3_FASTTX_BLOCK_FILTERING                        10002
#define SPORK_4_NOTUSED                                       10003
#define SPORK_5_MAX_VALUE                                     10004
#define SPORK_6_REPLAY_BLOCKS                                 10005
#define SPORK_7_MASTERNODE_SCANNING                           10006
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT                10007
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT                 10008
#define SPORK_10_MASTERNODE_PAY_UPDATED_NODES                 10009
#define SPORK_11_RESET_BUDGET                                 10010
#define SPORK_12_RECONSIDER_BLOCKS                            10011
#define SPORK_13_ENABLE_SUPERBLOCKS                           10012

#define SPORK_1_MASTERNODE_PAYMENTS_ENFORCEMENT_DEFAULT       0            // ON
#define SPORK_2_FASTTX_DEFAULT                                0            // ON
#define SPORK_3_FASTTX_BLOCK_FILTERING_DEFAULT                0            // ON
#define SPORK_4_RECONVERGE_DEFAULT                            0            // ON - BUT NOT USED
#define SPORK_5_MAX_VALUE_DEFAULT                             30000000     // 30,000,000 TGCH
#define SPORK_6_REPLAY_BLOCKS_DEFAULT                         0            // ON - BUT NOT USED
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT        0            // ON
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT         4070908800   // OFF
#define SPORK_10_MASTERNODE_PAY_UPDATED_NODES_DEFAULT         4070908800   // OFF
#define SPORK_11_RESET_BUDGET_DEFAULT                         0            // ON
#define SPORK_12_RECONSIDER_BLOCKS_DEFAULT                    0            // ON
#define SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT                   4070908800   // OFF


class CSporkMessage;
class CSporkManager;

using namespace std;
using namespace boost;

extern std::map<uint256, CSporkMessage> mapSporks;
extern std::map<int, CSporkMessage> mapSporksActive;
extern CSporkManager sporkManager;

void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
int64_t GetSporkValue(int nSporkID);
bool IsSporkActive(int nSporkID);
void ExecuteSpork(int nSporkID, int nValue);
//void ReprocessBlocks(int nBlocks);

//
// Spork Class
// Keeps track of all of the network spork settings
//

class CSporkMessage
{
public:
    std::vector<unsigned char> vchSig;
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    uint256 GetHash(){
        uint256 n = Hash(BEGIN(nSporkID), END(nTimeSigned));
        return n;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
	unsigned int nSerSize = 0;
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
	}
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;

    std::string strMasterPrivKey;
    std::string strTestPubKey;
    std::string strMainPubKey;

public:

    CSporkManager() {
        strMainPubKey = "04553d8a0134cc9658b90e39cb0c36f74ce8bc22576830c091738ae93170dfac72056c35c94d269aca4fae9add44b8385f56f2b3c65ced8b20bb74b5e3d6387bfc";
        strTestPubKey = "04553d8a0134cc9658b90e39cb0c36f74ce8bc22576830c091738ae93170dfac72056c35c94d269aca4fae9add44b8385f56f2b3c65ced8b20bb74b5e3d6387bfc";
    }

    std::string GetSporkNameByID(int id);
    int GetSporkIDByName(std::string strName);
    bool UpdateSpork(int nSporkID, int64_t nValue);
    bool SetPrivKey(std::string strPrivKey);
    bool CheckSignature(CSporkMessage& spork);
    bool Sign(CSporkMessage& spork);
    void Relay(CSporkMessage& msg);

};

#endif

