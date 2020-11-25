

#include <socket_helpers.hpp>


int recvupto_intobuf_tout( int sock, int size, recvbuffer& buf, const double tout_sec )
{
  if( true == buf.has_space( size ) )
    {
      fd_set set;
      struct timeval timeout;
      FD_ZERO(&set);  
      FD_SET(sock, &set);  
      timeout.tv_sec = 0; 
      timeout.tv_usec = tout_sec * 1e6; 
      
      int rv = select(sock+1, &set, NULL, NULL, &timeout); 
      
      
      
      if (rv < 0 ) 
	{
	  
	  fprintf(stderr, "Socket error\n");
	  exit(1);
	}
      else if (rv == 0)
	{
	  
	  return 0;
	  
	}
      else
	{
	  if( 0 >= FD_ISSET( sock, &set ) )
	    {
	      fprintf(stderr, "REV: socket requested [%d] was not the one recieved from (wtf?)\n", sock);
	      exit(1);
	    }
	  
	  
	  int recvsize = recv(sock, buf.tail_as_ptr(), size, MSG_WAITALL);

	   
	  if ( recvsize < 0 ) 
	    {
	      fprintf(stderr, "Error in recvupto_intobuf [%d]\n", recvsize);
	      fprintf(stderr, "Error is [%s]\n", strerror(recvsize) );
	      fprintf(stderr, "Read failed\n");
	      return 0;
	      
	    }
	  else if (recvsize == 0)
	    {
	      fprintf(stderr, "Peer disconnected\n");
	      return 0;
	      
	    }
	  else if( recvsize > 0 )
	    {
	      buf.wrote( recvsize );
	      return recvsize;
	      
	    }
	  else
	    {
	      fprintf(stderr, "This should never happen, recvall...\n");
	      exit(1);
	    }
	}
    }
  fprintf(stderr, "REV: error, not enough space in recvbuf to recieve data \n");
  return -1;
}

int recvupto_intobuf( int sock, int size, recvbuffer& buf )
{
  if( true == buf.has_space( size ) )
    {
      int recvsize = recv(sock, buf.tail_as_ptr(), size, MSG_WAITALL);
      if( recvsize > 0 )
	{
	  buf.wrote( recvsize );
	  return recvsize;
	}
      else if( recvsize < 0 )
	{
	  fprintf(stderr, "Error in recvupto_intobuf [%d]\n", recvsize);
	  fprintf(stderr, "Error is [%s]\n", strerror(recvsize) );
	  return -1;
	}
      else
	{
	  
	  fprintf(stderr, "REV: recv: other side disconnected?\n");
	  return 0;
	}
    }
  fprintf(stderr, "REV: error, not enough space in recvbuf to recieve data \n");
  exit(1);
  return 0;
}

int client_connect_udp_socket( const std::string& ip, const int& port, const int& recv_buf_size_bytes )
{
  int mysocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (mysocket < 0)
    {
      fprintf(stderr, "ERROR: CONNECT_TO_EYETRACKER_SOCKET: Could not create socket\n");
      exit(1);
    }

  
#define SETBUFSIZE 
#ifdef SETBUFSIZE
  fprintf(stdout, "Setting UDP port buffer size to [%d]\n", recv_buf_size_bytes);
  if( -1 == setsockopt(mysocket, SOL_SOCKET, SO_RCVBUF, &recv_buf_size_bytes, sizeof(recv_buf_size_bytes)) )
    {
      fprintf(stderr, "REV: error, setsocketopt failed (setting to [%d] bytes)\n", recv_buf_size_bytes);
      
    }
#endif
  
  int recvbufsize=-1;
  uint32_t size=sizeof(recvbufsize);
  int got = getsockopt( mysocket, SOL_SOCKET, SO_RCVBUF, &recvbufsize, &size );
  fprintf(stdout, "Got socket recvbuf size [%d]\n", recvbufsize);
  
#ifdef NONBLOCKING
  if( fcntl( mysocket, F_SETFL, fcntl( mysocket, F_GETFL ) | O_NONBLOCK ) < 0 )
    {
      fprintf(stderr, "REV: failed to put socket in non-blocking mode\n");
      exit(1);
    }
#endif
  
  if( fcntl( mysocket, F_GETFL ) & O_NONBLOCK ) { fprintf(stdout, "Socket is in non-blocking mode\n"); }

  struct sockaddr_in server;
  int ret = inet_aton( ip.c_str(), &server.sin_addr );
  if(ret < 0 )
    {
      fprintf(stderr, "REV: invalid IP\n");
      exit(1);
    }
    
  server.sin_family = AF_INET;
  server.sin_port = htons( port );
  
  int conn_error = connect(mysocket, (struct sockaddr *)&server , sizeof(server));
  if( conn_error < 0 )
    {
      fprintf(stderr, "CLIENT: ERROR: Connect error [%d]\n", conn_error);
      fprintf(stderr, "[%s]\n", strerror(errno) );
      exit(1);
    }
  
  return mysocket;
}

