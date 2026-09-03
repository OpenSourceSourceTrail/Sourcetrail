#pragma once
// boost
// date_time's templates instantiate under whatever warning flags the including translation unit
// carries, and they do not pass ours -- -isystem does not help, the instantiation point is ours.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif
#include <boost/date_time/posix_time/posix_time.hpp>
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

class TimeStamp final {
public:
  static TimeStamp now();

  static double durationSeconds(const TimeStamp& start);
  static std::string secondsToString(double secs);

  TimeStamp();
  TimeStamp(boost::posix_time::ptime pTime);
  TimeStamp(const std::string& timeAsString);

  [[nodiscard]] bool isValid() const;

  [[nodiscard]] std::string toString() const;
  [[nodiscard]] std::string getDDMMYYYYString() const;
  [[nodiscard]] std::string dayOfWeek() const;
  [[nodiscard]] std::string dayOfWeekShort() const;

  bool operator==(const TimeStamp& rhs) const {
    return m_time == rhs.m_time;
  }
  bool operator!=(const TimeStamp& rhs) const {
    return m_time != rhs.m_time;
  }
  bool operator<(const TimeStamp& rhs) const {
    return m_time < rhs.m_time;
  }
  bool operator>(const TimeStamp& rhs) const {
    return m_time > rhs.m_time;
  }
  bool operator<=(const TimeStamp& rhs) const {
    return m_time <= rhs.m_time;
  }
  bool operator>=(const TimeStamp& rhs) const {
    return m_time >= rhs.m_time;
  }

  [[nodiscard]] size_t deltaMS(const TimeStamp& other) const;
  [[nodiscard]] size_t deltaS(const TimeStamp& other) const;

  [[nodiscard]] bool isSameDay(const TimeStamp& other) const;

  // days are counted beginning at 00:00, so a tp of 1.1.2017 23:59 is 1 day ago if it's
  // the 2.1.2017 00:01
  [[nodiscard]] size_t deltaDays(const TimeStamp& other) const;
  [[nodiscard]] size_t deltaHours(const TimeStamp& other) const;

private:
  boost::posix_time::ptime m_time;
};