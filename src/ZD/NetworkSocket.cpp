#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "NetworkSocket.hpp"

#define MAX_BUFFER_SIZE 4096

NetworkSocket::NetworkSocket(int socket_fd, SocketType type)
: type { type }
, socket_fd { socket_fd }
{
}

NetworkSocket::~NetworkSocket()
{
  other_sockets.clear();

  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);
}

std::shared_ptr<NetworkSocket> NetworkSocket::server(SocketType type, int port)
{
  int t = type == SocketType::TCP ? SOCK_STREAM : SOCK_DGRAM;
  int fd = socket(AF_INET, t, 0);
  if (fd < 0)
  {
    perror("server socket");
    fprintf(stderr, "Cannot create socket (fd=%d type=%d)\n", fd, t);
    return {};
  }

  int opt = 1;
  if (setsockopt(
        fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
    perror("server setsockopt");
  }

  auto ns = std::shared_ptr<NetworkSocket>(new NetworkSocket(fd, type));
  ns->port = port;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("server bind");
    fprintf(stderr, "Socket bind error (fd=%d type=%d port=%d)\n", fd, t, port);
    return {};
  }

  if (SocketType::TCP == type)
  {
    if (listen(fd, SOMAXCONN) < 0)
    {
      perror("server listen");
    }
  }

  return ns;
}

std::shared_ptr<NetworkSocket> NetworkSocket::client(
  SocketType type, std::string_view ip, int port)
{
  int t = type == SocketType::TCP ? SOCK_STREAM : SOCK_DGRAM;
  int fd = socket(AF_INET, t, 0);
  if (fd < 0)
  {
    perror("client socket");
    fprintf(stderr, "Cannot create socket (fd=%d type=%d)\n", fd, t);
    return {};
  }

  if (ip == "")
  {
    ip = "127.0.0.1";
  }

  auto ns = std::shared_ptr<NetworkSocket>(new NetworkSocket(fd, type));
  ns->ip = ip;
  ns->port = port;

  if (type == SocketType::TCP)
  {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.data(), &addr.sin_addr) <= 0)
    {
      fprintf(stderr, "Unknown address \"%s\"!\n", ip.data());
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("client socket connect");
      fprintf(
        stderr,
        "Cannot connect to %s at port %d (fd=%d)\n",
        ip.data(),
        port,
        fd);
      return {};
    }
  }

  return ns;
}

int NetworkSocket::send(std::vector<uint8_t> data)
{
  assert(socket_fd != -1);

  //printf("Sending to %s:%d\n", ip.data(), port);

  int ret = -1;

  switch (type)
  {
    case SocketType::TCP:
      ret = ::send(socket_fd, data.data(), data.size(), 0);
      break;
    case SocketType::UDP:
    {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = INADDR_ANY;
      if (!ip.empty())
      {
        if (inet_pton(AF_INET, ip.data(), &addr.sin_addr) <= 0)
        {
          fprintf(stderr, "Unknown send address \"%s\"!\n", ip.data());
        }
      }
      ret = ::sendto(
        socket_fd,
        data.data(),
        data.size(),
        0,
        (struct sockaddr *)&addr,
        sizeof(struct sockaddr));
    }
    break;
    default:
      puts("Invalid socket type");
      assert(false);
      break;
  }

  return ret;
}

SocketData NetworkSocket::read()
{
  std::vector<uint8_t> data;
  data.resize(MAX_BUFFER_SIZE);
  int ret = -1;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  std::shared_ptr<NetworkSocket> other_socket;

  switch (type)
  {
    case SocketType::TCP:
    {
      // existing connections
      for (const auto &other : other_sockets)
      {
        ret = ::read(other->get_fd(), data.data(), data.size());
        if (ret > 0)
        {
          other_socket = other;
          break;
        }
      }

      if (ret <= 0)
      {
        int read_fd = socket_fd;

        if (is_server())
        {
          int client_fd = ::accept(socket_fd, (struct sockaddr *)&addr, &len);
          if (client_fd > 0)
          {
            // new connection
            auto new_other = std::shared_ptr<NetworkSocket>(
              new NetworkSocket(client_fd, SocketType::TCP));
            other_sockets.emplace_back(new_other);
            other_socket = new_other;
          }
          read_fd = client_fd;
        }
        else
        {
          other_socket = shared_from_this();
        }

        ret = ::read(read_fd, data.data(), data.size());
      }
    }
    break;
    case SocketType::UDP:
    {
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = INADDR_ANY;

      ret = ::recvfrom(
        socket_fd, data.data(), data.size(), 0, (struct sockaddr *)&addr, &len);
      if (ret > 0)
      {
        char oip_cstr[INET6_ADDRSTRLEN];
        inet_ntop(addr.sin_family, &addr.sin_addr, oip_cstr, sizeof(oip_cstr));
        std::string other_ip(oip_cstr);
        int other_port = ntohs(addr.sin_port);
        if (auto other_s = find_socket(other_ip, other_port))
        {
          other_socket = other_s.value();
        }
        else
        {
          auto new_other_socket = std::shared_ptr<NetworkSocket>(
            new NetworkSocket(socket_fd, SocketType::UDP));
          new_other_socket->ip = other_ip;
          new_other_socket->port = other_port;
          other_sockets.push_back(new_other_socket);
          other_socket = new_other_socket;
        }
      }
    }
    break;
    default:
      puts("Invalid socket type");
      assert(false);
      break;
  }
  if (ret < 0)
    ret = 0;

  data.resize(ret);

  return { other_socket, data };
}

std::optional<std::shared_ptr<NetworkSocket>> NetworkSocket::find_socket(
  std::string_view ip, int port)
{
  std::shared_ptr<NetworkSocket> found_s;

  for (const auto &other_s : other_sockets)
  {
    if (other_s->get_ip() == ip && other_s->get_port() == port)
    {
      found_s = other_s;
      break;
    }
  }

  if (found_s == nullptr)
    return {};
  return found_s;
}
