#ifndef LOGFIELDS_H
#define LOGFIELDS_H

#include "LogRecord.h"
#include "Stage.h"
#include <ctime>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

// Bir LogRecord'u metne çeviren işlev (ConsoleSink ve FileSink ortak kullanır).
typedef std::function<std::string(const LogRecord&)> RecordFormatter;

// LogRecord alanlarının adları ve metin gösterimleri.
// Hem ProjectionStage (hangi alanlar kalsın?) hem komut satırı (--fields)
// hem de çıktı biçimleyici aynı alan adlarını kullanır; tek doğruluk kaynağı burasıdır.
namespace LogFields {

inline const std::vector<std::string>& names() {
    static const std::vector<std::string> all = {
        "timestamp", "clientIp", "method", "path", "status", "bytes", "userAgent"
    };
    return all;
}

inline bool isKnown(const std::string& name) {
    const std::vector<std::string>& all = names();
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i] == name) return true;
    return false;
}

// UTC epoch saniyesi -> "2024-01-01T10:15:30Z"
inline std::string formatTimestamp(std::time_t t) {
    std::tm* parts = std::gmtime(&t);
    if (!parts) return "";
    char buf[32];
    std::size_t n = std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", parts);
    return std::string(buf, n);
}

inline std::string valueOf(const LogRecord& r, const std::string& field) {
    std::ostringstream os;
    if (field == "timestamp")      return formatTimestamp(r.timestamp());
    else if (field == "clientIp")  return r.clientIp();
    else if (field == "method")    return r.method();
    else if (field == "path")      return r.path();
    else if (field == "userAgent") return r.userAgent();
    else if (field == "status")    os << r.status();
    else if (field == "bytes")     os << r.bytes();
    else throw StageException("Bilinmeyen alan: " + field);
    return os.str();
}

// Seçilen alanları, verilen sırayla, TAB ile ayırarak tek satıra yazar.
inline RecordFormatter tabSeparated(const std::vector<std::string>& fields) {
    return [fields](const LogRecord& r) -> std::string {
        std::string line;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) line += '\t';
            line += valueOf(r, fields[i]);
        }
        return line;
    };
}

}  // namespace LogFields

#endif
