// Static logging class, synchronized and intended to be used globally.

#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <mutex>
#include <string>

class Log {
public:
  enum Level {DISABLE = -1, ERR = 0, INFO = 1, DBG = 2};

  static void set_level(Level lvl) {
    std::lock_guard<std::mutex> lck(mtx);
    level = lvl;
  }

  static void set_out(std::ostream &s) {
    std::lock_guard<std::mutex> lck(mtx);
    out = &s;
  }

  static std::ostream& get_out() {
    std::lock_guard<std::mutex> lck(mtx);
    return *out;
  }

  Log(const std::string &name_ = "")
    : name(name_) {}

  Log(const std::string &name_, Log &parent_)
    : parent(&parent_), name(name_) {}

  void err(const std::string &msg) {
    this->print(msg, ERR);
  }

  void dbg(const std::string &msg) {
   this->print(msg, DBG);
  }

  void info(const std::string &msg) {
    this->print(msg, INFO);
  }

private:
  static std::mutex mtx;
  static std::ostream *out;
  static Level level;
  Log *parent = nullptr;
  std::string name;

  void print(const std::string &msg, Level level_) {
    std::lock_guard<std::mutex> lck(mtx);

    if (level_ > level)
      return;

    std::string head;
    if (! this->name.empty()) {
      head = this->name + ": ";
      for(Log *log = this->parent; log != nullptr; log = this->parent->parent)
        head = log->name + "->" + head;
    }
    switch (level_) {
      case ERR: head = "(ERR) " + head; break;
      case DBG: head = "(DBG) " + head; break;
      case INFO: head = "(INF) " + head; break;
      case DISABLE: break;
    }
      *out << head + msg << std::endl;
  }
};

std::mutex Log::mtx;
std::ostream* Log::out = &std::cout;
Log::Level Log::level = Log::DBG;

Log root_log;

#endif //LOG_H
