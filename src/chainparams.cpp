// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2018 The DogeCash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Params.h"
#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"
#include "net.h"
#include "base58.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
    boost::assign::map_list_of
    (0, uint256("000009a7bad1966421754adaa60cfaaef30dd065b30e1a93b8c6d71e3cfe1be7")); 
static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
    1538406008, // * UNIX timestamp of last checkpoint block
    0,    // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
    2000        // * estimated number of transactions per day after checkpoint
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
    1740710,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0x001"));
static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
    1454124731,
    0,
    100};

libzerocoin::ZerocoinParams* CChainParams::Zerocoin_Params() const
{
    assert(this);
    static CBigNum bnTrustedModulus(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParams = libzerocoin::ZerocoinParams(bnTrustedModulus);

    return &ZCParams;
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        vTreasuryRewardAddress="DKv8dUifgBKkWM1nwjad7yNasQ41yA9ntR";
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0x6a;
        pchMessageStart[1] = 0x5c;
        pchMessageStart[2] = 0x1a;
        pchMessageStart[3] = 0x6f;
        vAlertPubKey = ParseHex("047a7df379bd5e6b93b164968c10fcbb141ecb3c6dc1a5e181c2a62328405cf82311dd5b40bf45430320a4f30add05c8e3e16dd56c52d65f7abe475189564bf2b1");
        nDefaultPort = 6740;
        bnProofOfWorkLimit = ~uint256(0) >> 20; // DogeCash starting difficulty is 1 / 2^12
        nSubsidyHalvingInterval = 210000;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // DogeCash: 1 day  
        nTargetSpacing = 1 * 60; //DogeCash: 1 Min
        nMaturity = 30;
        nMasternodeCountDrift = 20;
        nMasternodeCollateralLimit = 5000; //MN collateral
        nMaxMoneyOut = 21000000 * COIN; //21 mill
        nMinStakeReserve = 100;
        /** Height or Time Based Activations **/
        nLastPOWBlock = 1440; //24 Hours POW
        nModifierUpdateBlock = INT_MAX;
        nZerocoinStartHeight = INT_MAX;
        nAccumulatorStartHeight = 1;
        nZerocoinStartTime = INT_MAX; // Friday, June 1, 2018 12:00:00 AM - GMT
        nBlockEnforceSerialRange = 1; //Enforce serial range starting this block
        nBlockRecalculateAccumulators = ~1; //Trigger a recalculation of accumulators
        nBlockFirstFraudulent = ~1; //First block that bad serials emerged
        nBlockLastGoodCheckpoint = ~1; //Last valid accumulator checkpoint
        
        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         *
         * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
         *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
         *   vMerkleTree: e0028e
         */
    const char* pszTimestamp = "DogeCash MainNet Launch - 1st October"; 
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN; 
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("047a7df379bd5e6b93b164968c10fcbb141ecb3c6dc1a5e181c2a62328405cf82311dd5b40bf45430320a4f30add05c8e3e16dd56c52d65f7abe475189564bf2b1") << OP_CHECKSIG; 

        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime = 1538406008; 
        genesis.nBits = 0x1e0ffff0;
      genesis.nNonce = 1383794; 
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("000009a7bad1966421754adaa60cfaaef30dd065b30e1a93b8c6d71e3cfe1be7")); 
        assert(genesis.hashMerkleRoot == uint256("78238ed2a47655347272ac0feaaed0596ec057a1ad8958a9afd2ca1d4173b3e0")); 


        // hashGenesisBlock = genesis.GetHash();
        // assert(hashGenesisBlock == uint256("0000cc75a7c6fa2ce8186e24f872e43acf88b21a1cc02aa11a4ceaee2a562d4c"));
        // assert(genesis.hashMerkleRoot == uint256("3bf54807365f102ff9cdb07cf5f4af411503d5b544835dc96a5beaee140ad419"));

        vSeeds.push_back(CDNSSeedData("45.76.229.12", "45.76.229.12"));
	//added new nodes from command
	vSeeds.push_back(CDNSSeedData("gennodes","51.15.116.138:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.194.64:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","94.16.117.32:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","195.201.202.189:43380"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.132.216:45310"));
vSeeds.push_back(CDNSSeedData("gennodes","104.255.170.217:43218"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.229.127:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.37.61:42326"));
vSeeds.push_back(CDNSSeedData("gennodes","178.63.100.26:35752"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.96.62:42896"));
vSeeds.push_back(CDNSSeedData("gennodes","178.62.199.143:33362"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.130.26:60198"));
vSeeds.push_back(CDNSSeedData("gennodes","94.130.218.205:44328"));
vSeeds.push_back(CDNSSeedData("gennodes","94.177.255.199:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.55.198:57100"));
vSeeds.push_back(CDNSSeedData("gennodes","94.130.218.205:44052"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.2.200:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.81.34:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","198.13.47.173:35996"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.29.85:49576"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.61.77:41990"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.66.241:34656"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.66.241:41802"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.66.241:36794"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.66.241:56736"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.20.255:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.81.88:40576"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.37.13:35324"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.221.72:60976"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.176.148:39566"));
vSeeds.push_back(CDNSSeedData("gennodes","51.15.72.213:40168"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.113.51:57940"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.33.78:49302"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.114.171:56860"));
vSeeds.push_back(CDNSSeedData("gennodes","51.158.74.126:37404"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.11.208:56938"));
vSeeds.push_back(CDNSSeedData("gennodes","51.15.36.253:54556"));
vSeeds.push_back(CDNSSeedData("gennodes","[2601:602:a7f:de62:a00:27ff:fea6:461f]:41190"));
vSeeds.push_back(CDNSSeedData("gennodes","173.199.71.133:46792"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.28.143:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.1.127:42060"));
vSeeds.push_back(CDNSSeedData("gennodes","8.9.11.202:39236"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.181.162:47856"));
vSeeds.push_back(CDNSSeedData("gennodes","173.171.235.26:49807"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.174.208:49556"));
vSeeds.push_back(CDNSSeedData("gennodes","35.198.90.91:52302"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.86.21:60432"));
vSeeds.push_back(CDNSSeedData("gennodes","24.19.2.46:60987"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.87.132:57168"));
vSeeds.push_back(CDNSSeedData("gennodes","198.46.189.176:59455"));
vSeeds.push_back(CDNSSeedData("gennodes","107.191.51.146:34386"));
vSeeds.push_back(CDNSSeedData("gennodes","54.36.175.160:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.25.94:40732"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.140.254:43358"));
vSeeds.push_back(CDNSSeedData("gennodes","104.207.147.24:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.62.232.105:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.169.22:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.169.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.151.142:51920"));
vSeeds.push_back(CDNSSeedData("gennodes","165.227.99.17:42542"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.206.215:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.102.250:33380"));
vSeeds.push_back(CDNSSeedData("gennodes","80.240.20.4:54010"));
vSeeds.push_back(CDNSSeedData("gennodes","99.113.163.123:50037"));
vSeeds.push_back(CDNSSeedData("gennodes","91.42.180.135:49866"));
vSeeds.push_back(CDNSSeedData("gennodes","[2806:10b7:2:5840:fdd5:a5ee:acad:3067]:56345"));
vSeeds.push_back(CDNSSeedData("gennodes","104.55.221.86:50928"));
vSeeds.push_back(CDNSSeedData("gennodes","[2a02:c7d:bf1:9e00:80c7:13b9:8ee8:7f40]:54985"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.139.139:48056"));
vSeeds.push_back(CDNSSeedData("gennodes","209.250.254.115:38856"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.54.10:44294"));
vSeeds.push_back(CDNSSeedData("gennodes","64.182.71.76:42826"));
vSeeds.push_back(CDNSSeedData("gennodes","37.57.118.53:50468"));
vSeeds.push_back(CDNSSeedData("gennodes","13.228.176.60:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","[2603:9000:f291:8800:ac66:9b0b:d9ba:892d]:61787"));
vSeeds.push_back(CDNSSeedData("gennodes","67.41.7.161:62004"));
vSeeds.push_back(CDNSSeedData("gennodes","31.204.152.195:49472"));
vSeeds.push_back(CDNSSeedData("gennodes","37.112.235.247:61317"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.166.19:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","178.209.127.160:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","195.201.93.188:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.91:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.203.102.114:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","77.220.215.67:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","80.211.93.228:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.67.137:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","169.222.109.2:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.158.19:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.114.171:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","80.240.20.4:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.60.199:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.197.197:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.93.61:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.217.131:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.169.66:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.109.194:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.234.14:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","107.161.95.252:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.55.198:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.81.34:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","169.222.109.1:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.61.226:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.31.247:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.109.130:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.174.208:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","173.199.71.133:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","104.248.10.32:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.210.243:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","104.248.2.245:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.215.93:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.120.35:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","51.15.36.253:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.157.32:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.182.99:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","13.228.176.60:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.0.245:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.203.122:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.85:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","209.250.254.115:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.161.205:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.25.94:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.178.172:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.29.85:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.47.104:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","85.121.197.40:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.72.197:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","202.182.98.198:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.86.65:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.194.247:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.1.203:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.176.148:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.89.114.8:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.182.98:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.169.65:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.11.189:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.151.142:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","80.240.30.183:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.11.208:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","66.42.93.193:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","86.105.54.9:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","104.156.254.94:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","3.120.208.19:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","206.189.59.166:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.84:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.28.143:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.119.130:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","85.214.227.169:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.158.168:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.178.76:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","155.94.252.118:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.4.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","72.238.210.148:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","8.9.11.202:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","83.137.55.117:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.119.129:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.235.25.67:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","66.42.114.134:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.167.131:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.181.235:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.204.34:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","80.241.217.134:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","169.222.109.3:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.20.255:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.54.10:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.148.82:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.38.180:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.85.229:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.160.23:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.204.217:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.102.250:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","51.15.116.138:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.100:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.139.139:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","209.250.255.99:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.169.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","107.191.62.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.169.73:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.140.254:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","198.13.47.173:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.47.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.119.98:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","104.207.147.24:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.191.194:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.176.243:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","153.92.4.10:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","207.148.25.91:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","195.201.126.99:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.128.85:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.44.136:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","94.197.121.41:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.86:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.175.162:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.16.212:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.187.69:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.37.130:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.36.77:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.169.74:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.83:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.199.202:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","169.222.109.4:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","98.246.93.89:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.206.215:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.125.97:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","94.177.255.199:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.86.68:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.120.34:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.56.164:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.82.97:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","51.15.72.213:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.167.129:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.195.186:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.130.26:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.102.60:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.169.22:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.247.47:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","198.211.117.126:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","178.62.68.153:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.74.79:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.1.127:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.50.193:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","7pzjqolkdfyor35w.onion:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.69.17.172:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","209.250.243.88:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","165.227.99.17:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.34.211:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.2.200:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.113.51:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.81.33:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.63.90.105:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.61.225:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.164.148:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","195.201.132.70:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.61.16.25:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","89.40.121.232:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.177.251:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","207.246.118.139:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.239.203:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.167.132:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.171.129:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","51.158.74.126:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.90:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.132.216:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.38.71:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.142.55:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.194.64:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.103:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.169.75:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.125.225:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.105:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","165.227.54.114:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.22.87:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","217.69.4.223:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.181.162:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.9.120:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.145.106:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.21.24.22:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","66.42.113.138:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.40.237:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.125.98:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.204.86:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","142.93.72.39:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.14.1:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.125.99:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.182.97:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","52.220.231.146:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.229.127:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","142.93.135.201:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.86.66:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.86.21:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.241.122:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.106:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","159.203.119.183:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","142.93.173.251:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","199.247.26.40:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","209.250.245.222:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.101.236.183:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.109.129:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","188.40.147.33:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.109.193:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.141.115:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.120.36:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.123.164:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.87.132:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.86.67:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.150.222:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.102:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","149.28.96.94:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.98:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","140.82.57.23:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","8.3.29.68:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.160.135.137:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.216.120.33:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","209.97.176.48:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","94.16.117.32:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.76.96.62:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","144.202.11.96:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","178.128.215.146:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.4.167.130:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.109.114:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","176.9.176.97:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.32.238.83:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","45.77.162.22:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","95.179.145.100:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","46.235.25.65:6740"));
vSeeds.push_back(CDNSSeedData("gennodes","108.61.81.88:6740"));
                
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 30);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 122);
		base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x02)(0x2D)(0x25)(0x33).convert_to_container<std::vector<unsigned char> >();
		base58Prefixes[EXT_SECRET_KEY] = list_of(0x02)(0x21)(0x31)(0x2B).convert_to_container<std::vector<unsigned char> >();
        // 	BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0xbc).convert_to_container<std::vector<unsigned char> >();
        

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "047a7df379bd5e6b93b164968c10fcbb141ecb3c6dc1a5e181c2a62328405cf82311dd5b40bf45430320a4f30add05c8e3e16dd56c52d65f7abe475189564bf2b1";
        strObfuscationPoolDummyAddress = "DKv8dUifgBKkWM1nwjad7yNasQ41yA9ntR";
        nStartMasternodePayments = 1403728576; //Wed, 25 Jun 2014 20:36:16 GMT

        /** Zerocoin */
        zerocoinModulus = "c95577b6dce0049b0a20c779af38079355abadde1a1d80c353f6cb697a7ae5a087bad39caa5798478551d0f9d91e6267716506f32412de1d19d17588765eb9502b85c6a18abdb05791cfd8b734e960281193705eeece210920cc922b3af3ceb178bf12c22eb565d5767fbf19545639be8953c2c38ffad41f3371e4aac750ac2d7bd614b3faabb453081d5d88fdbb803657a980bc93707e4b14233a2358c97763bf28f7c933206071477e8b371f229bc9ce7d6ef0ed7163aa5dfe13bc15f7816348b328fa2c1e69d5c88f7b94cee7829d56d1842d77d7bb8692e9fc7b7db059836500de8d57eb43c345feb58671503b932829112941367996b03871300f25efb5";
        nMaxZerocoinSpendsPerTransaction = 7; // Assume about 20kb each
        nMinZerocoinMintFee = 1 * ZCENT; //high fee required for zerocoin mints
        nMintRequiredConfirmations = 20; //the maximum amount of confirmations until accumulated in 19
        nRequiredAccumulation = 1;
        nDefaultSecurityLevel = 100; //full security level for accumulators
        nZerocoinHeaderVersion = 4; //Block headers must be this version once zerocoin is active
        nBudget_Fee_Confirmations = 6; // Number of confirmations for the finalization fee
        
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
    
 
};

std::string CChainParams::GetTreasuryRewardAddressAtHeight(int nHeight) const {
    return vTreasuryRewardAddress;
}

CScript CChainParams::GetTreasuryRewardScriptAtHeight(int nHeight) const {
    CBitcoinAddress address(GetTreasuryRewardAddressAtHeight(nHeight));
    assert(address.IsValid());

    CScript script = GetScriptForDestination(address.Get());
    return script; 
}

static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0x83;
        pchMessageStart[1] = 0x63;
        pchMessageStart[2] = 0x69;
        pchMessageStart[3] = 0x78;
        vAlertPubKey = ParseHex("04c32c8ab64b43228550115a862847deb294b776a71d6395e9c49477d13eac413f022e40462770dbc665f8a32aeec2a5d87839239f9a0b91a85269f90e79ab0ccc");
        nDefaultPort = 40001;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 1 * 60; // DogeCash: 1 day
        nTargetSpacing = 1 * 60;  // DogeCash: 1 minute
        nLastPOWBlock = 200;
        nMaturity = 15;
        nMasternodeCountDrift = 4;
                nMasternodeCollateralLimit = 5000; //MN collateral

        nModifierUpdateBlock = 51197; //approx Mon, 17 Apr 2017 04:00:00 GMT
        nMaxMoneyOut = 43199500 * COIN;
        nZerocoinStartHeight = 201576;
        nZerocoinStartTime = 1501776000;
        nBlockEnforceSerialRange = 1; //Enforce serial range starting this block
        nBlockRecalculateAccumulators = 9908000; //Trigger a recalculation of accumulators
        nBlockFirstFraudulent = 9891737; //First block that bad serials emerged
        nBlockLastGoodCheckpoint = 9891730; //Last valid accumulator checkpoint
        
        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1538406008; 
        genesis.nBits = 0x1e0ffff0;
      genesis.nNonce = 1383794; 

	    hashGenesisBlock = genesis.GetHash();
        //assert(hashGenesisBlock == uint256("0x000007cff63ef602a51bf074e384b3516f0dd202f14d52f7c8c9b1af9423ab2e"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("testnet.dogecash.io", "testnet.dogecash.io"));         // Single node address
        vSeeds.push_back(CDNSSeedData("testnet1.dogecash.io", "testnet1.dogecash.io"));       // Single node address
        vSeeds.push_back(CDNSSeedData("testnet2.dogecash.io", "testnet2.dogecash.io"));       // Single node address


		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 65);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x02)(0x2D)(0x25)(0x33).convert_to_container<std::vector<unsigned char> >();
		base58Prefixes[EXT_SECRET_KEY] = list_of(0x02)(0x21)(0x31)(0x2B).convert_to_container<std::vector<unsigned char> >();
        // 	BIP44 coin type is from https://github.com/satoshilabs/slips/blob/master/slip-0044.md
        base58Prefixes[EXT_COIN_TYPE] = boost::assign::list_of(0x80)(0x00)(0x00)(0xbc).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "04fc640bba80713c0666acda4d3ffce670307a55f90b703995c830a1e9110b07244508724b7106395f8336c78d3691ae5ba05abe3840f3a7e18d6b95acdd0de71d";
        strObfuscationPoolDummyAddress = "xp87cG8UEQgzs1Bk67Yk884C7pnQfAeo7q";
        nStartMasternodePayments = 1420837558; //Fri, 09 Jan 2015 21:05:58 GMT
        nBudget_Fee_Confirmations = 3; // Number of confirmations for the finalization fee. We have to make this very short
                                       // here because we only have a 8 block finalization window on testnet
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
        pchMessageStart[0] = 0x69;
        pchMessageStart[1] = 0xcf;
        pchMessageStart[2] = 0x7e;
        pchMessageStart[3] = 0xac;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // DogeCash: 1 day
        nTargetSpacing = 1 * 60;        // DogeCash: 1 minutes
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1538406008; 
        genesis.nBits = 0x1e0ffff0;
      genesis.nNonce = 1383794; 

        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 51436;
        //assert(hashGenesisBlock == uint256("0x000008415bdca132b70cf161ecc548e5d0150fd6634a381ee2e99bb8bb77dbb3"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 51478;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval) { nSubsidyHalvingInterval = anSubsidyHalvingInterval; }
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};

static CChainParams* pCurrentParams = 0;

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
