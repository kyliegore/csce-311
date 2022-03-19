#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>



class UnixDomainSocket {
 public:
  explicit UnixDomainSocket(const char *socket_path) {
    socket_path_ = std::string(socket_path);  // std::string manages char *

    sock_addr_ = {};  // init struct (replaces memset)
    sock_addr_.sun_family = AF_UNIX;  // set to Unix domain socket (e.g. instead
                                      //   of internet domain socket)
    // leaving leading null char sets abstract socket
    strncpy(sock_addr_.sun_path + 1,  // use strncpy to limit copy for
            socket_path,              //   portability
            sizeof(sock_addr_.sun_path) - 2);  // -2 for leading/trailing \0s
  }

 protected:
  ::sockaddr_un sock_addr_;  // socket address from sys/un.h

  std::string socket_path_;  // let std::string manage char *
};

class DomainSocketServer : public UnixDomainSocket {
 public:
  using ::UnixDomainSocket::UnixDomainSocket;

  void RunServer() const {
    int sock_fd;  // unnamed socket file descriptor
    int client_req_sock_fd;  // client connect request socket file descriptor

    // (1) create a socket
    //       AF_UNIX -> file system pathnames
    //       SOCK_STREAM -> sequenced bytestream
    //       0 -> default protocol (let OS decide correct protocol)
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ( sock_fd < 0 ) {
      std::cerr << strerror(errno) << std::endl;
      exit(-1);
    }

    // (2) bind socket to address for the server
    unlink(socket_path_.c_str());  // sys call to delete file if it exists
                                   // already using Unix system calls for
                                   //   sockets, no reason to be non-Unix
                                   //   portable now.  :-/
    int success = bind(sock_fd,
                       // sockaddr_un is a Unix sockaddr and so may be cast "up"
                       //   to that pointer type (think of it as C polymorphism)
                       reinterpret_cast<const sockaddr*>(&sock_addr_),
                       // size needs be known due to underlying data layout,
                       //   i.e., there may be a size difference between parent
                       //   and child
                       sizeof(sock_addr_));
    if (success < 0) {
      std::cerr << strerror(errno) << std::endl;
      exit(-1);
    }

    // (3) Listen for connections from clients
    std::cout << "SERVER STARTED\t MAX CLIENTS: 7" << std::endl;
    

    size_t kMax_client_conns = 5;
    success = listen(sock_fd, kMax_client_conns);
    if (success < 0) {
      std::cerr << strerror(errno) << std::endl;
      exit(-1);
    }

    const size_t kRead_buffer_size = 32;
    char read_buffer[kRead_buffer_size];
    int bytes_read;
    while (true) {
      // (4) Accept connection from a client
      client_req_sock_fd = accept(sock_fd, nullptr, nullptr);
      if (client_req_sock_fd  < 0) {
        std::cerr << strerror(errno) << std::endl;
        continue;
      }

      std::cout << "CLIENT CONNECTED" << std::endl;

      // (5) Receive data from client(s)
      bytes_read = read(client_req_sock_fd, read_buffer, kRead_buffer_size);
      const char kKill_msg[] = "quit";  // TODO(lewisjs): trim whitespace
                                        //   from read_buffer for comparison
      while (bytes_read > 0) {
        if (strcmp(read_buffer, kKill_msg) == 0) {
          std::cout << "Server shutting down..." << std::endl;

          bytes_read = 0;  // message handled, disconnect client
          break;
        }

        std::cout << "read " << bytes_read << " bytes: ";
        std::cout.write(read_buffer, bytes_read) << std::endl;

        bytes_read = read(client_req_sock_fd, read_buffer, kRead_buffer_size);

        std::string myfile = "";
        // myfile = ;
        // std::ifstream (myfile);
        // myfile.open ("example.txt");
        // myfile << "Writing this to a file.\n";
        // myfile.close();

        if(bytes_read <= 1) { 
            std::cout << "INVALID FILE" << std::endl;
        } else {
            std::cout << "BYTES SENT: " << bytes_read << std::endl;
        }

      }

      if (bytes_read == 0) {
        std::cout << "Client disconnected" << std::endl;

        close(client_req_sock_fd);
      } else if (bytes_read < 0) {
        std::cerr << strerror(errno) << std::endl;

        exit(-1);
      }
    }
  }
};


const char kSocket_path[] = "socket_cli_srv_domain_socket";
int main(int argc, char *argv[]) {
  if (argc != 2)
    return 1;


  if (strcmp(argv[1], "server") == 0) {
    DomainSocketServer dss(kSocket_path);
    dss.RunServer();
  }

  return 0;
}
