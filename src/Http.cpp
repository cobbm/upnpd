#include "Http.hpp"

namespace HTTP {
    bool chariequals(char a, char b) {
        return tolower(a) == tolower(b);
    }

    int vtoi(const std::vector<uint8_t> &vec) {
        int value = 0;
        // convert to integer:
        for (auto code : vec) {
            value = value * 10 + (code - '0');
        }
        return value;
    }

    URL parseUrl(const std::string &url) {
        std::regex urlRegex(R"(^(.*?):\/\/([^\/:]+)(?::(\d+))?(\/.*)?$)");

        std::smatch matches;
        if (std::regex_match(url, matches, urlRegex)) {
            URL parsedUrl;
            parsedUrl.protocol = matches[1].str();
            parsedUrl.host = matches[2].str();
            parsedUrl.port = std::stoi(matches[3].str());
            parsedUrl.path = matches[4].str();

            return parsedUrl;
        } else {
            throw std::runtime_error("Invalid URL format");
        }
    }

    Request::Request(const std::string &method, const std::string &path, const std::string &host) {
        setVersion(1, 1);
        setMethod(method);
        setPath(path);
        if (host.size() > 0)
            addHeader("host", host);
    }

    void Request::setVersion(unsigned int major, unsigned int minor) {
        m_versionMajor = major;
        m_versionMinor = minor;
    }
    void Request::setMethod(const std::string &method) {
        m_method = method;
    }
    void Request::setPath(const std::string &path) {
        m_path = path;
    }

    void Request::addHeader(const std::string &name, const std::string &value) {
        m_headers.insert({ name, value });
    }

    std::optional<const std::string> Request::getHeader(const std::string &name) {
        auto it = m_headers.find(name);
        if (it == m_headers.end())
            return std::nullopt;
        return it->second;
    }

    void Request::setBody(const std::vector<uint8_t> &body) {
        m_body = body;
    }

    void Request::clearBody() {
        m_body.clear();
    }

    void Request::build(std::vector<uint8_t> &buff) {
        if (!getHeader("content-length").has_value() && m_body.size() > 0) {
            addHeader("content-length", std::to_string(m_body.size()));
        }

        std::stringstream ss;
        ss << m_method << " " << m_path << " "
           << "HTTP/" << m_versionMajor << "." << m_versionMinor << "\r\n";

        for (const auto &i : m_headers) {
            ss << i.first << ": " << i.second << "\r\n";
        }

        ss << "\r\n";

        buff.clear();
        std::copy(std::istreambuf_iterator<char>(ss), std::istreambuf_iterator<char>(), std::back_inserter(buff));

        if (m_body.size() > 0) {
            // TODO: is this the right way?
            std::copy(m_body.begin(), m_body.end(), std::back_inserter(buff));
        }
    }

    Response::Response(const std::vector<uint8_t> &from) {
        // parse response
        auto iter = from.begin();
        auto end = from.end();

        std::vector<uint8_t> terminator{ '\r', '\n' };
        std::vector<uint8_t> headerSep{ ':' };

        bool isStatusLine = true;
        bool isEndOfHeaders = false;

        while (!isEndOfHeaders) {
            auto chunkStart = iter;
            auto chunkEnd = std::search(iter, end, terminator.begin(), terminator.end());

            std::string line(chunkStart, chunkEnd);
            if (line.size() == 0) {
                isEndOfHeaders = true;
            } else if (isStatusLine) {
                isStatusLine = false;

                int statusParts = 0;
                auto partStart = chunkStart;
                while (statusParts < 3 && partStart < chunkEnd) {
                    auto partEnd = std::find(partStart, chunkEnd, ' ');

                    if (statusParts == 1) {
                        std::vector<uint8_t> subPart(partStart, partEnd);
                        m_statusCode = vtoi(subPart);
                    } else if (statusParts == 2) {
                        m_statusMessage = std::string(partStart, partEnd);
                    }
                    ++statusParts;
                    partStart = std::next(partEnd, 1);
                }
                if (statusParts != 3 || partStart < chunkEnd)
                    throw std::runtime_error("Failed to parse HTTP status line");

            } else {

                auto keyEnd = std::find(chunkStart, chunkEnd, ':');
                if (keyEnd >= chunkEnd) {
                    throw std::runtime_error("Failed to parse HTTP header - ':' not found!");
                }

                auto valueStart = keyEnd + 1;
                if (*(valueStart) == ' ') {
                    ++valueStart;
                }

                std::string key(chunkStart, keyEnd);
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                m_headers.insert(std::make_pair(key, std::string(valueStart, chunkEnd)));
            }
            // Advance the iterator to the next chunk
            iter = std::next(chunkEnd, 2);

            if (iter >= end || isEndOfHeaders)
                break;
        }
        if (isEndOfHeaders && iter < end) {
            m_body.clear();
            std::copy(iter, end, std::back_inserter(m_body));
        }
    }

    int Response::getStatusCode() {
        return m_statusCode;
    }

    const std::string &Response::getStatusMessage() {
        return m_statusMessage;
    }

    bool Response::ok() {
        return (m_statusCode >= 200 && m_statusCode < 400);
    }

    std::optional<const std::string> Response::getHeader(const std::string &name) {
        auto it = m_headers.find(name);
        if (it == m_headers.end())
            return std::nullopt;
        return it->second;
    }

    int Response::getVersionMinor() {
        return m_versionMinor;
    }

    int Response::getVersionMajor() {
        return m_versionMajor;
    }

    std::vector<uint8_t> &Response::getBody() {
        return m_body;
    }

}; // namespace HTTP
