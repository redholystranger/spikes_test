#pragma once
#include <fcntl.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

typedef void (*callback_function)(void *);

class Socket {
  public: 

    Socket( const std::string& ip, int32_t port, callback_function func, void* param )
        : sock_()
        , ip_(ip)
        , port_(port)
        , connected_func_(func)
        , param_(param)
    {}

    bool connect() {
      connected_ = false;
      struct hostent* host = gethostbyname( ip_.c_str() ); 
      sockaddr_in sendSockAddr;   
      bzero( (char*)&sendSockAddr, sizeof(sendSockAddr) ); 
      sendSockAddr.sin_family = AF_INET; 
      sendSockAddr.sin_addr.s_addr = inet_addr( inet_ntoa( *(struct in_addr*)*host->h_addr_list ) );
      sendSockAddr.sin_port = htons( port_ );
      sock_ = socket( AF_INET, SOCK_STREAM, 0 );
      if ( sock_ <= 0 ) {
        std::cout << "error socket create" << std::endl;
        exit(1);
        return false;
      }

      int status = ::connect( sock_, (sockaddr*) &sendSockAddr, sizeof(sendSockAddr) );
      if ( status < 0 ) {
        std::cout << "Error connecting to socket! ip " << ip_ << ":" << port_ << " err " << strerror(errno) << std::endl;
        return false;
      }

      int flags, s;
      flags = fcntl( sock_, F_GETFL, 0 );
      if ( flags == -1 ) {
        std::cout << "error getting fcntl" << std::endl;
        return false;
      }
    
      flags |= O_NONBLOCK;
      s = fcntl( sock_, F_SETFL, flags );
      if ( s == -1 ) {
        std::cout << "error setting fcntl" << std::endl;
        return false;
      }

      std::cout << port_ << " Connected" << std::endl;
      connected_ = true;
      connected_func_( param_ );
      return true;
    }

    template<class Func>
    bool run_once( Func&& func ) {
      while ( true ) {
        if ( !read() ) break;

        while ( head > tail ) {
          ssize_t handled = func( tail,  head - tail );
          if ( handled > 0 ) {
            tail += handled;
          } else if ( handled == 0 ) {
            break;
          }
        }
    
        if ( head == tail ) {
          head = tail = buf_;
        }
      }
      return true;
    }

    bool read() {
      constexpr auto half_size = sizeof( buf_ ) >> 1;
      auto size = sizeof( buf_ ) - ( head - buf_ );

      if ( size < half_size ) {
        const auto free_space = tail - buf_;
    
        if ( free_space ) {
          const auto copySize = head - tail;
          memmove( buf_, tail, copySize );
          tail = buf_;
          head -= free_space;
          size += free_space;
        }
        if ( size <= 0 ) {
          std::cout << "Empty buffer" << std::endl;
          return false;
        }
      }

      ssize_t count = ::read( sock_, head, size );

      if ( count == -1 ) {
        static uint64_t cnt = 0;
        if ( errno != EAGAIN ) {
          std::cout << port_ << " Connection lost" << std::endl;
          connect(); 
          return false;
        }
        return false;
      } else if ( !count ) {
        std::cout << port_ << " Connection lost" << std::endl;
        connect(); 
        return false;
      }

      head += count;

      return true;
    }

    size_t send( const std::string& msg ) {
      size_t count = msg.length();
      size_t size = 0;
      auto flags = MSG_NOSIGNAL;
      const char *ptr = msg.c_str();
      while ( size < count ) {
        auto send_size = count - size;
        auto res = ::send( sock_, ptr + size, send_size, flags );

        if ( res > 0 ) {
          size += res;
          continue;
        }
        if ( !res || errno == EWOULDBLOCK || errno == EAGAIN ) {
          continue;
        }
        std::cout << "res = " << res << " errno " << errno << " " << strerror(errno) << std::endl;
        throw std::invalid_argument( "connection closed" );
      }
      return size;
    }

  private:
    bool connected_;
    int sock_;
    char buf_[ 1 << 21 ];
    char* tail = buf_;
    char* head = buf_;

    std::string ip_;
    int32_t port_;
    callback_function connected_func_;
    void* param_;
};
