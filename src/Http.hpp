#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <string>

namespace HTTP {

    bool chariequals(char a, char b);

    int vtoi(const std::vector<uint8_t> &vec);

    /*
    struct HTTPHeaderComp {
        bool operator()(const std::string &lhs, const std::string &rhs) const {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), chariequals);
        }
    };

    struct comp {
        bool operator()(const std::string &lhs, const std::string &rhs) const {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](char a, char b) { return tolower(a) == tolower(b); });
        }
    };

    struct case_insensitive_map {
        struct comp {
            bool operator()(const std::string &lhs, const std::string &rhs) const {
                // On non Windows OS, use the function "strcasecmp" in #include <strings.h>
                return stricmp(lhs.c_str(), rhs.c_str()) < 0;
            }
        };
    };*/

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

    /*
            HTTP/1.1 200 OK
            Server: Custom/1.0 UPnP/1.0 Proc/Ver
            EXT:
            Location: http://192.168.0.1:5431/dyndev/uuid:3c9ec793-3e70-703e-93c7-9e3c9e93700000
            Cache-Control:max-age=1800
            ST:upnp:rootdevice
            USN:uuid:3c9ec793-3e70-703e-93c7-9e3c9e93700000::upnp:rootdevice
        */

    class Response {
      public:
        Response(const std::vector<uint8_t> &);

        int getStatusCode();
        const std::string &getStatusMessage();
        bool ok();

        std::optional<const std::string> getHeader(const std::string &);

      private:
        int m_versionMajor;
        int m_versionMinor;

        int m_statusCode;
        std::string m_statusMessage;

        std::unordered_map<std::string, std::string, case_insensitive_unordered_map::hash, case_insensitive_unordered_map::comp> m_headers;

        std::vector<uint8_t> m_body;
    };
}; // namespace HTTP
