#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include "ca_test.h"
#include "utils/console.h"
#include "utils/hexcode.h"
#include "../include/logging.h"
#include "utils/base58.h"
#include "ca/ca_global.h"
#include "utils/json.hpp"
#include "utils/time_util.h"
#include "utils/MagicSingleton.h"
#include "db/db_api.h"

int PrintFormatTime(uint64_t time, bool isConsoleOutput, std::ostream & stream)
{
    time_t s = (time_t)(time / 1000000);
    struct tm * gm_date;
    gm_date = localtime(&s);

	ca_console tmColor(kConsoleColor_White, kConsoleColor_Black, true);
    if(isConsoleOutput)
	{
        stream << tmColor.color() << gm_date->tm_year + 1900 << "-" << gm_date->tm_mon + 1 << "-" << gm_date->tm_mday << " "  << gm_date->tm_hour << ":" << gm_date->tm_min << ":" << gm_date->tm_sec << "(" << time << ")" << tmColor.reset() << std::endl;
	}
	else
	{
        stream << gm_date->tm_year + 1900 << "-" << gm_date->tm_mon + 1 << "-" << gm_date->tm_mday << " "  << gm_date->tm_hour << ":" << gm_date->tm_min << ":" << gm_date->tm_sec << "(" << time << ")" << std::endl;
    }

    return 0;
}

int printRocksdb(uint64_t start, uint64_t end, bool isConsoleOutput, std::ostream & stream)
{
    if(start > end)
    {
        ERRORLOG("start > end");
        return -1;
    }

	DBReader db_reader;
    ca_console bkColor(kConsoleColor_Blue, kConsoleColor_Black, true);
    uint64_t count = 0 ;
    uint64_t height = end;
    for (; height >= start; --height) 
    {
	    std::vector<std::string> vBlockHashs;
	    db_reader.GetBlockHashsByBlockHeight(height, vBlockHashs);
        std::vector<CBlock> blocks;
        for (auto hash : vBlockHashs)
        {
            std::string strHeader;
            db_reader.GetBlockByBlockHash(hash, strHeader);
            CBlock block;
            block.ParseFromString(strHeader);
            blocks.push_back(block);
        }
        std::sort(blocks.begin(), blocks.end(), [](CBlock & a, CBlock & b){
            return a.time() < b.time();
        });

        count ++;
        std::cout << "Progress------>" << count << "/" << end << std::endl;
        for (auto & block : blocks)
        {

            printBlock(block, isConsoleOutput, stream);
        }
        if(height == start)break;
    }
    
    return 0;
}

int printBlock(const CBlock & block, bool isConsoleOutput, std::ostream & stream)
{
    ca_console bkColor(kConsoleColor_Blue, kConsoleColor_Black, true);
    ca_console greenColor(kConsoleColor_Green, kConsoleColor_Black, true);
    stream << std::endl << "BlockInfo ---------------------- > height [" << block.height() << "]" << std::endl;
    stream << "HashMerkleRoot -> " << block.merkleroot() << std::endl;
    stream << "HashPrevBlock -> " << block.prevhash() << std::endl;
    if (isConsoleOutput)
    {
        stream << "BlockHash -> " << bkColor.color() << block.hash() << bkColor.reset() << std::endl;
    }
    else
    {
        stream << "BlockHash -> " << block.hash() << std::endl;
    }

    stream << "blockverifySign[" << block.sign_size() << "]" << std::endl;
    for (auto & verifySign : block.sign())
    {
        stream << "block Verify Sign " << Str2Hex(verifySign.sign()) << " : " << Str2Hex(verifySign.pub()) << "[" << greenColor.color() << GetBase58Addr(verifySign.pub()) << greenColor.reset() << "]" << std::endl;
    }
    
    for (auto & verifySign : block.sign())
    {
        stream << "block signer -> [" << greenColor.color() << GetBase58Addr(verifySign.pub()) << greenColor.reset() << "]" << std::endl;
    }
    
    stream << "Time-> ";
    PrintFormatTime(block.time(), isConsoleOutput, stream);
    
    for (int i = 0; i < block.txs_size(); i++) 
    {
        CTransaction tx = block.txs(i);
        stream << "TX_INFO -----------> index[" << i << "]" << std::endl;
        printTx(tx, isConsoleOutput, stream);
    }

    return 0;
}

