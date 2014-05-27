// Support for remote workers.
#ifndef NETWORKER_H
#define NETWORKER_H

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <cstdint>
#include <exception>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unistd.h>

#include "Scheduler.h"
#include "Log.h"

using namespace boost::asio::ip;

// T should be *a primitive*.
template <typename T>
std::string serialize(const T &data) {
  std::stringstream ss;
  ss.write((char *) &data, sizeof(T));
  return ss.str();
}

template <typename T>
void serialize(const T &data, std::ostream &os) {
  std::string ret;
  os.write((char *) &data, sizeof(data));
}

template <typename T>
T deserialize(const std::string &data) {
  T ret;
  std::stringstream ss;
  ss.str(data);
  ss.read((char *) &ret, sizeof(T));
  return ret;
}

template <typename T>
void deserialize(T &data, std::istream &is) {
  is.read((char *) &data, sizeof(T));
}

template <typename T>
T deserialize(std::istream &is) {
  T ret;
  deserialize(ret, is);
  return ret;
}

// Serializes the length together with the actual data, useful for safety.
// The template parameter must implement operators << and >>.
template <typename T>
std::string safe_serialize(const T &data) {
  std::stringstream ss;
  ss << data;
  return serialize(ss.str().size()) + ss.str();
}

template <typename T>
void safe_serialize(const T &data, std::ostream &os) {
  std::stringstream ss;
  ss << data;
  serialize(ss.str().size(), os);
  os << ss.str();
}

template <typename T>
T safe_deserialize(const std::string &data) {
  std::stringstream ss;
  T ret;
  auto size = deserialize<size_t>(data);
  ss.str(data.substr(size + 1, data.size()));
  ss >> std::noskipws >> ret;
  return ret;
}

template <typename T>
void safe_deserialize(T &data, std::istream &is) {
  auto size = deserialize<size_t>(is);
  if (! is.good())  // size must be non-garbage because of buf.resize(size)
    return;

  std::string buf;
  buf.resize(size);
  is.read(&buf[0], size);

  std::stringstream ss;
  ss.str(buf);
  ss >> std::noskipws >> data;
}

template <typename T>
T safe_deserialize(std::iostream &is) {
  T ret;
  safe_deserialize(ret, is);
  return ret;
}

// Used to transmit 2 variables and their operation.
template <typename T>
struct NetWorkerMsg {
  enum OP {SUM, PROD} op;
  T left;
  T right;

  std::string to_string() {
    return std::string() + (op == SUM ? "S " : "P ") + std::to_string(left)
      + " " + std::to_string(right);
  }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const NetWorkerMsg<T> &obj) {
  serialize(obj.op, os);
  safe_serialize(obj.left, os);
  safe_serialize(obj.right, os);

  return os;
}

template <typename T>
std::istream& operator>>(std::istream& is, NetWorkerMsg<T> &obj) {
  deserialize(obj.op, is);
  safe_deserialize(obj.left, is);
  safe_deserialize(obj.right, is);

  return is;
}

// Local stub, real execution should take place in NetWorkerRemote.
template <typename T>
class NetWorker : public Worker<T> {
public:
  NetWorker(Scheduler<T> &scheduler, const std::string &name_,
              std::shared_ptr<tcp::iostream> socket)
    : Worker<T>(scheduler, name_), ssock(socket) {}

  virtual ~NetWorker() {
    this->ssock->close();
  }

private:
  std::shared_ptr<tcp::iostream> ssock;

  virtual T do_sum(const T &left, const T &right) {
    return this->get_result(NetWorkerMsg<T>::SUM, left, right);
  }

  virtual T do_prod(const T &left, const T &right) {
    return this->get_result(NetWorkerMsg<T>::PROD, left, right);
  }

  T get_result(const typename NetWorkerMsg<T>::OP &op, const T &left, const T &right) {
    auto check_conn = [this] () { if (! *this->ssock) {
                                      auto err = this->ssock->error();
                                      if (err == boost::asio::error::eof) {
                                        this->log.info("Connection terminated, exiting.");
                                        this->sched.unregister_worker(this);
                                        delete this;
                                      }
                                      throw std::runtime_error(err.message());} };
    // Send request
    NetWorkerMsg<T> msg;
    msg.op = op;
    msg.left = left;
    msg.right = right;

    this->log.dbg("Sending request");
    *this->ssock << msg;
    check_conn();

    // Get reply;
    T reply;

    this->log.dbg("Waiting for reply");
    safe_deserialize(reply, *this->ssock);
    check_conn();

    return reply;
  }
};

