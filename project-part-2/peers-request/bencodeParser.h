#include <string>
#include <memory>
#include <unordered_map>
#include "map"
#include <utility>
#include <vector>

struct BencodeCommon;
struct BencodeInteger;
struct BencodeString;
struct BencodeList;
struct BencodeDictionary;
std::shared_ptr<BencodeCommon> parse_bencode(const char *buffer, size_t length, size_t &pos);





struct BencodeCommon{
    virtual ~BencodeCommon() = default;
    virtual std::string encode() const = 0;
};

struct BencodeInteger : public BencodeCommon{
    long long int_val;
    BencodeInteger(long long int_val) : int_val(int_val) {}
    std::string encode() const override {
        return "i" + std::to_string(int_val) + "e";
    }
    long long get_int() const {
        return int_val;
    }
};

struct BencodeString : public BencodeCommon{
    std::string str_val;
    BencodeString(std::string str_val) : str_val(std::move(str_val)) {}
    std::string encode() const override {
        return std::to_string(str_val.length()) + ":" + str_val;
    }
    std::string get_str() const {
        return str_val;
    }
};

struct BencodeList : public BencodeCommon{
    std::vector<std::shared_ptr<BencodeCommon>> list_val; // TODO: make iterator
    std::string encode() const override {
        std::string result = "l";
        for (auto &item : list_val) {
            result += item->encode();
        }
        result += "e";
        return result;
    }
    size_t get_size() const{
        return list_val.size();
    }
    std::shared_ptr<BencodeCommon> get(size_t index) const {
        if (index >= list_val.size()) {
            throw std::runtime_error("Invalid bencode: index out of range");
        }
        return list_val[index];
    }
};

struct BencodeDictionary : public BencodeCommon{
    std::map<std::string, std::shared_ptr<BencodeCommon>> dict_val;
    std::string encode() const override {
        std::string result = "d";
        for (auto &item : dict_val) {
            result += BencodeString(item.first).encode() + item.second->encode();
        }
        result += "e";
        return result;
    }
    std::shared_ptr<BencodeCommon> get(const std::string &key) const {
        auto it = dict_val.find(key);
        if (it == dict_val.end()) {
            throw std::runtime_error("Invalid bencode: no key " + key);
        }
        return it->second;
    }
};

std::shared_ptr<BencodeCommon> parse_bencode(const char *buffer, size_t length, size_t &pos) {
    std::shared_ptr<BencodeCommon> result;
    char c = buffer[pos++];
    switch (c) {
        case 'i': {
            // Integer
            size_t end_pos = pos;
            while (end_pos < length && buffer[end_pos] != 'e') {
                end_pos++;
            }
            if (end_pos >= length) {
                throw std::runtime_error("Invalid bencode: unterminated integer");
            }
            std::string int_str(buffer + pos, end_pos - pos);
            result = std::make_shared<BencodeInteger>(stoll(int_str));
            pos = end_pos + 1;
            break;
        }
        case 'l': {
            // List
            result = std::make_shared<BencodeList>();
            while (buffer[pos] != 'e') {
                dynamic_pointer_cast<BencodeList>(result)->list_val.push_back(parse_bencode(buffer, length, pos));
            }
            pos++;
            break;
        }
        case 'd': {
            // Dictionary
            result = std::make_shared<BencodeDictionary>();
            while (buffer[pos] != 'e') {
                std::string key = dynamic_pointer_cast<BencodeString>(parse_bencode(buffer, length, pos))->str_val;
                dynamic_pointer_cast<BencodeDictionary>(result)->dict_val[key] = parse_bencode(buffer, length, pos);
            }
            pos++;
            break;
        }
        default: {
            // String
            size_t end_pos = pos - 1;
            while (end_pos < length && buffer[end_pos] != ':') {
                if (buffer[end_pos] == 'e') {
                    throw std::runtime_error("Invalid bencode: unexpected end of list or dictionary");
                }
                end_pos++;
            }
            if (end_pos >= length) {
                throw std::runtime_error("Invalid bencode: unterminated string length");
            }
            std::string length_str(buffer + pos - 1, end_pos - pos + 1);
            size_t str_length = stoll(length_str);
            pos = end_pos + 1;
            result = make_shared<BencodeString>(std::string(buffer + pos, str_length));
            pos += str_length;
            break;
        }
    }
    return result;
}