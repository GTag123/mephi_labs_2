#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include "map"
#include <utility>
#include <vector>

using namespace std;

struct BencodeCommon{
    virtual ~BencodeCommon() = default;
    virtual string encode() const = 0;
};

struct BencodeInteger : public BencodeCommon{
    long long int_val;
    BencodeInteger(long long int_val) : int_val(int_val) {}
    string encode() const override {
        return "i" + to_string(int_val) + "e";
    }
    long long get_int() const {
        return int_val;
    }
};

struct BencodeString : public BencodeCommon{
    string str_val;
    BencodeString(string str_val) : str_val(std::move(str_val)) {}
    string encode() const override {
        return to_string(str_val.length()) + ":" + str_val;
    }
    string get_str() const {
        return str_val;
    }
};

struct BencodeList : public BencodeCommon{
    vector<shared_ptr<BencodeCommon>> list_val; // TODO: make iterator
    string encode() const override {
        string result = "l";
        for (auto &item : list_val) {
            result += item->encode();
        }
        result += "e";
        return result;
    }
    size_t get_size() const{
        return list_val.size();
    }
    shared_ptr<BencodeCommon> get(size_t index) const {
        if (index >= list_val.size()) {
            throw runtime_error("Invalid bencode: index out of range");
        }
        return list_val[index];
    }
};

struct BencodeDictionary : public BencodeCommon{
    map<string, shared_ptr<BencodeCommon>> dict_val;
    string encode() const override {
        string result = "d";
        for (auto &item : dict_val) {
            result += BencodeString(item.first).encode() + item.second->encode();
        }
        result += "e";
        return result;
    }
    shared_ptr<BencodeCommon> get(const string &key) const {
        auto it = dict_val.find(key);
        if (it == dict_val.end()) {
            throw runtime_error("Invalid bencode: no key " + key);
        }
        return it->second;
    }
};

shared_ptr<BencodeCommon> parse_bencode(const char *buffer, size_t length, size_t &pos) {
    shared_ptr<BencodeCommon> result;
    char c = buffer[pos++];
    switch (c) {
        case 'i': {
            // Integer
            size_t end_pos = pos;
            while (end_pos < length && buffer[end_pos] != 'e') {
                end_pos++;
            }
            if (end_pos >= length) {
                throw runtime_error("Invalid bencode: unterminated integer");
            }
            string int_str(buffer + pos, end_pos - pos);
            result = make_shared<BencodeInteger>(stoll(int_str));
            pos = end_pos + 1;
            break;
        }
        case 'l': {
            // List
            result = make_shared<BencodeList>();
            while (buffer[pos] != 'e') {
                dynamic_pointer_cast<BencodeList>(result)->list_val.push_back(parse_bencode(buffer, length, pos));
            }
            pos++;
            break;
        }
        case 'd': {
            // Dictionary
            result = make_shared<BencodeDictionary>();
            while (buffer[pos] != 'e') {
                string key = dynamic_pointer_cast<BencodeString>(parse_bencode(buffer, length, pos))->str_val;
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
                    throw runtime_error("Invalid bencode: unexpected end of list or dictionary");
                }
                end_pos++;
            }
            if (end_pos >= length) {
                throw runtime_error("Invalid bencode: unterminated string length");
            }
            string length_str(buffer + pos - 1, end_pos - pos + 1);
            size_t str_length = stoll(length_str);
            pos = end_pos + 1;
            result = make_shared<BencodeString>(string(buffer + pos, str_length));
            pos += str_length;
            break;
        }
    }
    return result;
}