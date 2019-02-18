//
// Created by oyoung on 18-12-10.
//

#ifndef DISPATCH_FORMAT_HPP
#define DISPATCH_FORMAT_HPP

#include <string>
#include <sstream>

#include <chrono>
#include <iomanip>

namespace std {
    inline std::string to_string(double value, int precision) {
        std::ostringstream stream;
        stream.precision(precision);
        stream << std::fixed << value ;
        return stream.str();
    }
}

namespace oyoung {

    struct formatter {
        formatter(const std::string& format): _format_(format) {}

        template <typename T>
        formatter& arg(T value) {

            auto holder = current_place_holder();

            if(holder >= 0) {
                replace("%" + std::to_string(holder), std::to_string(value));
            }

            return *this;
        }

        formatter& arg(double value, int precision) {
            auto holder = current_place_holder();

            if(holder >= 0) {
                replace("%" + std::to_string(holder), std::to_string(value, precision));
            }

            return *this;
        }

        formatter& arg(const std::string& value) {
            auto holder = current_place_holder();

            if(holder >= 0) {
                replace("%" + std::to_string(holder), value);
            }


            return *this;
        }

        formatter& arg(const char * value) {
            auto holder = current_place_holder();

            if(holder >= 0) {
                replace("%" + std::to_string(holder), value);
            }

            return *this;
        }

        template <typename Clock, typename Duration>
        formatter& arg(const std::chrono::time_point<Clock, Duration>& time_point, const std::string& fmt) {
            auto holder = current_place_holder();
            auto value = time_format(time_point, fmt);

            if(holder >= 0) {
                replace("%" + std::to_string(holder), value);
            }

            return *this;
        }


        formatter& replace(const std::string& src, const std::string& target) {

            auto begin = _format_.find(src);
            while (begin != _format_.npos) {
                _format_.replace(begin, src.size(), target);
                begin = _format_.find(src);
            }

            return *this;
        }

        template <typename Iterator>
        std::string join(Iterator begin, Iterator end)
        {
            auto last = end - 1;
            auto joined = std::string {};

            if(end <= begin) return joined;

            joined.reserve(256);

            for(auto it = begin; it != last; ++ it) {
                joined += *it;
                joined += _format_;
            }

            joined += *last;

            return joined;
        }

        template <typename Clock, typename Duration>
        static std::string time_format(const std::chrono::time_point<Clock, Duration>& time_point, const std::string& fmt)
        {
            auto time = Clock::to_time_t(time_point);
            auto puttime  = std::put_time(std::localtime(&time), fmt.c_str());
            auto ostream = std::ostringstream {};
            return (ostream << puttime, ostream).str();
        }

        std::string to_string() const  {
            return _format_;
        }

    private:

        int current_place_holder() const {
            int holder {-1};
            char *pos = nullptr;
            auto begin = _format_.c_str();
            auto end =  begin + _format_.size();
            while (begin < end) {
                if(begin[0] != '%') {
                    ++begin;
                    continue;
                }

                pos = nullptr;
                auto integer = std::strtol(begin + 1, &pos, 10);

                if(pos == nullptr || pos == begin) {
                    ++begin;
                    continue;
                }

                if(holder == -1 || integer < holder) {
                    holder = integer;
                }

                begin = pos;
            }
            return holder;
        }

    private:
        std::string _format_;
    };

    inline formatter format(const std::string& fmt) { return  formatter(fmt); }
}

#endif //DISPATCH_FORMAT_HPP
