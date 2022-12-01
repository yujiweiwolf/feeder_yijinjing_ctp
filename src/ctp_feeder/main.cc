#include <iostream>
#include <sstream>
#include <stdio.h>
#include <boost/program_options.hpp>
#include <x/x.h>
#include "define.h"

#include "config.h"
#include "ctp_server.h"


using namespace std;
using namespace co;
namespace po = boost::program_options;

void signal_callback(int signum) {
    std::cout << "signal num: " << signum << std::endl;
}

int main(int argc, char* argv[]) {
    std::string kVersion = "v1.0.1";
    try {
//        po::options_description desc("[ctp_feeder] Usage");
//        desc.add_options()
//            ("passwd", po::value<std::string>(), "encode plain password")
//            ("help,h", "show help message")
//            ("version,v", "show version information");
//        po::variables_map vm;
//        po::store(po::parse_command_line(argc, argv, desc), vm);
//        po::notify(vm);
//        if (vm.count("passwd")) {
//			cout << co::EncodePassword(vm["passwd"].as<std::string>()) << endl;
//			return 0;
//		} else if (vm.count("help")) {
//            cout << desc << endl;
//            return 0;
//        } else if (vm.count("version")) {
//            cout << kVersion << endl;
//            return 1;
//        }
        signal(SIGHUP, signal_callback);
        __info << "start ctp feeder: version = " << kVersion << " ...";
        Config::Instance();
        CTPServer server;
        server.Run();
        __info << "server is stopped.";
    } catch (x::Exception& e) {
        std::cout << "server is crashed, " << e.what();
        return 1;
    } catch (std::exception& e) {
        std::cout << "server is crashed, " << e.what();
        return 2;
    } catch (...) {
        std::cout << "server is crashed, unknown reason";
        return 3;
    }
    return 0;
}

