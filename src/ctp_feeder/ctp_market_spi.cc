#include <regex>
#include <boost/algorithm/string.hpp>
#include "ctp_market_spi.h"
#include "instrument_field.h"

namespace co {

    CTPMarketSpi::CTPMarketSpi(CThostFtdcMdApi* qApi) {
        api_ = qApi;
    }

    CTPMarketSpi::~CTPMarketSpi() {

    }

    string InsertCzceCode(const string ctp_code, TThostFtdcProductClassType class_type) {
        string code = ctp_code;
        int index = 0;
        for (int i = 0; i < code.length(); i++) {
            char temp = code.at(i);
            if (temp >= '0' && temp <= '9') {
                break;
            } else {
                index++;
            }
        }
        code.insert(index, "2");
        if (class_type == THOST_FTDC_PC_Combination) {
            index += 5;
            for (int i = index; i < code.length(); i++) {
                char temp = code.at(i);
                if (temp >= '0' && temp <= '9') {
                    break;
                } else {
                    index++;
                }
            }
            code.insert(index, "2");
        }
        return code;
    }

    void CTPMarketSpi::Init() {
        string journal_dir = Config::Instance()->journal_dir();
        string journal_file = Config::Instance()->journal_file() + "_" + std::to_string(x::RawDate());
        feeder_writer_ = yijinjing::JournalWriter::create(journal_dir.c_str(), journal_file.c_str(), "Client");
        vector<CThostFtdcInstrumentField>& all_instrument = Singleton<InstrumentMgr>::GetInstance()->GetInstrument();
        for (auto it : all_instrument) {
            string ctp_code = it.InstrumentID;
            int8_t market = ctp_market2std(it.ExchangeID);
            if (market == 0) {
                continue;
            }
            string suffix = market_suffix(market);
            string std_code = ctp_code + suffix;
            if (market == co::kMarketCZCE) {
                std_code = InsertCzceCode(ctp_code, it.ProductClass) + suffix;
            }
            int status = it.IsTrading ? kStateOK : kStateSuspension;
            bool is_future = (it.ProductClass == THOST_FTDC_PC_Futures || it.ProductClass == THOST_FTDC_PC_Combination);
            bool is_option = (it.ProductClass == THOST_FTDC_PC_Options || it.ProductClass == THOST_FTDC_PC_SpotOption);
            bool enable_future = Config::Instance()->enable_future();
            bool enable_option = Config::Instance()->enable_option();

            QTickT m;
            memset(&m, 0, sizeof(QTickT));
            m.src = kSrcQTickLevel1;
            m.dtype = is_future ? kDTypeFuture : kDTypeOption;
            m.timestamp = x::RawDate() * 1000000000;
            strcpy(m.code, std_code.c_str());
            strcpy(m.name, ctp_str(it.InstrumentName).c_str());
            m.market = market;
            m.status = status;
            m.multiple = it.VolumeMultiple >= 0 ? it.VolumeMultiple : 0;
            m.price_step = it.PriceTick;
            m.create_date = atoi(it.CreateDate);
            m.list_date = atoi(it.OpenDate);
            m.expire_date = atoi(it.ExpireDate);
            m.start_settle_date = atoi(it.StartDelivDate);
            m.end_settle_date = atoi(it.EndDelivDate);
            if (is_option) {
                m.exercise_date = m.expire_date;
                m.exercise_price = it.StrikePrice;
                strcpy(m.underlying_code, x::Trim(it.UnderlyingInstrID).c_str());
                m.cp_flag = it.OptionsType == THOST_FTDC_CP_CallOptions ? kCpFlagCall : kCpFlagPut;
            }
            m.new_volume = 0;
            m.new_amount = 0;
            m.sum_volume = 0;
            m.sum_amount = 0;
            all_ticks_.insert(std::make_pair(ctp_code, m));
        }
    }

    void CTPMarketSpi::OnFrontConnected() {
        __info << "connect to CTP market server ok, ApiVersion: " << api_->GetApiVersion();
        ReqUserLogin();
    };

