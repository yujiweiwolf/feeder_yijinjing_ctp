期权行情服务器

-------------------------------------------------------------------
测试环境：
服务器IP：114.80.159.20
端口：10000
帐号：axzqcs_tdf
密码：axzqcs_tdf

==============================================================================
安信证券：tdfapi-2.7_axzq
..\..\..\vclibs\boost-1.62.0\include;..\..\..\vclibs\libiconv-1.14\include;..\..\..\vclibs\libopenssl-1.1.0c\include;..\..\..\vclibs\libprotobuf-3.1.0\include;..\..\..\vclibs\libsodium-1.0.11\include;..\..\..\vclibs\libzmq-4.2.0\include;..\..\..\vclibs\libx-1.0\include;..\..\..\vclibs\libcoral-1.0\include;..\..\lib\tdfapi-2.7_axzq\include;%(AdditionalIncludeDirectories)
..\..\..\vclibs\boost-1.62.0\lib\Win32;..\..\..\vclibs\libiconv-1.14\lib\Win32\Release\v140\static;..\..\..\vclibs\libopenssl-1.1.0c\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libprotobuf-3.1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libsodium-1.0.11\lib\Win32\Release\v140\static;..\..\..\vclibs\libzmq-4.2.0\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libx-1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libcoral-1.0\lib\Win32\Release\v140\static;..\..\lib\tdfapi-2.7_axzq\lib\Win32Release;%(AdditionalLibraryDirectories)
#pragma comment(lib, "TDFAPI2.lib")

==============================================================================
东吴证券：tdfapi-2.5_dwzq
..\..\..\vclibs\boost-1.62.0\include;..\..\..\vclibs\libiconv-1.14\include;..\..\..\vclibs\libopenssl-1.1.0c\include;..\..\..\vclibs\libprotobuf-3.1.0\include;..\..\..\vclibs\libsodium-1.0.11\include;..\..\..\vclibs\libzmq-4.2.0\include;..\..\..\vclibs\libx-1.0\include;..\..\..\vclibs\libcoral-1.0\include;..\..\lib\tdfapi-2.5_dwzq\include;%(AdditionalIncludeDirectories)
..\..\..\vclibs\boost-1.62.0\lib\Win32;..\..\..\vclibs\libiconv-1.14\lib\Win32\Release\v140\static;..\..\..\vclibs\libopenssl-1.1.0c\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libprotobuf-3.1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libsodium-1.0.11\lib\Win32\Release\v140\static;..\..\..\vclibs\libzmq-4.2.0\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libx-1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libcoral-1.0\lib\Win32\Release\v140\static;..\..\lib\tdfapi-2.5_dwzq\lib\Win32Release;%(AdditionalLibraryDirectories)
#pragma comment(lib, "TDFAPI25.lib")

==============================================================================
海通证券：tdfapi-3.2.0_htzq（3.2.0不支持订阅日期，使用tdfapi-2.7axzq版本。）
..\..\..\vclibs\boost-1.62.0\include;..\..\..\vclibs\libiconv-1.14\include;..\..\..\vclibs\libopenssl-1.1.0c\include;..\..\..\vclibs\libprotobuf-3.1.0\include;..\..\..\vclibs\libsodium-1.0.11\include;..\..\..\vclibs\libzmq-4.2.0\include;..\..\..\vclibs\libx-1.0\include;..\..\..\vclibs\libcoral-1.0\include;..\..\lib\tdfapi-3.2.0_htzq\include;%(AdditionalIncludeDirectories)
..\..\..\vclibs\boost-1.62.0\lib\Win32;..\..\..\vclibs\libiconv-1.14\lib\Win32\Release\v140\static;..\..\..\vclibs\libopenssl-1.1.0c\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libprotobuf-3.1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libsodium-1.0.11\lib\Win32\Release\v140\static;..\..\..\vclibs\libzmq-4.2.0\lib\Win32\Release\v140\dynamic;..\..\..\vclibs\libx-1.0\lib\Win32\Release\v140\static;..\..\..\vclibs\libcoral-1.0\lib\Win32\Release\v140\static;..\..\lib\tdfapi-3.2.0_htzq\lib\Win32Release;%(AdditionalLibraryDirectories)
#pragma comment(lib, "TDFAPI30.lib")


==============================================================================
停牌状态修正逻辑：
宏汇推过来的股票停牌状态有可能不准，所以需要增加自动修正的逻辑。

对于停牌股票，有可能在开盘前或开盘后推过来一条或多条行情，时间戳可能是八点多，也可能是开盘后，状态有的是停牌有的不是停牌。
情况举例：
1.共推过来两条行情：第一条行情cStatus为“停牌”；第二条cStatus为0，盘口为0。
2.共推过来两条行情，第一条行情cStatus为“正常”；第二条cStatus为“停牌”，但是两条行情的时间戳都是一样的。
3.共推过来一条行情：第一条行情cStatus为0，盘口为0；

==============================================================================
对于指数行情，在9:25分之前可能会推过来几条行情，时间戳都是0，在9:25之后时间戳才正常。

期权交易所时间戳：
在没有任何成交数据之前，期权的交易所时间戳都是0。比如9:20的时候，期权行情里面的时间戳是0.


===========================
股票状态清洗流程：
1.每收到一条股票行情数据，执行如下检查：行情时间>=9:25:00，成交量=0，买一量=0，卖一量=0，将状态置为停牌。
2.当收到第一条时间>=9:26:00.000的股票行情数据时，执行如下操作：
   检查当前接收到的所有股票行情，如果行情原始股票状态为非停牌，行情时间>=9:20，买一量=0，卖一量=0，成交量=0，则将状态置为停牌



#############################################################################
# TDF里面没有提供主买和主卖数据，需要自己根据快照数据计算出大概的值，
备注：
1.这样通过快照算出来的主买和主卖只是个大概的值，因为判断主买和主卖是用的最后一笔成交的成交价格，但是两个快照之间的真实成交价格其实是很多的。
2.涨跌停时需要特殊处理：涨停时的成交算主买，跌停时的成交算主卖；

以下为宏汇的算法：
// PreAks1 前次卖一价
// PreBid1 前次买一价
// Price      最新成交价
// PrePrice 前次成交价
// volume  现量: TotalVolume – PreTotalVolume
// SellVol 卖盘累计
// BuyVol 买盘累计

If (PreTotalVolume == 0) { // 开盘第一笔
         If ( Price < PreClose )    // 小于前收盘
                  SellVol += volume;
else if ( Price > PreClose )   //大于前收盘
BuyVol += volume;
else { // 方向不明
SellVol += volume / 2;
BuyVol += volume / 2;
}
}
else if ( PreAsk1==0 ) // 涨停
         BuyVol += volume;
else if ( PreBid1==0)  // 跌停
         SellVol += volume;
else If   (Price >= PreAsk1 ) // 卖价成交
         BuyVol += volume;
else if ( Price<= PreBid1 ) // 买价成交
        SellVol += volume;
else if ( Price < PrePrice ) // 小于上笔成交
         SellVol += volume;
else if ( Price > PrePrice )  // 大于上笔成交
         BuyVol += volume;
else {  // 方向不明
         SellVol += volume / 2;
         BuyVol += volume / 2;
}
