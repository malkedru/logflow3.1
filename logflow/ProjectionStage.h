#ifndef PROJECTIONSTAGE_H
#define PROJECTIONSTAGE_H

#include "Stage.h"
#include "LogRecord.h"
#include "LogFields.h"
#include <map>
#include <set>
#include <string>
#include <vector>

// Her kaydı seçilen alanların bir alt kümesine indirger (1 -> 1).
//
// Veri minimizasyonu: yalnızca istenen alanlar taşınır. Seçilmeyen alanlar
// boşaltılır (metin: "", sayı: 0). Ham satır (raw) ve genişletme alanları
// (attributes) her zaman atılır; çünkü ham satır, seçilmeyen alanların
// (ör. IP, user agent) tamamını içerir ve projeksiyonu anlamsız kılardı.
//
// Çıktı hâlâ bir LogRecord'dur; bu yüzden aşama, hattın herhangi bir yerine
// (filtreden önce ya da sonra) yerleştirilebilir. DİKKAT: projeksiyondan
// sonra gelen bir filtre, atılmış alanı yalnızca boş/0 değeriyle görür.
class ProjectionStage : public Stage<LogRecord, LogRecord> {
private:
    std::set<std::string> kept_;
    long long projected_;

    bool keeps(const char* field) const { return kept_.count(field) > 0; }

public:
    explicit ProjectionStage(const std::vector<std::string>& fields) : projected_(0) {
        if (fields.empty())
            throw StageException("ProjectionStage: en az bir alan seçilmeli");
        for (size_t i = 0; i < fields.size(); ++i) {
            if (!LogFields::isKnown(fields[i]))
                throw StageException("ProjectionStage: bilinmeyen alan: " + fields[i]);
            kept_.insert(fields[i]);
        }
    }

    void process(const LogRecord& in, Emitter<LogRecord>& out) override {
        ++projected_;
        out.emit(LogRecord(
            keeps("timestamp") ? in.timestamp() : 0,
            keeps("clientIp")  ? in.clientIp()  : std::string(),
            keeps("method")    ? in.method()    : std::string(),
            keeps("path")      ? in.path()      : std::string(),
            keeps("status")    ? in.status()    : 0,
            keeps("bytes")     ? in.bytes()     : 0LL,
            keeps("userAgent") ? in.userAgent() : std::string(),
            std::map<std::string, std::string>(),
            std::string()));
    }

    long long projectedCount() const { return projected_; }
};

#endif