// Listens for new worker connections and creates an Worker for the given
// scheduler.
template <typename T>
class NetWorkerListener {
public:
  NetWorkerListener(Scheduler<T> &scheduler, uint32_t port)
    : sched(scheduler), log("NetExecListener"),
      listen_sock(this->io_service, tcp::endpoint(tcp::v4(), port)) {
      this->thrd = std::thread([&] (NetWorkerListener *exec) { exec->run(); }, this);
  }

  virtual ~NetWorkerListener() {
    log.info("Stopping");
    this->io_service.stop();
    this->thrd.join();
    log.dbg("Stopped");
  }

private:
  Scheduler<T> &sched;  // Scheduler that receives the Workers.
  Log log;

  std::thread thrd;  // The thread that runs this->run().
  std::mutex mtx;

  // Boost socket.
  boost::asio::io_service io_service;
  tcp::acceptor listen_sock;

  void run() {
    log.info("Listening for new connections");
    this->accept();
    this->io_service.run();
  }

  // New connection handler, Boost ASIO flavor.
  void accept_handle(std::shared_ptr<tcp::iostream> ssock) {
    std::string worker_name = ssock->rdbuf()->remote_endpoint().address().to_string();
    worker_name += ":" + std::to_string(ssock->rdbuf()->remote_endpoint().port());
    log.info("New connection from " + worker_name);

    // Create the worker, it will register itself with the scheduler.
    worker_name = "NetWorker_" + worker_name;
    new NetWorker<T>(this->sched, worker_name, ssock);

    this->accept();
  }

  // Sets up a non-blocking listen socket.
  void accept() {
    std::shared_ptr<tcp::iostream> ssock(new tcp::iostream());
    this->listen_sock.async_accept(
      *ssock->rdbuf(), bind(&NetWorkerListener::accept_handle, this, ssock));
  }

};

// Counterpart to NetWorker that actually performs the tasks.
template <typename T>
class NetWorkerRemote {
public:
  NetWorkerRemote(const std::string &host, const std::string &port)
    : log("NetWorkerRemote") {
    this->connect(host, port);
    this->run();
  }

  NetWorkerRemote(const std::string &addr)
    : log("NetWorkerRemote") {
    auto tok = addr.find(":");
    auto host = addr.substr(0, tok);
    auto port = addr.substr(++tok, addr.size());
    this->connect(host, port);
    this->run();
  }

private:
  tcp::iostream ssock;  // Streaming socket.
  Log log;

  void connect(const std::string &host, const std::string &port) {
    log.info("Connecting to " + host + ":" + port);
    this->ssock.connect(host, port);
    if (this->ssock)
      log.info("Connected");
    else
      throw std::runtime_error(this->ssock.error().message());
  }

  // Checks if the out stream is valid. Returns false for the conditions that should
  // result in graceful termination (i.e. socket was closed), throws on the others.
  bool check_valid() {
    if (! *this->out){
      auto err = this->out->error();
      if (err == boost::asio::error::eof)
        return false;
      throw std::runtime_error(err.message());
    }
    return true;
  }

  void run() {
    try {
      this->loop();
    } catch (std::exception &e) {
      log.err(std::string("Terminating on exception: ") + e.what());
    } catch(...) {
      log.err("Terminating on unrecognized exception");
    }
  }

  void loop() {
    while(true) {
      auto check_conn = [this] () { if (! this->ssock) {
                                      auto err = this->ssock.error();
                                      if (err == boost::asio::error::eof) {
                                        this->log.info("Connection terminated, exiting");
                                        exit(0);
                                      }
                                      throw std::runtime_error(err.message());} };

      // Get a request.
      log.dbg("Waiting for request");
      auto msg = NetWorkerMsg<T>();
      this->ssock >> msg;
      check_conn();

      // Process it.
      log.dbg("Got request, processing");
      T reply;
      switch (msg.op) {
        case NetWorkerMsg<T>::SUM: reply = msg.left + msg.right; break;
        case NetWorkerMsg<T>::PROD: reply = msg.left * msg.right; break;
      }

      // Send back the reply;
      log.dbg("Sending reply");
      this->ssock << safe_serialize(reply);
      check_conn();
    }
  }
};

// The NetWorkerRemote is single-threaded, this forks and connects after 0.5s.
// This is only a convenience for testing, doesn't close the proper sockets, etc.
template <typename T>
pid_t fork_remote_worker(const std::string &addr) {
  pid_t pid = fork();
  if (pid == 0) {
    usleep(500000);
    new NetWorkerRemote<T>(addr);
    exit(0);
  }
  return pid;
}

#endif  //NETWORKER_H