int server_connect_udp_socket(const int& port, int& mysocket, const int& recv_buf_size_bytes)
{
  if( mysocket < 0 )
    {
      mysocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (mysocket < 0)
	{
	  fprintf(stderr, "ERROR: CONNECT_TO_EYETRACKER_SOCKET: Could not create socket\n");
	  exit(1);
	}

      struct sockaddr_in serv_addr;

      bzero((char *) &serv_addr, sizeof(serv_addr));
  
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      serv_addr.sin_port = htons(port); 
      
      int ret = bind(mysocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
      if( ret < 0 )
	{
	  fprintf(stderr, "REV: bind failed [%s]\n", strerror(errno));
	  exit(1);
	}
    }
  else
    {
    }
  
  fprintf(stdout, "Will wait for UDP connections...\n");
  
  netinfo ni;
  connect_udp_and_recvupto( mysocket, 4096, ni );
  
  return mysocket;
}
  



void send_to_socket( const int& socket, const std::string& message )
{
  int ret = send( socket, message.c_str(), strlen(message.c_str()), 0 );
  if( ret < 0 )
    {
      fprintf(stderr, "REV: error in send [%d]\n", ret);
      exit(1);
    }
  else if( ret != strlen(message.c_str()) )
    {
      fprintf(stderr, "REV: incorrect number of characters sent (sent %ld expected %ld)\n", ret, strlen(message.c_str()) );
      exit(1);
    }
}

void send_to_socket( const int& socket, const std::vector<uint8_t>& message )
{
  int ret = send( socket, message.data(), message.size(), 0 );
  if( ret < 0 )
    {
      fprintf(stderr, "REV: error in send [%d]\n", ret);
      exit(1);
    }
  else if( ret != message.size() )
    {
      fprintf(stderr, "REV: incorrect number of characters sent (sent %ld expected %ld)\n", ret, message.size() );
      exit(1);
    }
}


std::vector<uint8_t> connect_udp_and_recvupto( const int& sock, const int& size, netinfo& ni )
{
  struct sockaddr_in si_other;
  uint32_t slen;
  int32_t recv_len;
  std::vector<uint8_t> buf( size );
  
  
  if ((recv_len = recvfrom(sock, buf.data(), buf.size(), 0, (struct sockaddr *) &si_other, &slen)) == -1)
    {
      fprintf(stderr, "REV: failed to receive in connect_udp_and_recvupto\n");
    }
  
  
  printf("Received packet from %s:%d (making connection) [%s]\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), (char*)buf.data());

  ni = netinfo( std::string(inet_ntoa(si_other.sin_addr)), ntohs(si_other.sin_port), si_other );
  
  return buf;
}

std::vector<uint8_t> recvupto( int sock, int size)
{
  std::vector<uint8_t> result(size, 0);
  int recvsize = recv(sock, result.data(), size, MSG_WAITALL);

  if(recvsize < 0 )
    {
      fprintf(stderr, "REV: error in socket!\n");
      fprintf(stderr, "[%s]\n", strerror(errno) );
      exit(1);
      result.resize( 0 );
    }
  else
    {
      result.resize( recvsize );
    }
  return result;
}



void sendto_socket(  const int& sock, const std::vector<uint8_t>& data, const netinfo& ni )
{
  int ret = sendto( sock, data.data(), data.size(), 0, (struct sockaddr*)&ni.si, sizeof(ni.si) );
  if( ret < 0 )
    {
      fprintf(stderr, "REV: err sending\n");
      exit(1);
    }
}
