#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include "feeder/feeder.h"

namespace co {
    using namespace std;

    class Config {
    public:
        static Config* Instance();

        inline string ctp_market_front() {
            return ctp_market_front_;
        }
        inline string ctp_trade_front() {
            return ctp_trade_front_;
        }
        inline string ctp_broker_id() {
            return ctp_broker_id_;
        }
        inline string ctp_investor_id() {
            return ctp_investor_id_;
        }
        inline string ctp_password() {
            return ctp_password_;
        }
        inline string ctp_app_id() {
            return ctp_app_id_;
        }
        inline string ctp_product_info() {
            return ctp_product_info_;
        }
        inline string ctp_auth_code() {
            return ctp_auth_code_;
        }

    protected:
        Config() = default;
        ~Config() = default;
        Config(const Config&) = delete;
        const Config& operator=(const Config&) = delete;

        void Init();

    private:
        static Config* instance_;

        string ctp_market_front_;
        string ctp_trade_front_;
        string ctp_broker_id_;
        string ctp_investor_id_;
        string ctp_password_;
        string ctp_app_id_;
        string ctp_product_info_;
        string ctp_auth_code_;
        string sub_code_prefix_;
    };

}