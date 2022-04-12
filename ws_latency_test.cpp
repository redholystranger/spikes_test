#include <cstdlib>
#include <string>
#include <chrono>
#include <stdio.h>
#include <sys/time.h>

#include "TimeUtils.h"

#define USE_LIB_SSL
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include <rapidjson/document.h>
#include "websocket/client_wss.hpp"

using namespace std::chrono;

using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

class BitfinexWs {
  public:
    BitfinexWs( const std::string& path )
      : active_(false)
      , client_(path, false)

    {
       if (path.length() == 0) {
         std::cout << "Do not start BFX due to path is empty\n";
         return;
       }
       std::srand( std::time(nullptr) );
       client_.on_open = [&, this, path](std::shared_ptr<WssClient::Connection> connection) {
       };

       client_.on_close = [&](std::shared_ptr<WssClient::Connection> /*connection*/, int status, const std::string & /*reason*/) {
         std::cout << "Client: Closed connection with status code: " <<  status << std::endl;
         exit(1);
       };

       client_.on_message = [&](std::shared_ptr<WssClient::Connection> connection, std::shared_ptr<WssClient::Message> message) {
          int64_t timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
          active_ = true;
          std::string msg(message->string().c_str());

          rapidjson::Document json_msg; 
          if ( json_msg.Parse( msg.c_str() ).HasParseError()) {
            std::cout << "bitfinex: Invalid Json: " << msg << std::endl;
            return;
          }

          if ( json_msg.IsArray() && json_msg.GetArray().Size() == 3 && json_msg.GetArray()[0] != 0) {
            auto ts = json_msg.GetArray()[2].GetUint64();

            auto dela = timestamp - ts * 1000ULL;
            std::cout << timeStampToHReadble(ts/1000) << ts%1000 << " Delta " << dela << std::endl;
          }
          check_send();
       };
    }

    std::string timeStampToHReadble(const time_t unix_timestamp) {
      char time_buf[80];
      struct tm ts;
      ts = *localtime(&unix_timestamp);
      strftime(time_buf, sizeof(time_buf), "%Y%m%d-%H:%M:%S.", &ts);
      
      return time_buf;
    }

    bool run() {
      while(true) {
        active_ = false;
        client_.start();
        std::cout << "Bitfinex connection closed" << std::endl;
        exit(1);
      }
    }

    void check_send() {
      if ( !active_ ) { return; }
      auto it = msgs_.begin();
      auto end = msgs_.end();
      while ( it != end ) {
        send( *it );
        it = msgs_.erase( it );
      }
    }

    bool send(const std::string& msg) {
      if ( !active_ ) {
        msgs_.emplace_back( msg );
        return true;
      }
      auto send_stream = std::make_shared<WssClient::SendStream>();
      *send_stream << msg;
      client_.send( send_stream );
      return true;
    }

    void subscribe_R0_pair(const std::string& pair) {
      std::string cmd = "{\"event\":\"subscribe\",\"channel\":\"book\",\"prec\":\"R0\",\"len\":\"250\",\"symbol\":\"" + pair + "\"}";
      send(cmd);
    }

  private:
    bool active_;
    WssClient client_;
    std::list<std::string> msgs_;
};


int main( int argc, char** argv ) {

  try {
    std::vector<std::string> pairs = {"ALGBTC","ALGUST","ANCUSD","ANTETH","ANTUSD","ATOUST","AXSUST","B21X:USD","BALUSD","BAND:UST","BATUSD","BATUST","BTCMIM","BTC:XAUT","BTGUSD","CCDUSD","CHZUSD","CHZUST","COMP:UST","CRVUSD","CRVUST","CTKUSD","DAIUSD","DOGBTC","DOGE:BTC","DOGE:UST","DOGUSD","DORA:UST","DOTUST","EOSBTC","EOSUSD","EOSUST","ETCUSD","ETH2X:UST","ETHBTC","ETHUSD","ETPBTC","EUTUSD","FORTH:USD","FTMUST","FTTUSD","GTXUSD","GTXUST","HIXUSD","ICPUST","IDXUST","IOTBTC","IOTUSD","IQXUSD","JASMY:USD","JSTUST","KAIUSD","KAIUST","KANUSD","KSMUST","LEOEOS","LINK:UST","LTCUSD","LUNA:BTC","LUNA:UST","MATIC:BTC","MATIC:USD","MIMUSD","MIMUST","MIRUSD","MNABTC","NEOETH","NEOUSD","NEOUST","NEXO:BTC","NEXO:USD","OCEAN:USD","OXYUSD","PAXUSD","PAXUST","PLANETS:UST","POLC:UST","QTFUSD","REEF:USD","ROSE:USD","SENATE:UST","SGBUST","SHIB:USD","SHIB:UST","SIDUS:USD","SIDUS:UST","SOLUSD","SOLUST","SPELL:MIM","SPELL:UST","SUKU:UST","SUNUSD","SUNUST","SUSHI:UST","SXXUSD","TERRAUST:UST","TLOS:USD","TRXUSD","TSDUST","UDCUST","USTUSD","VELO:USD","VELO:UST","VETBTC","VETUST","VSYUSD","WAVES:USD","WNCG:USD","WNCG:UST","XLMUST","XMRBTC","XMRUST","XRDBTC","XRDUSD","XTZBTC","XTZUSD","XTZUST","ZECBTC","ZILBTC","ZILUSD","ZRXBTC"};

    //std::string bfx_ws = "api.staging.bitfinex.com/ws/2/";
    std::string bfx_ws = "api.bitfinex.com/ws/2/";
    auto session = BitfinexWs( bfx_ws );

    session.send( R"({"event": "conf", "flags": 32768 })" );
    for (const auto& pair : pairs) {
      session.subscribe_R0_pair( "t" + pair );
    }

    session.run();
    std::cout << "stop session" << std::endl;
    return 0;

  } catch ( std::exception & e ) {
    std::cout << e.what() << std::endl;
    return 1;
  }

}

