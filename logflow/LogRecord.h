#ifndef LOGRECORD_H
#define LOGRECORD_H

#include <ctime>
#include <map>
#include <string>

// Alan kaydı (domain record). DEĞİŞMEZ (immutable): tüm alanlar const'tır,
// yalnızca yapıcı (constructor) ile atanır, dışarıya sadece okuma erişimi verilir.
//
// attributes neden var?
// Kayıt biçimi genişletmeye açık olmalıdır. 5-7. haftalarda eklenecek aşamalar
// (ör. coğrafi konum, oturum kimliği, etiketler) bugünkü ayrıştırıcının hiçbir
// şey bilmediği verileri kayda ekleyecektir. Her yeni bilgi için LogRecord'a
// yeni alan ekleyip tüm aşamaları değiştirmek yerine, bu eşleme (map) ortak
// bir genişleme noktası sağlar. Kayıt değişmez olduğu için aşamalar var olan
// kaydı değiştirmez; eski alanlar + yeni attributes ile YENİ bir kayıt üretir.
class LogRecord {
private:
    const std::time_t timestamp_;   // UTC epoch saniyesi (Instant karşılığı)
    const std::string clientIp_;
    const std::string method_;
    const std::string path_;        // sorgu dizesi (query string) dahil
    const int status_;
    const long long bytes_;
    const std::string userAgent_;
    const std::map<std::string, std::string> attributes_;
    const std::string raw_;

public:
    LogRecord(std::time_t timestamp, const std::string& clientIp,
              const std::string& method, const std::string& path,
              int status, long long bytes, const std::string& userAgent,
              const std::map<std::string, std::string>& attributes,
              const std::string& raw)
        : timestamp_(timestamp), clientIp_(clientIp), method_(method),
          path_(path), status_(status), bytes_(bytes), userAgent_(userAgent),
          attributes_(attributes), raw_(raw) {}

    std::time_t timestamp() const { return timestamp_; }
    const std::string& clientIp() const { return clientIp_; }
    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    int status() const { return status_; }
    long long bytes() const { return bytes_; }
    const std::string& userAgent() const { return userAgent_; }
    const std::map<std::string, std::string>& attributes() const { return attributes_; }
    const std::string& raw() const { return raw_; }
};

#endif
