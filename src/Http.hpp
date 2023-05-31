#pragma once

#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace HTTP {

    struct URL {
        std::string protocol;
        std::string host;
        int port;
        std::string path;
    };

    bool chariequals(char, char);

    int vtoi(const std::vector<uint8_t> &);

    URL parseUrl(const std::string &);

    struct case_insensitive_unordered_map {
        struct comp {
            bool operator()(const std::string &lhs, const std::string &rhs) const {
                return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), chariequals);
            }
        };

        struct hash {
            std::size_t operator()(std::string str) const {

                unsigned long hash = 5381;

                for (std::size_t index = 0; index < str.size(); ++index) {
                    auto ch = static_cast<unsigned char>(str[index]);
                    hash = ((hash << 5) + hash) + std::tolower(ch); /* hash * 33 + c */
                }

                return hash;
            }
        };
    };

    class Request {
      public:
        Request(const std::string &, const std::string & = "/", const std::string & = "");

        void setVersion(unsigned int, unsigned int);
        void setMethod(const std::string &);
        void setPath(const std::string &);

        void addHeader(const std::string &, const std::string &);
        std::optional<const std::string> getHeader(const std::string &);

        void setBody(const std::vector<uint8_t> &);
        void clearBody();

        void build(std::vector<uint8_t> &);

      private:
        int m_versionMajor;
        int m_versionMinor;

        std::string m_method;
        std::string m_path;

        std::unordered_map<std::string, std::string, case_insensitive_unordered_map::hash, case_insensitive_unordered_map::comp> m_headers;

        std::vector<uint8_t> m_body;
    };

    class Response {
      public:
        Response(const std::vector<uint8_t> &);

        int getStatusCode();
        const std::string &getStatusMessage();
        bool ok();

        std::optional<const std::string> getHeader(const std::string &);

        int getVersionMinor();
        int getVersionMajor();

        std::vector<uint8_t> &getBody();

      private:
        int m_versionMajor;
        int m_versionMinor;

        int m_statusCode;
        std::string m_statusMessage;

        std::unordered_map<std::string, std::string, case_insensitive_unordered_map::hash, case_insensitive_unordered_map::comp> m_headers;

        std::vector<uint8_t> m_body;
    };
}; // namespace HTTP
