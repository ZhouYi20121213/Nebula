#if !defined(FMX_TLS_HPP)
#define FMX_TLS_HPP
#include "tcp.hpp"
#include <stdint.h>
namespace fmx {

    class Tls{
        private:
        std::string TlsVersion;
        uint32_t TlsCipher;
        std::string TlsCert;
        std::string TlsKey;
        std::string TlsCaCert;
        std::string TlsSni;
        unsigned long long TlsTimeout = 0;
        public:
        Tls() = default;
        ~Tls() = default;
        void set_version(const std::string& version) { TlsVersion = version; }
        void set_cipher(uint32_t cipher) { TlsCipher = cipher; }
        void set_cert(const std::string& cert) { TlsCert = cert; }
        void set_key(const std::string& key) { TlsKey = key; }
        void set_ca_cert(const std::string& ca_cert) { TlsCaCert = ca_cert; }
        void set_sni(const std::string& sni) { TlsSni = sni; }
        void set_timeout(unsigned long long timeout) { TlsTimeout = timeout; }
        const std::string& get_version() const { return TlsVersion; }
        uint32_t get_cipher() const { return TlsCipher; }
        const std::string& get_cert() const { return TlsCert; }
        const std::string& get_key() const { return TlsKey; }
        const std::string& get_ca_cert() const { return TlsCaCert; }
        const std::string& get_sni() const { return TlsSni; }
        unsigned long long get_timeout() const { return TlsTimeout; }
    };

} // namespace fmx

#endif // FMX_TLS_HPP