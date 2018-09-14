/**
* any.hpp
*/

#ifndef OYOUNG_ANY_HPP
#define OYOUNG_ANY_HPP

#include <map>
#include <string>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <cstddef>
#include <initializer_list>

namespace oyoung
{
    struct holder
    {

        virtual std::shared_ptr<holder> clone() const = 0;
        virtual std::string type_name() const = 0;
        virtual std::size_t value_size() const = 0;
        virtual ~holder() {}
    };

    template<typename T>
    struct place_holder: public holder
    {
        place_holder(const T& v): value(v) {}

        std::string type_name() const override
        {
            return typeid(T).name();
        }

        std::shared_ptr<holder> clone() const override
        {
            return std::make_shared<place_holder>(value);
        }

        std::size_t value_size() const override
        {
            return sizeof(value);
        }

        T value;
    };

    struct any
    {
        using array_t = std::vector<any>;
        using object_t = std::map<std::string, any>;

        any() {}


        template<typename T>
        any(const T& value)
            : _holder(std::make_shared<place_holder<T>>(value)) {}
        
        any(const char *str): _holder(std::make_shared<place_holder<std::string>>(str)) {}
        
        any(const any& other): _holder(other._holder) {}

        any(any&& other)
            : _holder(other._holder) { other._holder.reset(); }

        any(const std::initializer_list<any>& list)
            : _holder (std::make_shared<place_holder<std::vector<any>>>(list))
        {

        }
        
        any& operator=(const any& other) 
        {
            _holder = other._holder;
            return *this;
        }
        
        template<typename T>
        any& operator=(const T& val)
        {
            if(_holder == nullptr 
                || _holder->type_name() != typeid(T).name()
                || _holder.use_count() > 1) {
                _holder = std::make_shared<place_holder<T>>(val);
            } else {
            std::dynamic_pointer_cast<place_holder<T>>(_holder)->value = val;
            }
            return *this;
        }

        std::vector<std::string> all_keys() const noexcept
        {
            if(is_object()) {
                auto& value = std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value;
                std::vector<std::string> keys;
                keys.reserve(value.size());
                for(const auto& pair: value)
                {
                    keys.emplace_back(pair.first);
                }
                return keys;
            }
            return {};
        }

        any& operator=(const char *str) 
        {        
            return *this = std::string(str);
        }
        

        int count() const
        {
            if(is_object()) {
                return std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value.size();
            }
            if(is_array()) {
                return std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value.size();
            }

            return 1;
        }


        
        template<typename T>
        T value(T&& def) const noexcept
        {
            if(_holder && typeid(T).name() == _holder->type_name()) {
                return std::dynamic_pointer_cast<place_holder<T>>(_holder)->value;
            } else {
                return def;
            }
        }

        void push_back(const any& value)
        {
            if(_holder == nullptr || _holder->type_name() != typeid(std::vector<any>).name()) {
                std::vector<any> v{value};
                _holder = std::make_shared<place_holder<std::vector<any>>>(v);
            } else {
                std::dynamic_pointer_cast<place_holder<std::vector<any>>>(_holder)->value.push_back(value);
            }

        }

        const any& operator[](std::size_t index) const
        {
            if(_holder == nullptr || _holder->type_name() != typeid(array_t).name()) {
                return any_null();
            } else {
                return (std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value)[index];
            }
        }

        any& operator [](std::size_t index)
        {
            if(_holder == nullptr || _holder->type_name() != typeid(array_t).name()) {
                std::vector<any> v;
                _holder = std::make_shared<place_holder<array_t>>(v);
            }
            return (std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value)[index];
        }

        template<typename T>
        void insert(const std::string& key, const T& value)
        {
            insert(key, any(value));
        }

        template<typename T>
        void insert(const char *key, const T& value)
        {
            insert(key, any(value));
        }

        void insert(const char * key, const any& value)
        {
            insert(std::string(key), value);
        }

        void insert(const std::string& key, const any& value)
        {
            if(_holder == nullptr || _holder->type_name() != typeid(object_t).name()) {
                object_t m { {key, value} };
                _holder = std::make_shared<place_holder<object_t>>(m);
            } else {
                (std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value)
                        .insert(std::make_pair(key, value));
            }
        }

        const any& operator [](const std::string& key) const
        {
            if(_holder == nullptr || _holder->type_name() != typeid(object_t).name()) {
                return any_null();
            } else {
                return (std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value)[key];
            }
        }

        const any& operator [](const char * key) const
        {
            return (*this)[std::string(key)];
        }

        any& operator [](const std::string& key)
        {
            if(_holder == nullptr || _holder->type_name() != typeid(std::map<std::string, any>).name()) {
                std::map<std::string, any> m { {key, any{}} };
                _holder = std::make_shared<place_holder<std::map<std::string, any>>>(m);
            }
            return (std::dynamic_pointer_cast<place_holder<std::map<std::string, any>>>(_holder)->value)[key];
        }

        any& operator [](const char * key)
        {
            return (*this)[std::string(key)];
        }

        std::string value(const char* def) const noexcept
        {
            return is_string() ? std::dynamic_pointer_cast<place_holder<std::string> >(_holder)->value : def;
        }


        bool is_string() const noexcept
        {
            return _holder && _holder->type_name() == typeid(std::string).name();
        }
        
        operator bool() const noexcept
        {
            return not is_null();
        }


        
        template<typename T>
        T value() const
        {
            if(_holder && typeid(T).name() == _holder->type_name()) {
                return std::dynamic_pointer_cast<place_holder<T>>(_holder)->value;
            } else {
                throw std::bad_cast();
            }
            
        }

