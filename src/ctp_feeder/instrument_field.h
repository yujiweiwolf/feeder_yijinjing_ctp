// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include <utility>
#include "ctp.h"
#include "singleton.h"
#include <vector>

namespace co {
    class InstrumentMgr {
    public:
        void AddInstrument(CThostFtdcInstrumentField *p) {
            all_instrument_.push_back(*p);
        };
        std::vector<CThostFtdcInstrumentField>& GetInstrument() {
            return all_instrument_;
        }
    private:
        std::vector<CThostFtdcInstrumentField> all_instrument_;
    };
}
