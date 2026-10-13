#ifndef PREDICATES_H
#define PREDICATES_H

#include "LogRecord.h"
#include <ctime>
#include <functional>
#include <string>

// Bir kaydın koşulu sağlayıp sağlamadığını söyleyen işlev (Java'daki Predicate<LogRecord>).
typedef std::function<bool(const LogRecord&)> LogPredicate;

// PredicateFilterStage ile kullanılan hazır koşullar.
// Her biri ayrı ve küçük bir işlevdir; yeni koşul eklemek aşamayı değiştirmeyi gerektirmez.
namespace Predicates {

// status >= alt sınır
inline LogPredicate statusAtLeast(int minStatus) {
    return [minStatus](const LogRecord& r) { return r.status() >= minStatus; };
}

// path, verilen önekle başlıyor mu?
inline LogPredicate pathStartsWith(const std::string& prefix) {
    return [prefix](const LogRecord& r) {
        return r.path().compare(0, prefix.size(), prefix) == 0;
    };
}

// from <= zaman damgası <= to (iki uç dahil, UTC epoch saniyesi)
inline LogPredicate timestampBetween(std::time_t from, std::time_t to) {
    return [from, to](const LogRecord& r) {
        return r.timestamp() >= from && r.timestamp() <= to;
    };
}

}  // namespace Predicates

#endif
