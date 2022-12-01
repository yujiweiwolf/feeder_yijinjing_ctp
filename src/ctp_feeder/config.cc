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
		// opt_ = QOptions::Load(filename);
        ctp_market_front_ = ini.get<string>("ctp.ctp_market_front");
        ctp_trade_front_ = ini.get<string>("ctp.ctp_trade_front");
        ctp_broker_id_ = ini.get<string>("ctp.ctp_broker_id");
        ctp_investor_id_ = ini.get<string>("ctp.ctp_investor_id");
        ctp_password_ = ini.get<string>("ctp.ctp_password");
        ctp_app_id_ = ini.get<string>("ctp.ctp_app_id");
        ctp_product_info_ = ini.get<string>("ctp.ctp_product_info");
        ctp_auth_code_ = ini.get<string>("ctp.ctp_auth_code");
		stringstream ss;
		ss << "+-------------------- configuration begin --------------------+" << endl;
		// ss << opt_->ToString() << endl;
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