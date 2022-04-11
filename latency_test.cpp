#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <list>
#include <map>
#include <tuple>
#include <vector>

#include "TimeUtils.h"
#include "socket.h"
using namespace std::chrono;


std::vector<std::string> split(const char *str, size_t len, char c = ' ') {
  std::vector<std::string> result;
  if ( str == nullptr ) return result;
  const char *begin = str;
  const char *end = str + len;
  for ( int i = 0; i < len; i++ ) {
    if ( str[i] != c ) continue;
    result.push_back( std::string( begin, str + i ) );
    begin = str + i + 1;
  }
  if ( begin < end ) {
    result.push_back( std::string( begin, end ) );
  }
  return result;
}

std::vector<std::string> split(const char *str, char c = ' ') {
  std::vector<std::string> result;
  if ( str == nullptr ) return result;
  do {
      const char *begin = str;
      while ( *str != c && *str ) { str++; }
      result.push_back(std::string(begin, str));
  } while (0 != *str++);
  return result;
}

std::string get_str_between_two_str(const std::string &s, const std::string &start_delim, const std::string &stop_delim) {
  unsigned first_delim_pos = s.find(start_delim);
  unsigned end_pos_of_first_delim = first_delim_pos + start_delim.length();
  unsigned last_delim_pos = s.find_first_of(stop_delim, end_pos_of_first_delim);
  return s.substr(end_pos_of_first_delim, last_delim_pos - end_pos_of_first_delim);
}

class FIX {
  public:
    FIX( const std::string& msg ) : msg_(msg), type_("-1")
    {
      pretty_ = msg;
      std::replace( pretty_.begin(), pretty_.end(), '\1', '|');
      std::replace( msg_.begin(), msg_.end(), '|', '\1');
      splited_ = split( msg_.c_str(), msg_.length() , '\1' );
      for ( const auto& token : splited_ ) {
        auto splt = split( token.c_str(), '=' );
        if ( splt.size() != 2 ) {
          std::cout << "Invalid msg: " << pretty_ << " fail to parse: " << std::endl << "'" << token << "'" << std::endl;
          continue;
        }
        fields_[ splt[0] ] = splt[1];
      }
    }

    std::string msg() const { return pretty_; }
    std::string raw() const { return msg_; }
    std::vector<std::string> splited() const { return splited_; }

    std::string tag(const std::string& id) const {
      auto it = fields_.find( id );
      if ( it == fields_.end() ) {
        return std::string();
      }
      return it->second;
    }

    std::string operator+(const std::string& rhs) noexcept {
      return msg_ + rhs;
    }

  protected:
    std::string msg_;
    std::string pretty_;
    std::string type_;
    std::vector<std::string> splited_;
    std::map<std::string, std::string> fields_;
};

class SmallApplication : public Socket {
public:
  static void notifyConnected(void *data) {
    if ( data == nullptr ) { return; }
    auto ptr = reinterpret_cast< SmallApplication* >( data );
    ptr->reconnected();
  }

  void reconnected() {
    ready_to_send_next_ = true;
    std::cout << " reconnected " << std::endl;
  }

  SmallApplication( const std::string& ip
             , int32_t port
             , const std::string& account
             , const std::string& passwd 
             , const std::string& sender_compid
             , const std::string& target_compid)
      : Socket( ip, port, notifyConnected, this )
      , account_(account)
      , passwd_(passwd)
      , sender_compid_(sender_compid)
      , target_compid_(target_compid)
  {
    connect();
  }

  void check_pending() {
    if (!ready_to_send_next_) return;

    if (msgs_.empty()) { return; }

    if (send_command(msgs_.front())) {
      msgs_.pop_front();
      return;
    }
  }

  void run_session() {
    while(true) {
      if ( !run_once( [this](char* rxbuf, size_t size) -> ssize_t { return handleFixMsg( rxbuf, size ); } ) ) {
        return;
      }
      check_pending();
    }
  }

  size_t handleFixMsg(const char *str, size_t size) {
    size_t handled = 0;
    while ( true ) {
      if ( size < 26 ) {
        std::cout << size << " size < 26 | " << std::string(str, size) << std::endl;
        return handled;
      }
      auto [ msg, len ] = extractFixMsg( str + handled, size - handled );
      if ( len <= 0 ) {
        return handled;
      }
      handled += len;
      processFixMsg( msg, len );
    }
    return handled;
  }

  std::tuple<const char*, size_t> extractFixMsg(const char *str, size_t size) {
    std::string patern = '\1' + std::string("10=");
    auto ptr = strstr( str, patern.c_str() );
    if ( ptr == NULL ) { return std::make_tuple( str, 0 ); }

    auto len = ptr - str + 8;
    if ( size < len ) { return std::make_tuple( str, 0 ); }

    return std::make_tuple( str, len );
  }

