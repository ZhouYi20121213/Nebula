#if !defined(FMX_UDP_HPP)
#define FMX_UDP_HPP

#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>

namespace fmx {
    class UdpBase {
    protected:
        int socket_fd = -1;
        unsigned long long time_out = 0;
    public:
        UdpBase() = default;
        virtual ~UdpBase() { if (socket_fd >= 0) close_connection(); }
        
        void set_timeout(unsigned long long ms) { time_out = ms; }
        
        int send_data(const std::string& data, const struct sockaddr* dest_addr, socklen_t addr_len) {
            if (time_out > 0) {
                fd_set write_set;
                FD_ZERO(&write_set);
                FD_SET(socket_fd, &write_set);
                struct timeval tv {
                    .tv_sec = time_out / 1000,
                    .tv_usec = (time_out % 1000) * 1000
                };
                if (select(socket_fd + 1, nullptr, &write_set, nullptr, &tv) <= 0)
                    return -1;
            }
            return sendto(socket_fd, data.data(), data.size(), 0, dest_addr, addr_len);
        }
        
        int recv_data(std::string& result, struct sockaddr* src_addr, socklen_t* addr_len) {
            result.clear();
            char buffer[65536];
            if (time_out > 0) {
                fd_set read_set;
                FD_ZERO(&read_set);
                FD_SET(socket_fd, &read_set);
                struct timeval tv {
                    .tv_sec = time_out / 1000,
                    .tv_usec = (time_out % 1000) * 1000
                };
                if (select(socket_fd + 1, &read_set, nullptr, nullptr, &tv) <= 0)
                    return -1;
            }
            
            ssize_t received_bytes = recvfrom(socket_fd, buffer, sizeof(buffer), 0, src_addr, addr_len);
            if (received_bytes <= 0) return -1;
            
            result.assign(buffer, received_bytes);
            return static_cast<int>(received_bytes);
        }
        
        void close_connection() {
            close(socket_fd);
            socket_fd = -1;
        }
    };

    class UdpIPv4 : public UdpBase {
    private:
        struct sockaddr_in server_addr{};
    public:
        UdpIPv4() = default;
        ~UdpIPv4() override = default;
        
        int initUdp() {
            socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
            return socket_fd < 0 ? -1 : 0;
        }
        
        int set_address(const char* ip, int port) {
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
                close(socket_fd);
                return -1;
            }
            return 0;
        }
        
        int bind_port(unsigned short port) {
            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            return bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
        }
        
        const struct sockaddr* get_server_addr() const { 
            return reinterpret_cast<const struct sockaddr*>(&server_addr); 
        }
        
        socklen_t get_addr_len() const { 
            return sizeof(server_addr); 
        }
    };

    class UdpIPv6 : public UdpBase {
    private:
        struct sockaddr_in6 server_addr{};
    public:
        UdpIPv6() = default;
        ~UdpIPv6() override = default;
        
        int initUdp() {
            socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
            return socket_fd < 0 ? -1 : 0;
        }
        
        int set_address(const char* ip, int port) {
            server_addr.sin6_family = AF_INET6;
            server_addr.sin6_port = htons(port);
            if (inet_pton(AF_INET6, ip, &server_addr.sin6_addr) <= 0) {
                close(socket_fd);
                return -1;
            }
            return 0;
        }
        
        int bind_port(unsigned short port) {
            struct sockaddr_in6 addr{};
            addr.sin6_family = AF_INET6;
            addr.sin6_addr = in6addr_any;
            addr.sin6_port = htons(port);
            return bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
        }
        
        const struct sockaddr* get_server_addr() const { 
            return reinterpret_cast<const struct sockaddr*>(&server_addr); 
        }
        
        socklen_t get_addr_len() const { 
            return sizeof(server_addr); 
        }
    };

    class UdpServerIPv4 {
    private:
        UdpIPv4 socket;
    public:
        UdpServerIPv4() = default;
        ~UdpServerIPv4() = default;
        
        int bind_port(unsigned short port) {
            if (socket.initUdp() < 0) return -1;
            return socket.bind_port(port);
        }
        
        int recv_data(std::string& result, struct sockaddr_in* client_addr) {
            socklen_t addr_len = sizeof(*client_addr);
            return socket.recv_data(
                result, 
                reinterpret_cast<struct sockaddr*>(client_addr), 
                &addr_len
            );
        }
        
        int send_data(const std::string& data, const struct sockaddr_in* client_addr) {
            return socket.send_data(
                data, 
                reinterpret_cast<const struct sockaddr*>(client_addr), 
                sizeof(*client_addr)
            );
        }
    };

    class UdpServerIPv6 {
    private:
        UdpIPv6 socket;
    public:
        UdpServerIPv6() = default;
        ~UdpServerIPv6() = default;
        
        int bind_port(unsigned short port) {
            if (socket.initUdp() < 0) return -1;
            return socket.bind_port(port);
        }
        
        int recv_data(std::string& result, struct sockaddr_in6* client_addr) {
            socklen_t addr_len = sizeof(*client_addr);
            return socket.recv_data(
                result, 
                reinterpret_cast<struct sockaddr*>(client_addr), 
                &addr_len
            );
        }
        
        int send_data(const std::string& data, const struct sockaddr_in6* client_addr) {
            return socket.send_data(
                data, 
                reinterpret_cast<const struct sockaddr*>(client_addr), 
                sizeof(*client_addr)
            );
        }
    };
}

#endif // FMX_UDP_HPP