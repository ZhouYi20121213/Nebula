#if !defined(FMX_SCTP_HPP)
#define FMX_SCTP_HPP

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <string>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <cstring>

namespace fmx {
    class SctpBase {
    protected:
        int socket_fd = -1;
        unsigned long long time_out = 0;
    public:
        SctpBase() = default;
        virtual ~SctpBase() { if (socket_fd >= 0) close_connection(); }
        int get_socket_fd() const { return socket_fd; }
        void set_timeout(unsigned long long ms) { time_out = ms; }
        
        int send_data(const std::string& data, const struct sockaddr* dest_addr, socklen_t addr_len, int stream_no = 0) {
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
            return sctp_sendmsg(
                socket_fd, data.data(), data.size(), 
               (sockaddr*)dest_addr, addr_len, 
                0, 0, stream_no, 0, 0
            );
        }
        
        int recv_data(std::string& result, struct sockaddr* src_addr, socklen_t* addr_len, int* stream_no = nullptr) {
            result.clear();
            char buffer[65536];
            struct sctp_sndrcvinfo sri;
            int msg_flags = 0;

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
            
            ssize_t received_bytes = sctp_recvmsg(
                socket_fd, buffer, sizeof(buffer),
                src_addr, addr_len, 
                &sri, &msg_flags
            );
            if (received_bytes <= 0) return -1;
            
            if (stream_no) *stream_no = sri.sinfo_stream;
            result.assign(buffer, received_bytes);
            return static_cast<int>(received_bytes);
        }
        
        void close_connection() {
            close(socket_fd);
            socket_fd = -1;
        }
    };

    class SctpIPv4 : public SctpBase {
    private:
        struct sockaddr_in server_addr{};
    public:
        SctpIPv4() = default;
        ~SctpIPv4() override = default;
        
        int initSctp() {
            socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
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
        
        int connect_to_server() {
            return connect(socket_fd, get_server_addr(), get_addr_len());
        }
    };

    class SctpIPv6 : public SctpBase {
    private:
        struct sockaddr_in6 server_addr{};
    public:
        SctpIPv6() = default;
        ~SctpIPv6() override = default;
        
        int initSctp() {
            socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
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
        
        int connect_to_server() {
            return connect(socket_fd, get_server_addr(), get_addr_len());
        }
    };

    class SctpServerIPv4 {
    private:
        SctpIPv4 socket;
    public:
        SctpServerIPv4() = default;
        ~SctpServerIPv4() = default;
        
        int bind_port(unsigned short port) {
            if (socket.initSctp() < 0) return -1;
            
            // Enable SCTP notifications
            struct sctp_event_subscribe events;
            memset(&events, 0, sizeof(events));
            events.sctp_data_io_event = 1;
            setsockopt(socket.get_socket_fd(), IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events));
            
            return socket.bind_port(port);
        }
        
        int listen_connections(int backlog = 5) {
            return listen(socket.get_socket_fd(), backlog);
        }
        
        int accept_connection(struct sockaddr_in* client_addr) {
            socklen_t addr_len = sizeof(*client_addr);
            return accept(socket.get_socket_fd(), 
                         reinterpret_cast<struct sockaddr*>(client_addr), 
                         &addr_len);
        }
        
        int recv_data(std::string& result, struct sockaddr_in* client_addr, int* stream_no = nullptr) {
            socklen_t addr_len = sizeof(*client_addr);
            return socket.recv_data(
                result, 
                reinterpret_cast<struct sockaddr*>(client_addr), 
                &addr_len,
                stream_no
            );
        }
        
        int send_data(const std::string& data, const struct sockaddr_in* client_addr, int stream_no = 0) {
            return socket.send_data(
                data, 
                reinterpret_cast<const struct sockaddr*>(client_addr), 
                sizeof(*client_addr),
                stream_no
            );
        }
        
        int get_socket_fd() const { return socket.get_socket_fd(); }
    };

    class SctpServerIPv6 {
    private:
        SctpIPv6 socket;
    public:
        SctpServerIPv6() = default;
        ~SctpServerIPv6() = default;
        
        int bind_port(unsigned short port) {
            if (socket.initSctp() < 0) return -1;
            
            // Enable SCTP notifications
            struct sctp_event_subscribe events;
            memset(&events, 0, sizeof(events));
            events.sctp_data_io_event = 1;
            setsockopt(socket.get_socket_fd(), IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events));
            
            return socket.bind_port(port);
        }
        
        int listen_connections(int backlog = 5) {
            return listen(socket.get_socket_fd(), backlog);
        }
        
        int accept_connection(struct sockaddr_in6* client_addr) {
            socklen_t addr_len = sizeof(*client_addr);
            return accept(socket.get_socket_fd(), 
                         reinterpret_cast<struct sockaddr*>(client_addr), 
                         &addr_len);
        }
        
        int recv_data(std::string& result, struct sockaddr_in6* client_addr, int* stream_no = nullptr) {
            socklen_t addr_len = sizeof(*client_addr);
            return socket.recv_data(
                result, 
                reinterpret_cast<struct sockaddr*>(client_addr), 
                &addr_len,
                stream_no
            );
        }
        
        int send_data(const std::string& data, const struct sockaddr_in6* client_addr, int stream_no = 0) {
            return socket.send_data(
                data, 
                reinterpret_cast<const struct sockaddr*>(client_addr), 
                sizeof(*client_addr),
                stream_no
            );
        }
        
        int get_socket_fd() const { return socket.get_socket_fd(); }
    };
}

#endif // FMX_SCTP_HPP