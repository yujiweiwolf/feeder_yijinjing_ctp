#include <mutex>
#include <thread>

#include <x/x.h>

#include "config.h"


namespace co {

    Config* Config::instance_ = 0;

    Config* Config::Instance() {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            if (instance_ == 0) {
                instance_ = new Config();
                instance_->Init();
            }
        });
        return instance_;
    }

    void Config::Init() {
        string filename = x::FindFile("config.ini");
        x::INI ini = x::Ini(filename);
        ctp_market_front_ = ini.get<string>("ctp.ctp_market_front");
        ctp_trade_front_ = ini.get<string>("ctp.ctp_trade_front");
        ctp_broker_id_ = ini.get<string>("ctp.ctp_broker_id");
        ctp_investor_id_ = ini.get<string>("ctp.ctp_investor_id");
        ctp_password_ = DecodePassword(ini.get<string>("ctp.ctp_password"));
        ctp_app_id_ = ini.get<string>("ctp.ctp_app_id");
        ctp_product_info_ = ini.get<string>("ctp.ctp_product_info");
        ctp_auth_code_ = ini.get<string>("ctp.ctp_auth_code");

        journal_dir_ = ini.get<string>("feeder.journal_dir");
        journal_file_ = ini.get<string>("feeder.journal_file");
        sub_instrument_ = ini.get<string>("feeder.sub_instrument");
        enable_spot_ = ini.get<string>("feeder.enable_spot") == "true";
        enable_future_ = ini.get<string>("feeder.enable_future") == "true";
        enable_option_ = ini.get<string>("feeder.enable_option") == "true";
        stringstream ss;
        ss << "+-------------------- configuration begin --------------------+" << endl;
        ss << endl;
        ss << "feeder:" << endl
           << "  journal_dir: " << journal_dir_ << endl
           << "  journal_file: " << journal_file_ << endl
           << "  sub_instrument: " << sub_instrument_ << endl
           << "  enable_spot:        " << (enable_spot_ ? "true" : "false") << endl
           << "  enable_future:        " << (enable_future_ ? "true" : "false") << endl
           << "  enable_option:        " << (enable_option_ ? "true" : "false") << endl;
        ss << endl;
        ss << "ctp:" << endl
            << "  ctp_market_front: " << ctp_market_front_ << endl
            << "  ctp_trade_front: " << ctp_trade_front_ << endl
            << "  ctp_broker_id: " << ctp_broker_id_ << endl
            << "  ctp_investor_id: " << ctp_investor_id_ << endl
            << "  ctp_password: " << string(ctp_password_.size(), '*') << endl
            << "  ctp_app_id: " << ctp_app_id_ << endl
            << "  ctp_product_info: " << ctp_product_info_ << endl
            << "  ctp_auth_code: " << ctp_auth_code_ << endl;
        ss << "+-------------------- configuration end   --------------------+";
        __info << endl << ss.str();
    }

}