  void processFixMsg(const char *str, size_t size) {
    auto msg = std::string( str, size );
    auto fix = FIX( msg );

    auto msg_type = fix.tag("35");

    if ( msg_type == "1" ) { // Test Request
      send_command( "35=0|112=FIX.4.4" );
      return;
    }

    int64_t timestamp = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    auto sendTime = fix.tag("52");
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    strptime( sendTime.data(), "%Y%m%d-%T", &tm );
    uint64_t ts = mktime(&tm) * 1'000'000;
    auto dot = sendTime.find( '.' );
    if ( dot != std::string::npos ) {
      ts += strtoull( sendTime.substr( dot+1 ).data(), nullptr, 10 ) * 1'000;
    }
    auto dela = timestamp - ts;
    std::cout << sendTime << " Delta " << dela << std::endl;
  }

  bool send_command(const std::string& cmd) {
    if (!ready_to_send_next_) {
      msgs_.push_back(cmd);
      return false;
    }
    auto splited = split( cmd.c_str(), cmd.length() , '&' );
    for ( const auto& token : splited ) {
      auto next = genFixFromString( token );
      auto fix_msg = FIX( next );
      send( next );
    }
    return true;
  }

  std::string genFixFromString(const std::string& msg, bool use_default = true) {
    return genFixFromString( msg.c_str(), use_default );
  }

  std::string genFixFromString(const char* msg, bool use_default = true) {
    auto splited = split( msg, '|' );
    for ( const auto& token : splited ) {
      auto splt = split( token.c_str(), '=' );
      if ( splt.size() != 2 ) {
        continue;
      }
      if ( splt[0] != "35" ) {
        continue;
      }
      if ( splt[1] == "A" ) {
        return ganerateLogon( splited, use_default );
      } else if ( splt[1] == "V" ) {
        return ganerateMDRequest( splited, use_default );
      } else {
        return ganerateCommon( splited, use_default );
      }
    }
    return std::string();
  }

  std::string val( const std::string& key, std::map<std::string, std::string>& fields, bool delete_after = true ) {
    auto itr = fields.find(key);
    if ( itr == fields.end() ) {
      return std::string();
    }

    std::string res = form_tag( key, itr->second );
    if ( delete_after ) {
      fields.erase( itr );
    }
    return res;
  }

  std::tuple<std::string, std::string> get_key_and_val( const std::string& token ) {
    auto splt = split( token.c_str(), '=' );
    if ( splt.size() != 2 ) {
      return std::make_tuple( splt[0], std::string() );
    }
    return std::make_tuple( splt[0], splt[1] );
  }

  std::string form_tag(const std::string& key, const std::string& value) {
    return key + "=" + value + '\001';
  }

  std::string ganerateCommon( const std::vector<std::string>& splited, bool use_default ) {
    std::map<std::string, std::string> m;
    if ( use_default ) {
      initHeadr( m );
    }
    return formMsg( m, splited );
  }

  std::string ganerateLogon( const std::vector<std::string>& splited, bool use_default ) {
    std::map<std::string, std::string> m;
    if ( use_default ) {
      initHeadr( m );
      m["553"] = account_; // Username
      m["554"] = passwd_; // Password
    }
    return formMsg( m, splited );
  }

  std::string ganerateMDRequest( const std::vector<std::string>& splited, bool use_default ) {
    std::map<std::string, std::string> m;
    m["35"] = "V";
    if ( use_default ) {
      initHeadr( m );
    }

    auto begin_str = val("8", m);
    std::stringstream body;
    body << val("35", m) << val("49", m) << val("56", m) << val("34", m) << val("52", m);
    bool need_resp = false;
    for ( const auto& i : splited ) {
      auto [ key, value ] = get_key_and_val( i );
      if ( key == "need_response" && value == "true" ) {
        need_resp =  true;
      }
      body << i << '\001';
    }

    auto bodys = body.str();
    auto body_len = gen_bodylength( bodys );

    std::stringstream ss;
    ss << begin_str     // BeginString
       << body_len      // BodyLength
       << bodys;

    auto fix_msg = ss.str();
    return fix_msg + calc_checksumm( fix_msg );
  }

  template<typename T>
  void initHeadr( T& m ) {
    auto cur_time = currentDateTime();
    m["8"] = "FIX.4.4"; // BeginString
    m["49"] = sender_compid_; // SenderCompID
    m["56"] = target_compid_; // TargetCompID
    m["34"] = getMsgSeqNum(); // MsgSeqNum
    m["52"] = cur_time; // SendingTime
  }

  std::string formMsg( auto& m, auto& splited ) {
    for ( const auto& token : splited ) {
      if ( !token.length() ) {
        continue;
      }
      auto splt = split( token.c_str(), '=' );
      if ( splt.size() != 2 ) {
        std::cout << "Invalid msg3: fail to parse: " << token << std::endl;
        exit (1);
        return std::string();
      }
      m[ splt[0] ] = splt[1];
    }

    auto begin_str = val("8", m);
    auto body_len = val("9", m);
    auto check_summ = val("10", m);

    if ( !body_len.length() ) {
      body_len = gen_bodylength(m);
    }

    auto msg_type = val("35", m);

    std::stringstream ss;

    // ------ head -------
    ss << begin_str     // BeginString
       << body_len      // BodyLength
       << msg_type      // MsgType
       << val("49", m)  // SenderCompID
       << val("56", m)  // TargetCompID
       << val("34", m)  // MsgSeqNum
       << val("52", m); // SendingTime

    // ------ body -------
    for ( const auto& it : m ) {
      if ( it.first == "269" ) {
        ss << form_tag( "269", (it.second == "1") ? "0" : "1" );
      }
      ss << val( it.first, m, false );
    }

    // ------ tail -------
    if ( !check_summ.length() ) {
      auto fix_msg = ss.str();
      return fix_msg + calc_checksumm( fix_msg );
    }
    ss << check_summ;
    return ss.str();
  }