        template<typename T>
        operator T() const
        {
            return value<T>();
        }

        bool is_null() const noexcept
        {
            return _holder == nullptr;
        }

        bool is_number() const noexcept
        {
            return is_number_integer() || is_number_unsigned() || is_number_float();
        }

        bool is_number_integer() const noexcept
        {
            if(is_null()) return false;
            auto type = _holder->type_name();
            return type == typeid(char).name() 
                || type == typeid(short).name() 
                || type == typeid(int).name() 
                || type == typeid(long).name() 
                || type == typeid(long long).name();
        }

        bool is_number_unsigned() const noexcept
        {
            if(is_null()) return false;
            auto type = _holder->type_name();
            return type == typeid(unsigned short).name() 
                || type == typeid(unsigned char).name() 
                || type == typeid(unsigned).name() 
                || type == typeid(unsigned long).name() 
                || type == typeid(unsigned long long).name();
        }

        bool is_number_float() const noexcept
        {
            if(is_null()) return false;
            auto type = _holder->type_name();
            return type == typeid(float).name() 
                || type == typeid(double).name();
        }

        bool is_array() const noexcept
        {
            return _holder && _holder->type_name() == typeid(array_t).name();
        }

        bool is_object() const noexcept
        {
            return _holder && _holder->type_name() == typeid(object_t).name();
        }

        struct internal_iterator
        {
            object_t::iterator object_iterator {};
            array_t::iterator array_iterator{};
        };

        struct const_iterator {
            const enum value_type {
                array, object
            } type;

            const_iterator(object_t & obj, bool begin = true) : type(object) {
                it.object_iterator = begin ? obj.begin() : obj.end();
            }

            const_iterator(array_t& arr, bool begin = true) : type(array) {
                it.array_iterator = begin ? arr.begin(): arr.end();
            }

            const std::string& key() const
            {
                if(type == object) {
                    return (*it.object_iterator).first;
                }
                throw std::runtime_error("any value type does not has key");
            }

            const any& value() const
            {
                return type == object ?
                 (*it.object_iterator).second: *it.array_iterator;
            }

            bool operator ==(const const_iterator& other) const
            {
                if(type != other.type) return false;

                switch (type) {
                case object:
                    return it.object_iterator == other.it.object_iterator;
                case array:
                    return it.array_iterator == other.it.array_iterator;
                default:
                    break;
                }
                return false;
            }

            bool operator!=(const const_iterator& other) const
            {
                return !(*this == other);
            }

            const_iterator& operator++()
            {
                switch (type) {
                case object:
                    ++it.object_iterator;
                    break;
                case array:
                    ++it.array_iterator;
                    break;
                default:
                    break;
                }
                return *this;
            }

            const_iterator operator ++(int)
            {
                auto result = *this;
                ++result;
                return result;
            }

            const_iterator& operator --()
            {
                switch (type) {
                case object:
                    --it.object_iterator;
                    break;
                case array:
                    --it.array_iterator;
                    break;
                default:
                    break;
                }
                return *this;
            }

            const_iterator operator --(int)
            {
                auto result = *this;
                ++result;
                return result;
            }

            const any& operator*() const
            {
                return type == array ? *it.array_iterator: (*it.object_iterator).second;
            }

            const any* operator ->() const
            {
                const any& n = type == array ? *it.array_iterator: (*it.object_iterator).second;
                return &n;
            }

        protected:

            internal_iterator it;
        };

        struct iterator: public const_iterator
        {
            iterator(object_t & obj, bool begin = true): const_iterator(obj, begin) {}
            iterator(array_t & arr, bool begin = true): const_iterator(arr, begin) {}

            any& operator *()
            {
                return type == array ? *it.array_iterator: (*it.object_iterator).second;
            }

            any* operator ->()
            {
                any& n = type == array ? *it.array_iterator: (*it.object_iterator).second;
                return &n;
            }

            any& value()
            {
                return type == object ? (*it.object_iterator).second: *it.array_iterator;
            }
        };

        iterator begin()
        {
            if(is_object()) {
                return iterator(std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value);
            }

            if(is_array()) {
                return iterator(std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value);
            }

            throw std::runtime_error("any value has not begin iterator");
        }

        iterator end()
        {
            if(is_object()) {
                return iterator(std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value, false);
            }

            if(is_array()) {
                return iterator(std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value, false);
            }

            throw std::runtime_error("any value has end not iterator");
        }

        const_iterator begin() const
        {
            if(is_object()) {
                return const_iterator(std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value, true);
            }

            if(is_array()) {
                return const_iterator(std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value, true);
            }

            throw std::runtime_error("any value has not const begin iterator");
        }

        const_iterator end() const
        {
            if(is_object()) {
                return const_iterator(std::dynamic_pointer_cast<place_holder<object_t>>(_holder)->value, false);
            }

            if(is_array()) {
                return const_iterator(std::dynamic_pointer_cast<place_holder<array_t>>(_holder)->value, false);
            }

            throw std::runtime_error("any value has not const end iterator");
        }


        static const any& any_null()
        {
            static any null{};
            return null;
        }
        
    private:
        std::shared_ptr<holder> _holder;	
    };

    template<typename T>
    T any_cast(const any& a)
    {
        return a.value<T>();
    }
}

#endif // !OYOUNG_ANY_HPP
