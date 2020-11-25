

#pragma once

#ifndef __SOCKET_HELPERS_HPP__
#define __SOCKET_HELPERS_HPP__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>

struct netinfo
{
  
  std::string ip4addr;
  int port;
  struct sockaddr_in si;
  netinfo()
    : ip4addr("0.0.0.0"), port(-1)
  {
  }
  
  netinfo( const std::string& ip, const int& p, const sockaddr_in& s )
    : ip4addr(ip), port(p), si(s)
  {
  }
  
};

struct recvbuffer
{
  size_t tail;
  std::vector<uint8_t> buf;

  recvbuffer()
    : tail(0)
  {    
  }
  
  recvbuffer( const size_t& s )
    : tail( 0 ), buf( std::vector<uint8_t>( s, 0 ) )
  {
  }

  void init( const size_t& s )
  {
    tail = 0;
    buf.resize( s, 0 );
  }
  
  uint8_t* tail_as_ptr( const size_t& expected_write )
  {
    if( tail + expected_write > buf.size() )
      {
	fprintf(stderr, "REV: error, not enough in buffer to write to me\n");
	exit(1);
      }
    else
      {
	return (buf.data() + tail);
      }
  }

  uint8_t* tail_as_ptr()
  {
    return (buf.data() + tail);
  }

  void wrote( const size_t& written )
  {
    tail += written;
  }

  bool has_space( const size_t& s )
  {
    if( buf.size() - tail >= s )
      {
	return true;
      }
    return false;
  }
  
  size_t avail( )
  {
    return tail;
  }

  void reset()
  {
    tail = 0;
  }
};

void sendto_socket(  const int& sock, const std::vector<uint8_t>& data, const netinfo& ni );

int recvupto_intobuf( int sock, int size, recvbuffer& buf );
int recvupto_intobuf_tout( int sock, int size, recvbuffer& buf, const double tout_sec );

int client_connect_udp_socket( const std::string& ip, const int& port, const int& recv_buf_size_bytes=1000000 );

int server_connect_udp_socket(const int& port, int& prevsocket, const int& recv_buf_size_bytes=1000000);

void send_to_socket( const int& socket, const std::string& message );

void send_to_socket( const int& socket, const std::vector<uint8_t>& message );

std::vector<uint8_t> recvupto( int sock, int size);

std::vector<uint8_t> connect_udp_and_recvupto( const int& sock, const int& size, netinfo& ni );

#endif 