    void CTPMarketSpi::OnFrontDisconnected(int nReason) {
        stringstream ss;
        ss << "ret=" << nReason << ", msg=";
        switch (nReason) {
        case 0x1001:
            ss << "read error";
            break;
        case 0x1002:
            ss << "write error";
            break;
        case 0x2001:
            ss << "recv heartbeat timeout";
            break;
        case 0x2002:
            ss << "send heartbeat timeout";
            break;
        case 0x2003:
            ss << "recv broken data";
            break;
        default:
            ss << "unknown error";
            break;
        }
        __info << "connection is broken: " << ss.str();
        x::Sleep(10000);
    };

    void CTPMarketSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID == 0) {
            int64_t login_trading_day = atol(api_->GetTradingDay());
            local_date_ = x::RawDate();
            if (local_date_ != login_trading_day) {
                local_next_date_ = x::NextDay(local_date_);
            }
            __info << "OnRspUserLogin, login_trading_day = " << login_trading_day << ", local_date: " << local_date_
                   << ", local_next_date: " << local_next_date_;
            ReqSubMarketData();
        } else {
            __error << "login failed: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
        }
    };

    void CTPMarketSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        string code = pSpecificInstrument->InstrumentID;
        bool sub_ok = pRspInfo->ErrorID == 0 ? true : false;
        sub_status_[code] = sub_ok;
        int success = 0;
        int failure = 0;
        for (auto p : sub_status_) {
            if (p.second == 1) {
                success++;
            } else if (p.second == -1) {
                failure++;
            }
        }
        int count = success + failure;
        if (sub_ok) {
            __info << "[" << count << "/" << sub_status_.size() << "] sub ok: code = " << code;
        } else {
            __error << "[" << count << "/" << sub_status_.size() << "] sub failed: code = "
                << code << ", error = " << pRspInfo->ErrorID << "-" << ctp_str(pRspInfo->ErrorMsg);
        }
        if (count >= (int)sub_status_.size()) {
            __info << "sub quotation over, total: " << sub_status_.size() << ", success: " << success << ", failure: " << failure;
            __info << "server startup successfully";
        }
    };

    //void CTPMarketSpi::OnRspQryMulticastInstrument(CThostFtdcMulticastInstrumentField* pMulticastInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    //    __info << "OnRspQryMulticastInstrument, InstrumentID: " << pMulticastInstrument->InstrumentID
    //        << ", InstrumentNo: " << pMulticastInstrument->InstrumentID
    //        << ", TopicID: " << pMulticastInstrument->TopicID
    //        << ", reserve1: " << pMulticastInstrument->reserve1
    //        << ", CodePrice: " << pMulticastInstrument->CodePrice
    //        << ", VolumeMultiple: " << pMulticastInstrument->VolumeMultiple
    //        << ", PriceTick: " << pMulticastInstrument->PriceTick;
    //}

    int64_t GetUpdateTime(char* data) {
        int64_t timestamp = 0;
        if (strlen(data)!= 8) {
            return timestamp;
        }
        int index = 5;
        for (int i = 0; i < 8; i++) {
            char letter = *(data + i);
            if (letter >= '0' && letter <= '9') {
                timestamp += (int)(letter - '0') * pow(10, index--);
            }
        }
        return timestamp;
    }

    void CTPMarketSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *p) {
        if (!p) {
            return;
        }
        string ctp_code = x::Trim(p->InstrumentID);
        int64_t date = atoi(p->TradingDay);
        if (date < local_date_) {
            __warn << "ignore data with unexpected date: code = " << ctp_code << ", TradingDay = " << date << ", local_date = " << local_date_;
            return;
        }

        int64_t stamp = GetUpdateTime(p->UpdateTime)* 1000LL + p->UpdateMillisec;
        bool first_flag = false;
        auto it = all_ticks_.find(ctp_code);
        if (it != all_ticks_.end()) {
            if (it->second.sum_volume == 0) {
                first_flag = true;
                __info << "monitor info, code: " << ctp_code
                       << ", ExchangeID: " << p->ExchangeID
                       << ", UpdateTime: " << p->UpdateTime;
            }
        } else {
            __error << "not valid ctp_code: " << ctp_code;
            return;
        }

        bool not_valid_flag = false;
        if (stamp < 0 || stamp > 235959999) {
            not_valid_flag = true;
        }

        if (stamp > 55959999 && stamp < 83959999) {
            not_valid_flag = true;
        }

        if (stamp > 155959999 && stamp < 203959999) {
            not_valid_flag = true;
        }

        if (not_valid_flag && !first_flag) {
            __warn << "ignore data with unexpected time: code = " << ctp_code << ", UpdateTime = " << p->UpdateTime << ", UpdateMillisec = " << p->UpdateMillisec;
            return;
        }
        int64_t timestamp = 0;
        if (stamp < 55959999) {
            timestamp = local_next_date_ * 1000000000LL + stamp;
        } else {
            timestamp = local_date_ * 1000000000LL + stamp;
        }
        Frame frame = feeder_writer_->locateFrame();
        QTickT* tick = (QTickT*)(frame.getData());
        tick->receive_time = getNanoTime();
        tick->timestamp = timestamp;
        tick->pre_close = ctp_price(p->PreClosePrice);
        tick->upper_limit = ctp_price(p->UpperLimitPrice);
        tick->lower_limit = ctp_price(p->LowerLimitPrice);

        if (p->BidVolume1 > 0) {
            tick->bp[0] = ctp_price(p->BidPrice1);
            tick->bv[0] = p->BidVolume1;
            if (p->BidVolume2 > 0) {
                tick->bp[1] = ctp_price(p->BidPrice2);
                tick->bv[1] = p->BidVolume2;
                if (p->BidVolume3 > 0) {
                    tick->bp[2] = ctp_price(p->BidPrice3);
                    tick->bv[2] = p->BidVolume3;
                    if (p->BidVolume4 > 0) {
                        tick->bp[3] = ctp_price(p->BidPrice4);
                        tick->bv[3] = p->BidVolume4;
                        if (p->BidVolume5 > 0) {
                            tick->bp[4] = ctp_price(p->BidPrice5);
                            tick->bv[4] = p->BidVolume5;
                        }
                    }
                }
            }
        }

        if (p->AskVolume1 > 0) {
            tick->ap[0] = ctp_price(p->AskPrice1);
            tick->av[0] = p->AskVolume1;
            if (p->AskVolume2 > 0) {
                tick->ap[1] = (ctp_price(p->AskPrice2));
                tick->av[1] = (p->AskVolume2);
                if (p->AskVolume3 > 0) {
                    tick->ap[2] = ctp_price(p->AskPrice3);
                    tick->av[2] = p->AskVolume3;
                    if (p->AskVolume4 > 0) {
                        tick->ap[3] = ctp_price(p->AskPrice4);
                        tick->av[3] = p->AskVolume4;
                        if (p->AskVolume5 > 0) {
                            tick->ap[4] = ctp_price(p->AskPrice5);
                            tick->av[4] = p->AskVolume5;
                        }
                    }
                }
            }
        }

        int64_t sum_volume = p->Volume;
        double sum_amount = tick->market == co::kMarketCZCE && tick->multiple > 0 ? p->Turnover * tick->multiple : p->Turnover;
        if (not_valid_flag) {
            tick->new_volume = 0;
            tick->new_amount = 0;
        } else {
            if (first_flag) {
                tick->new_volume = 0;
                tick->new_amount = 0;
            } else {
                tick->new_volume = sum_volume - it->second.sum_volume;
                tick->new_amount = sum_amount - it->second.sum_amount;
            }
        }
        it->second.sum_volume = sum_volume;
        it->second.sum_amount = sum_amount;
        tick->sum_volume = sum_volume;
        tick->sum_amount = sum_amount;
        tick->new_price = ctp_price(p->LastPrice);
        tick->open = ctp_price(p->OpenPrice);
        tick->high = ctp_price(p->HighestPrice);
        tick->low = ctp_price(p->LowestPrice);
        tick->open_interest = (int64_t)p->OpenInterest;
        tick->pre_settle = ctp_price(p->PreSettlementPrice);
        tick->pre_open_interest = (int64_t)p->PreOpenInterest;
        tick->close = ctp_price(p->ClosePrice);
        tick->settle = ctp_price(p->SettlementPrice);
        {
            tick->dtype = it->second.dtype;
            strcpy(tick->code, it->second.code);
            strcpy(tick->name, it->second.name);
            tick->market = it->second.market;
            tick->status = it->second.status;
            tick->multiple = it->second.multiple;
            tick->price_step = it->second.price_step;
            tick->create_date = it->second.create_date;
            tick->list_date = it->second.list_date;
            tick->expire_date = it->second.expire_date;
            tick->start_settle_date = it->second.start_settle_date;
            tick->end_settle_date = it->second.end_settle_date;
            if (it->second.dtype == kDTypeOption) {
                tick->exercise_date = it->second.exercise_date;
                tick->exercise_price = it->second.exercise_price;
                strcpy(tick->underlying_code, it->second.underlying_code);
                tick->cp_flag = it->second.cp_flag;
            }
        }
        tick->write_time = getNanoTime();
        feeder_writer_->passFrame(frame, sizeof(QTickT), FEEDER_TICK, 0);
        // __info << "write, " << tick->code << ", " << tick->timestamp;
    };

    void CTPMarketSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        __error << "OnRspError: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
    };

    int CTPMarketSpi::next_id() {
        return ctp_request_id_++;
    }

    void CTPMarketSpi::ReqUserLogin() {
        __info << "login ...";
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, Config::Instance()->ctp_broker_id().c_str());
        strcpy(req.UserID, Config::Instance()->ctp_investor_id().c_str());
        strcpy(req.Password, Config::Instance()->ctp_password().c_str());
        int rc = 0;
        while ((rc = api_->ReqUserLogin(&req, next_id())) != 0) {
            __warn << "ReqUserLogin failed: ret=" << rc << ", retring ...";
            x::Sleep(kCtpRetrySleepMs);
        }
    }

    void CTPMarketSpi::ReqSubMarketData() {
        __info << "sub quotation ...";
        string sub_instrument = Config::Instance()->sub_instrument();
        vector<string> vec_info;
        boost::split(vec_info, sub_instrument, boost::is_any_of(";"), boost::token_compress_on);
        bool enable_future = Config::Instance()->enable_future();
        bool enable_option = Config::Instance()->enable_option();
        sub_status_.clear();
        vector<string> sub_codes;
        for (auto& it : all_ticks_) {
            bool need_flag = false;
            if (enable_future && it.second.dtype == kDTypeFuture) {
                if (sub_instrument.empty()) {
                    need_flag = true;
                } else {
                    string product = get_product(it.first);
                    for (auto& itor : vec_info) {
                        if (itor == product) {
                            need_flag = true;
                            __info << "need, " << it.first << ", type:" << (int)it.second.dtype << ", product:" << product;
                            break;
                        }
                    }
                }
            } else if (enable_option && it.second.dtype == kDTypeOption) {
                if (sub_instrument.empty()) {
                    need_flag = true;
                } else {
                    string product = get_product(it.first);
                    for (auto& itor : vec_info) {
                        if (itor == product) {
                            need_flag = true;
                            __info << "need option, " << it.first << ", type:" << (int)it.second.dtype << ", product:" << product;
                            break;
                        }
                    }
                }
            }

            if (need_flag) {
                sub_status_[it.first] = false;
                sub_codes.push_back(it.first);
            }
        }

        size_t limit = 100;
        for (size_t i = 0; i < sub_codes.size(); ) {
            size_t size = i + limit <= sub_codes.size() ? limit : sub_codes.size() - i;
            char** pCodes = new char*[size];
            memset(pCodes, 0, sizeof(char*) * size);
            for (size_t j = i, k = 0; j < i + size; j++, k++) {
                pCodes[k] = (char *)sub_codes[j].c_str();
            }
            int rc = 0;
            while ((rc = api_->SubscribeMarketData(pCodes, size)) != 0) {
                __warn << "SubscribeMarketData failed: ret=" << rc << ", retring ...";
                x::Sleep(kCtpRetrySleepMs);
            }
            delete[] pCodes;
            i += size;
        }
    }
}

