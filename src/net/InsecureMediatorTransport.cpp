#include "net/InsecureMediatorTransport.hpp"

#include "battle/ProtocolCodec.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <string>

namespace tp {

InsecureMediatorTransport::InsecureMediatorTransport(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port) {}

InsecureMediatorTransport::~InsecureMediatorTransport() {
  close();
}

bool InsecureMediatorTransport::connect() {
  close();
  hello_done_ = false;
  read_buf_.clear();
  inbox_.clear();

  addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* res = nullptr;
  const std::string port_str = std::to_string(port_);
  if (getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
    return false;
  }

  int fd = -1;
  for (addrinfo* p = res; p; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd < 0) {
      continue;
    }
    const int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }
    ::close(fd);
    fd = -1;
  }
  freeaddrinfo(res);
  if (fd < 0) {
    return false;
  }

  const int flags = fcntl(fd, F_GETFL, 0);
  if (flags >= 0) {
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }

  fd_ = fd;
  alive_ = true;

  WireHello hello;
  hello.client_ver = 1;
  hello.proto_ver = 1;
  hello.build_id = 1;
  if (!write_all(encode_hello(hello))) {
    close();
    return false;
  }

  for (int i = 0; i < 50 && alive_ && !hello_done_; ++i) {
    pump_reads();
    if (!hello_done_) {
      timespec ts {.tv_sec = 0, .tv_nsec = 20'000'000};
      nanosleep(&ts, nullptr);
    }
  }
  if (!hello_done_) {
    close();
    return false;
  }
  return true;
}

void InsecureMediatorTransport::close() {
  alive_ = false;
  hello_done_ = false;
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

void InsecureMediatorTransport::send(const OutboundEvent& ev) {
  if (!alive_ || fd_ < 0) {
    return;
  }
  CodecError err = CodecError::Ok;
  auto bytes = encode_outbound(ev, &err);
  if (err != CodecError::Ok || bytes.empty()) {
    return;
  }
  if (!write_all(bytes)) {
    close();
  }
}

void InsecureMediatorTransport::poll(std::vector<InboundEvent>& out) {
  if (alive_) {
    pump_reads();
  }
  out.insert(out.end(), inbox_.begin(), inbox_.end());
  inbox_.clear();
}

bool InsecureMediatorTransport::alive() const {
  return alive_;
}

bool InsecureMediatorTransport::write_all(const std::vector<std::uint8_t>& bytes) {
  std::size_t off = 0;
  while (off < bytes.size()) {
    const ssize_t n = ::send(fd_, bytes.data() + off, bytes.size() - off, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        timespec ts {.tv_sec = 0, .tv_nsec = 5'000'000};
        nanosleep(&ts, nullptr);
        continue;
      }
      return false;
    }
    if (n == 0) {
      return false;
    }
    off += static_cast<std::size_t>(n);
  }
  return true;
}

void InsecureMediatorTransport::pump_reads() {
  if (fd_ < 0) {
    return;
  }
  std::uint8_t tmp[2048];
  for (;;) {
    const ssize_t n = ::recv(fd_, tmp, sizeof(tmp), 0);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      close();
      return;
    }
    if (n == 0) {
      close();
      return;
    }
    read_buf_.insert(read_buf_.end(), tmp, tmp + n);
  }

  for (;;) {
    CodecError err = CodecError::Ok;
    auto frame = try_decode_frame(read_buf_, &err);
    if (!frame) {
      if (err == CodecError::TooLarge || err == CodecError::NonzeroFlags ||
          err == CodecError::BadPayload) {
        close();
      }
      break;
    }
    if (frame->msg_type == kMsgHelloAck) {
      if (decode_hello_ack(*frame)) {
        hello_done_ = true;
      } else {
        close();
        return;
      }
      continue;
    }
    auto inbound = decode_inbound(*frame, &err);
    if (!inbound) {
      if (err == CodecError::UnknownType || err == CodecError::BadPayload) {
        close();
        return;
      }
      continue;
    }
    inbox_.push_back(std::move(*inbound));
  }
}

}  // namespace tp
