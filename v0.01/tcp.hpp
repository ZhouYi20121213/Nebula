#if !defined(FMX_TCP_HPP)
#define FMX_TCP_HPP
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <fcntl.h>
#include <netdb.h>

namespace fmx {
    class TcpBase {
    protected:
        int socket_fd = -1;
        unsigned long long time_out = 0;
    public:
        TcpBase() = default;
        virtual ~TcpBase() { if (socket_fd >= 0) close_connection(); }
        void set_timeout(unsigned long long ms) { time_out = ms; }
        int send_data(const std::string& data) {
            size_t total_sent = 0;
            const char* buf = data.data();
            size_t length = data.size();
            while (total_sent < length) {
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
                ssize_t sent_bytes = send(socket_fd, buf + total_sent, length - total_sent, 0);
                if (sent_bytes <= 0) return -1;
                total_sent += sent_bytes;
            }
            return static_cast<int>(total_sent);
        }
        int recv_data(std::string& result, size_t length) {
            result.clear();
            result.reserve(length);
            size_t total_received = 0;
            char buffer[4096];
            while (total_received < length) {
                if (time_out > 0) {
                    fd_set read_set;
                    FD_ZERO(&read_set);
                    FD_SET(socket_fd, &read_set);
                    struct timeval tv {
                        .tv_sec = time_out / 1000,
                        .tv_usec = (time_out % 1000) * 1000
                    };
                    if (select(socket_fd + 1, &read_set, nullptr, nullptr, &tv) <= 0)
                        break;
                }
                size_t to_read = std::min(sizeof(buffer), length - total_received);
                ssize_t received_bytes = recv(socket_fd, buffer, to_read, 0);
                if (received_bytes <= 0) break;
                result.append(buffer, received_bytes);
                total_received += received_bytes;
            }
            return static_cast<int>(total_received);
        }
        void close_connection() {
            close(socket_fd);
            socket_fd = -1;
        }
    };

    class TcpIPv4 : public TcpBase {
    public:
        void set_fd(int fd) { socket_fd = fd; }
    private:
        struct sockaddr_in server_addr{};
    public:
        TcpIPv4() = default;
        ~TcpIPv4() override = default;
        int initTcp() {
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0) return -1;
            return 0;
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
        int connect_to_server() {
            if (time_out == 0) {
                if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    close(socket_fd);
                    return -1;
                }
                return 0;
            }
            int flags = fcntl(socket_fd, F_GETFL, 0);
            fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
            int ret = connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (ret == 0) {
                fcntl(socket_fd, F_SETFL, flags);
                return 0;
            }
            if (errno != EINPROGRESS) {
                close(socket_fd);
                return -1;
            }
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(socket_fd, &wfds);
            struct timeval tv;
            tv.tv_sec = time_out / 1000;
            tv.tv_usec = (time_out % 1000) * 1000;
            ret = select(socket_fd + 1, nullptr, &wfds, nullptr, &tv);
            if (ret <= 0) {
                close(socket_fd);
                return -1;
            }
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            fcntl(socket_fd, F_SETFL, flags);
            if (so_error != 0) {
                close(socket_fd);
                return -1;
            }
            return 0;
        }
    };

    class TcpIPv6 : public TcpBase {
    public:
        void set_fd(int fd) { socket_fd = fd; }
    private:
        struct sockaddr_in6 server_addr{};
    public:
        TcpIPv6() = default;
        ~TcpIPv6() override = default;
        int initTcp() {
            socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
            if (socket_fd < 0) return -1;
            return 0;
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
        int connect_to_server() {
            if (time_out == 0) {
                if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    close(socket_fd);
                    return -1;
                }
                return 0;
            }
            int flags = fcntl(socket_fd, F_GETFL, 0);
            fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
            int ret = connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (ret == 0) {
                fcntl(socket_fd, F_SETFL, flags);
                return 0;
            }
            if (errno != EINPROGRESS) {
                close(socket_fd);
                return -1;
            }
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(socket_fd, &wfds);
            struct timeval tv;
            tv.tv_sec = time_out / 1000;
            tv.tv_usec = (time_out % 1000) * 1000;
            ret = select(socket_fd + 1, nullptr, &wfds, nullptr, &tv);
            if (ret <= 0) {
                close(socket_fd);
                return -1;
            }
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            fcntl(socket_fd, F_SETFL, flags);
            if (so_error != 0) {
                close(socket_fd);
                return -1;
            }
            return 0;
        }
    };
    class TcpServerIPv4 {
    private:
        int listen_fd = -1;
        unsigned short port = 0;
    public:
        TcpServerIPv4() = default;
        ~TcpServerIPv4() { if (listen_fd >= 0) close(listen_fd); }
        int bind_port(unsigned short p) {
            port = p;
            listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (listen_fd < 0) return -1;
            int opt = 1;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
            return 0;
        }
        int start_listen(int backlog = 5) {
            if (listen(listen_fd, backlog) < 0) return -1;
            return 0;
        }
        TcpIPv4 accept_client() {
            struct sockaddr_in cli_addr{};
            socklen_t cli_len = sizeof(cli_addr);
            int client_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
            TcpIPv4 cli;
            cli.set_fd(client_fd);
            return cli;
        }
        int get_fd() const { return listen_fd; }
    };

    class TcpServerIPv6 {
    private:
        int listen_fd = -1;
        unsigned short port = 0;
    public:
        TcpServerIPv6() = default;
        ~TcpServerIPv6() { if (listen_fd >= 0) close(listen_fd); }
        int bind_port(unsigned short p) {
            port = p;
            listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
            if (listen_fd < 0) return -1;
            int opt = 1;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            struct sockaddr_in6 addr{};
            addr.sin6_family = AF_INET6;
            addr.sin6_addr = in6addr_any;
            addr.sin6_port = htons(port);
            if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
            return 0;
        }
        int start_listen(int backlog = 5) {
            if (listen(listen_fd, backlog) < 0) return -1;
            return 0;
        }
        TcpIPv6 accept_client() {
            struct sockaddr_in6 cli_addr{};
            socklen_t cli_len = sizeof(cli_addr);
            int client_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
            TcpIPv6 cli;
            cli.set_fd(client_fd);
            return cli;
        }
        int get_fd() const { return listen_fd; }
    };
}

#endif // FMX_TCP_HPP