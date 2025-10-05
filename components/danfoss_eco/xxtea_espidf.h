// xxtea_espidf.h
// Pure C++ XXTEA implementation for ESP-IDF (no Arduino dependencies)
#pragma once

#include <cstdint>
#include <cstring>

#define MAX_XXTEA_KEY8 16
#define MAX_XXTEA_KEY32 4
#define MAX_XXTEA_DATA8 256
#define MAX_XXTEA_DATA32 64

#define XXTEA_STATUS_SUCCESS 0
#define XXTEA_STATUS_NOT_INITIALIZED -1
#define XXTEA_STATUS_GENERAL_ERROR -2
#define XXTEA_STATUS_PARAMETER_ERROR -3
#define XXTEA_STATUS_SIZE_ERROR -4

#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))

class XxteaESPIDF {
public:
    XxteaESPIDF() : status_(XXTEA_STATUS_NOT_INITIALIZED) {
        memset(xxtea_key_, 0, sizeof(xxtea_key_));
        memset(xxtea_data_, 0, sizeof(xxtea_data_));
    }

    int set_key(const uint8_t *key, size_t len) {
        status_ = XXTEA_STATUS_GENERAL_ERROR;
        
        if (key == nullptr || len <= 0 || len > MAX_XXTEA_KEY8) {
            status_ = XXTEA_STATUS_PARAMETER_ERROR;
            return status_;
        }

        size_t key_len_32 = (len + 3) / 4;
        if (key_len_32 > MAX_XXTEA_KEY32) {
            status_ = XXTEA_STATUS_SIZE_ERROR;
            return status_;
        }

        memset(xxtea_key_, 0, sizeof(xxtea_key_));
        memcpy(xxtea_key_, key, len);

        status_ = XXTEA_STATUS_SUCCESS;
        return status_;
    }

    int encrypt(const uint8_t *data, size_t len, uint8_t *buf, size_t *maxlen) {
        if (data == nullptr || len <= 0 || len > MAX_XXTEA_DATA8 ||
            buf == nullptr || maxlen == nullptr || *maxlen <= 0 || *maxlen < len) {
            return XXTEA_STATUS_PARAMETER_ERROR;
        }

        size_t n = (len + 3) / 4;
        if (n > MAX_XXTEA_DATA32 || *maxlen < (n * 4)) {
            return XXTEA_STATUS_SIZE_ERROR;
        }

        memset(xxtea_data_, 0, sizeof(xxtea_data_));
        memcpy(xxtea_data_, data, len);

        xxtea_encrypt(xxtea_data_, n);

        memcpy(buf, xxtea_data_, n * 4);
        *maxlen = n * 4;

        return XXTEA_STATUS_SUCCESS;
    }

    int decrypt(uint8_t *data, size_t len) {
        if (data == nullptr || len <= 0 || (len % 4) != 0) {
            return XXTEA_STATUS_PARAMETER_ERROR;
        }
        if (len > MAX_XXTEA_DATA8) {
            return XXTEA_STATUS_SIZE_ERROR;
        }

        memset(xxtea_data_, 0, sizeof(xxtea_data_));
        memcpy(xxtea_data_, data, len);

        size_t n = len / 4;
        xxtea_decrypt(xxtea_data_, n);

        memcpy(data, xxtea_data_, len);

        return XXTEA_STATUS_SUCCESS;
    }

    int status() const { return status_; }

private:
    void xxtea_encrypt(uint32_t *v, size_t n) {
        uint32_t y, z, sum;
        unsigned p, rounds, e;
        
        if (n <= 1) return;
        
        rounds = 6 + 52 / n;
        sum = 0;
        z = v[n - 1];
        
        do {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p = 0; p < n - 1; p++) {
                y = v[p + 1];
                z = v[p] += MX;
            }
            y = v[0];
            z = v[n - 1] += MX;
        } while (--rounds);
    }

    void xxtea_decrypt(uint32_t *v, size_t n) {
        uint32_t y, z, sum;
        unsigned p, rounds, e;
        
        if (n <= 1) return;
        
        rounds = 6 + 52 / n;
        sum = rounds * DELTA;
        y = v[0];
        
        do {
            e = (sum >> 2) & 3;
            for (p = n - 1; p > 0; p--) {
                z = v[p - 1];
                y = v[p] -= MX;
            }
            z = v[n - 1];
            y = v[0] -= MX;
            sum -= DELTA;
        } while (--rounds);
    }

    int status_;
    uint32_t xxtea_data_[MAX_XXTEA_DATA32];
    uint32_t xxtea_key_[MAX_XXTEA_KEY32];
};

// Compatibility typedef
using Xxtea = XxteaESPIDF;