std::string printBlocks(int num, bool pre_hash_flag)
{
    DBReader db_read;
    uint64_t top = 0;
    db_read.GetBlockTop(top);
    std::string str = "top:\n";
    str += "--------------\n";
    int j = 0;
    for(int i = top; i >= 0; i--){
        str += (std::to_string(i) + "\t");
        std::vector<std::string> vBlockHashs;
        db_read.GetBlockHashsByBlockHeight(i, vBlockHashs);
        std::sort(vBlockHashs.begin(), vBlockHashs.end());
        for (auto hash : vBlockHashs) {
            std::string strHeader;
            db_read.GetBlockByBlockHash(hash, strHeader);
            CBlock header;
            header.ParseFromString(strHeader);
            if(pre_hash_flag)
            {
                str = str + hash.substr(0,6) + "(" + header.prevhash().substr(0,6) + ")" + " ";
            }else{
                str = str + hash.substr(0,6) + " ";
            }
        }
        str += "\n";
        j++;
        if(num > 0 && j >= num)
        {
            break;
        }
    }
    str += "--------------\n";
    return str;
}

int printTx(const CTransaction & tx, bool isConsoleOutput, std::ostream & stream)
{
    if (isConsoleOutput)
    {
        ca_console txColor(kConsoleColor_Red, kConsoleColor_Black, true);
        ca_console greenColor(kConsoleColor_Green, kConsoleColor_Black, true);
        stream << "TxHash -> " << txColor.color() << tx.hash() << txColor.reset() << std::endl;
        stream << "n -> " << tx.n() << std::endl;
        stream << "identity -> " << "[" << greenColor.color() << tx.identity() << greenColor.reset() << "] " << std::endl;
        stream << "type -> " << tx.type() << std::endl;

        stream << "verifySign[" << tx.verifysign_size() << "]" << std::endl;

        for (auto & verifySign : tx.verifysign())
        {
            stream << "Verify Sign " << Str2Hex(verifySign.sign()) << " : " << Str2Hex(verifySign.pub()) << "[" << greenColor.color() << GetBase58Addr(verifySign.pub()) << greenColor.reset() << "]" << std::endl;
        }
        
        for (auto & verifySign : tx.verifysign())
        {
            stream << "Transaction signer -> [" << greenColor.color() << GetBase58Addr(verifySign.pub()) << greenColor.reset() << "]" << std::endl;
        }

        stream << "Owner -> ";
        for (auto & addr : tx.utxo().owner())
        {
            stream << "[" << greenColor.color() << addr << greenColor.reset() << "]";
        }
        stream << std::endl;

        for (int j = 0; j < tx.utxo().vin_size(); j++)
        {
            const CTxInput & vin = tx.utxo().vin(j);
            stream << "vin[" << j << "] sequence -> " << vin.sequence() << std::endl;
            stream << "vin[" << j << "] sign -> " << Str2Hex(vin.vinsign().sign()) << " : " << Str2Hex(vin.vinsign().pub()) << "[" << greenColor.color() << GetBase58Addr(vin.vinsign().pub()) << greenColor.reset() << "]" << std::endl;

            for (auto & prevout : vin.prevout())
            {
                stream << "vin[" << j << "] Prev Output Hash -> " << prevout.n() << " : " << prevout.hash() << std::endl;
            }
        }

        for (int j = 0; j < tx.utxo().vout_size(); j++)
        {
            const CTxOutput & vout = tx.utxo().vout(j);
            ca_console amount(kConsoleColor_Yellow, kConsoleColor_Black, true);
            stream << "vout[" << j << "] public key -> [" << greenColor.color() <<  vout.addr() << greenColor.reset() << "]" << std::endl;
            stream << "vout[" << j << "] value -> [" << amount.color() <<  vout.value() << amount.reset() << "]" << std::endl;
        }

        for (int j = 0; j < tx.utxo().multisign_size(); j++)
        {
            const CSign & multiSign = tx.utxo().multisign(j);
            stream << "multiSign[" << j << "] -> " << Str2Hex(multiSign.sign()) << " : " << Str2Hex(multiSign.pub()) << "[" << greenColor.color() << GetBase58Addr(multiSign.pub()) << greenColor.reset() << "]" << std::endl;
        }
    }
    else
    {
        stream << "TxHash -> " << tx.hash() << std::endl;
        stream << "n -> " << tx.n() << std::endl;
        stream << "identity -> " << tx.identity() << std::endl;
        stream << "type -> " << tx.type() << std::endl;

        stream << "verifySign[" << tx.verifysign_size() << "]" << std::endl;

        for (auto & verifySign : tx.verifysign())
        {
            stream << "Verify Sign " << Str2Hex(verifySign.sign()) << " : " << Str2Hex(verifySign.pub()) << "[" << GetBase58Addr(verifySign.pub()) << "]" << std::endl;
        }
        
        for (auto & verifySign : tx.verifysign())
        {
            stream << "Transaction signer -> [" << GetBase58Addr(verifySign.pub()) << "]" << std::endl;
        }

        stream << "Owner -> ";
        for (auto & addr : tx.utxo().owner())
        {
            stream << "[" << addr << "]";
        }
        stream << std::endl;

        for (int j = 0; j < tx.utxo().vin_size(); j++)
        {
            const CTxInput & vin = tx.utxo().vin(j);
            stream << "vin[" << j << "] sequence -> " << vin.sequence() << std::endl;
            stream << "vin[" << j << "] sign -> " << Str2Hex(vin.vinsign().sign()) << " : " << Str2Hex(vin.vinsign().pub()) << "[" << GetBase58Addr(vin.vinsign().pub()) << "]" << std::endl;

            for (auto & prevout : vin.prevout())
            {
                stream << "vin[" << j << "] Prev Output Hash -> " << prevout.n() << " : " << prevout.hash() << std::endl;
            }
        }

        for (int j = 0; j < tx.utxo().vout_size(); j++)
        {
            const CTxOutput & vout = tx.utxo().vout(j);
            stream << "vout[" << j << "] public key -> [" << vout.addr() << "]" << std::endl;
            stream << "vout[" << j << "] value -> [" << vout.value() << "]" << std::endl;
        }

        for (int j = 0; j < tx.utxo().multisign_size(); j++)
        {
            const CSign & multiSign = tx.utxo().multisign(j);
            stream << "multiSign[" << j << "] -> " << Str2Hex(multiSign.sign()) << " : " << Str2Hex(multiSign.pub()) << GetBase58Addr(multiSign.pub()) << std::endl;
        }
    }

    stream << "Time -> " << MagicSingleton<TimeUtil>::GetInstance()->formatUTCTimestamp(tx.time()) << std::endl;
    stream << "(" << tx.time() <<")" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> dataMap;
    std::string strData;
    if((global::ca::TxType)tx.txtype() != global::ca::TxType::kTxTypeTx)
    {
        try
        {
            nlohmann::json data_json = nlohmann::json::parse(tx.data());
            global::ca::TxType txType = (global::ca::TxType)tx.txtype();
            dataMap.push_back(std::make_pair("TxType", std::to_string((int32_t)txType)));
            dataMap.push_back(std::make_pair("Consensus", std::to_string(tx.consensus())));
            dataMap.push_back(std::make_pair("Gas", std::to_string(tx.gas())));
            dataMap.push_back(std::make_pair("Cost", std::to_string(tx.cost())));
            if (txType == global::ca::TxType::kTxTypeTx)
            {
                // nothing to do
            }
            else if (txType == global::ca::TxType::kTxTypeStake)
            {
                dataMap.push_back(std::make_pair("StakeType", data_json["TxInfo"]["StakeType"].get<std::string>()));
                dataMap.push_back(std::make_pair("StakeAmount", std::to_string(data_json["TxInfo"]["StakeAmount"].get<uint64_t>())));
            }
            else if (txType == global::ca::TxType::kTxTypeUnstake)
            {
                dataMap.push_back(std::make_pair("UnstakeUtxo", data_json["TxInfo"]["UnstakeUtxo"].get<std::string>()));
            }
            
            for (auto & item : dataMap)
            {
                strData += "  " + item.first + " : " + item.second + "\n";
            }
        }
        catch (...)
        {
        }
    }

    stream << "data -> " << std::endl;
    stream << strData;
    stream << "version -> " << tx.version() << std::endl;
    
    return 0;
}