  std::string getMsgSeqNum() { return std::to_string( ++msg_seq_num_ ); }

  std::string gen_bodylength( const std::string& msg ) { return "9=" + std::to_string( msg.length() ) + '\001'; }

  std::string gen_bodylength( const std::map<std::string, std::string>& fields ) {
    size_t len = 0;
    for (const auto& it : fields ) {
      len += it.first.length() + it.second.length() + 2;
    }
    return "9=" + std::to_string( len ) + '\001';
  }

  std::string calc_checksumm( const std::string& msg ) {
    size_t sum = 0;
    for ( size_t i = 0; i < msg.length(); i++ ) {
      sum += msg[i];
    }
    sum %= 256;
    char r3 = static_cast<char>('0' + (sum % 10));
    sum /= 10;
    char r2 = static_cast<char>('0' + (sum % 10));
    sum /= 10;
    char r1 = static_cast<char>('0' + (sum % 10));

    return std::string("10=") + r1 + r2 + r3 + '\001';
  }

  std::string account_;
  std::string passwd_;

  std::string sender_compid_; // SenderCompID
  std::string target_compid_; // TargetCompID

  size_t msg_seq_num_ = 0;
  bool ready_to_send_next_ = false;
  std::list<std::string> msgs_;
};

int main( int argc, char** argv ) {

  if (argc != 7) {
    std::cout << "Usage: " << argv[0] << " <ip> <port> <account> <passwd> <SenderCompID> <TargetCompID>" << std::endl;
    exit(1);
  }
  try {
    std::string ip = argv[1];
    int32_t port = std::atoi(argv[2]);

    std::string account = argv[3];
    std::string passwd = argv[4];
    std::string sender_compid = argv[5];
    std::string target_compid = argv[6];

    std::vector<std::string> pairs = {"ALGBTC","ALGUST","ANCUSD","ANTETH","ANTUSD","ATOUST","AXSUST","B21X:USD","BALUSD","BAND:UST","BATUSD","BATUST","BTCMIM","BTC:XAUT","BTGUSD","CCDUSD","CHZUSD","CHZUST","COMP:UST","CRVUSD","CRVUST","CTKUSD","DAIUSD","DOGBTC","DOGE:BTC","DOGE:UST","DOGUSD","DORA:UST","DOTUST","EOSBTC","EOSUSD","EOSUST","ETCUSD","ETH2X:UST","ETHBTC","ETHUSD","ETPBTC","EUTUSD","FORTH:USD","FTMUST","FTTUSD","GTXUSD","GTXUST","HIXUSD","ICPUST","IDXUST","IOTBTC","IOTUSD","IQXUSD","JASMY:USD","JSTUST","KAIUSD","KAIUST","KANUSD","KSMUST","LEOEOS","LINK:UST","LTCUSD","LUNA:BTC","LUNA:UST","MATIC:BTC","MATIC:USD","MIMUSD","MIMUST","MIRUSD","MNABTC","NEOETH","NEOUSD","NEOUST","NEXO:BTC","NEXO:USD","OCEAN:USD","OXYUSD","PAXUSD","PAXUST","PLANETS:UST","POLC:UST","QTFUSD","REEF:USD","ROSE:USD","SENATE:UST","SGBUST","SHIB:USD","SHIB:UST","SIDUS:USD","SIDUS:UST","SOLUSD","SOLUST","SPELL:MIM","SPELL:UST","SUKU:UST","SUNUSD","SUNUST","SUSHI:UST","SXXUSD","TERRAUST:UST","TLOS:USD","TRXUSD","TSDUST","UDCUST","USTUSD","VELO:USD","VELO:UST","VETBTC","VETUST","VSYUSD","WAVES:USD","WNCG:USD","WNCG:UST","XLMUST","XMRBTC","XMRUST","XRDBTC","XRDUSD","XTZBTC","XTZUSD","XTZUST","ZECBTC","ZILBTC","ZILUSD","ZRXBTC"};

    auto session = SmallApplication( ip, port, account, passwd, sender_compid, target_compid);
    session.send_command("35=A|98=0|108=30|141=Y"); //Logon
    for (const auto& pair : pairs) {
      session.send_command("35=V|262="+pair+"|263=1|265=1|267=2|269=0|269=1|146=1|55="+pair ); //Subscribe
    }

    session.run_session();
    std::cout << "stop session" << std::endl;
    return 0;

  } catch ( std::exception & e ) {
    std::cout << e.what() << std::endl;
    return 1;
  }


}

