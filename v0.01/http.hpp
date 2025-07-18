#ifndef FMX_HTTP_HPP
#define FMX_HTTP_HPP

#include "tcp.hpp"
#include <netdb.h>
#include <string>

namespace fmx {
    template <typename AddrType> class TcpImpl;

    class HttpBase {
    protected:
        std::string host;
        unsigned short port = 80;
        std::string request;
        std::string response;
    public:
        virtual ~HttpBase() = default;
        virtual int initHttp(const std::string& host) = 0;
        virtual int connectToServer() = 0;
        virtual int sendRequest() = 0;
        virtual int receiveResponse() = 0;
        virtual const char* getResponse() const = 0;
        virtual int GET(const std::string& path = "/", const std::string& headers = "") = 0;
        virtual int POST(const std::string& path, const std::string& body = "", const std::string& headers = "") = 0;
        virtual int HEAD(const std::string& path = "/", const std::string& headers = "") = 0;
        virtual int PUT(const std::string& path, const std::string& body = "", const std::string& headers = "") = 0;
        virtual int DELETE(const std::string& path, const std::string& headers = "") = 0;
        virtual void setTimeout(unsigned long long ms) = 0;
        virtual int set_port(unsigned short p) = 0; 
    };
    template <typename TcpType>
    class HttpImpl : public HttpBase {
    private:
        TcpType tcp;
    protected:
        int build_and_send(const std::string& method, const std::string& path, 
                        const std::string& headers, const std::string& body) {
            request = method + " " + path + " HTTP/1.1\r\n";
            request += "Host: " + host + "\r\n";
            request += "User-Agent: FMX-HttpClient/1.0\r\n";
            request += "Accept: */*\r\n";
            request += "Connection: close\r\n";
            
            if (!headers.empty()) {
                std::string h = headers;
                size_t pos = 0;
                while ((pos = h.find("\n", pos)) != std::string::npos) {
                    if (pos == 0 || h[pos-1] != '\r') h.replace(pos, 1, "\r\n");
                    pos += 2;
                }
                request += h;
                if (h.substr(h.size()-2) != "\r\n") request += "\r\n";
            }
            
            if (!body.empty()) {
                request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            }
            
            request += "\r\n";
            if (!body.empty()) request += body;
            
            return sendRequest();
        }

    public:
        HttpImpl() = default;
        ~HttpImpl() override { tcp.close_connection(); }
        
        int initHttp(const std::string& host) override {
            this->host = host;
            struct addrinfo hints{}, *res;
            hints.ai_family = TcpType::get_address_family();
            hints.ai_socktype = SOCK_STREAM;
            
            if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0) {
                return -1; // Error resolving host
            }
            
            port = 80; // Default HTTP port
            tcp.initTcp();
            
            if (tcp.set_address(host.c_str(), port) < 0) {
                freeaddrinfo(res);
                return -1; // Error setting address
            }
            
            freeaddrinfo(res);
            return 0;
        }
        int connectToServer() override {return tcp.connect_to_server();}
        int sendRequest() override {return tcp.send_data(request.c_str(), request.size());}
        int receiveResponse() override {
            response.clear();
            char buffer[4096];
            int bytes_received=0;
            while ((bytes_received = tcp.receive_data(buffer, sizeof(buffer))) > 0) {
                response.append(buffer, bytes_received);
            }
            return bytes_received;
        }
        const char* getResponse() const override {return response.c_str();}
        int GET(const std::string& path, const std::string& headers) override 
        {return build_and_send("GET", path, headers, "");}
        int POST(const std::string& path, const std::string& body, const std::string& headers) override 
        {return build_and_send("POST", path, headers, body);}
        int HEAD(const std::string& path, const std::string& headers) override 
        {return build_and_send("HEAD", path, headers, "");}
        int PUT(const std::string& path, const std::string& body, const std::string& headers) override 
        {return build_and_send("PUT", path, headers, body);}
        int DELETE(const std::string& path, const std::string& headers) override 
        {return build_and_send("DELETE", path, headers, "");}
        void setTimeout(unsigned long long ms) override {tcp.set_timeout(ms);}
        int set_port(unsigned short p) override {
            port = p;
            return 0;
        }
    };

    class Httpv4 : public HttpImpl<TcpIPv4> {
    public:
        Httpv4() = default;
    };

    class Httpv6 : public HttpImpl<TcpIPv6> {
    public:
        Httpv6() = default;
    };

} // namespace fmx

#endif // FMX_HTTP_HPP