#ifndef PREDICATEFILTERSTAGE_H
#define PREDICATEFILTERSTAGE_H

#include "Stage.h"
#include "LogRecord.h"
#include "Predicates.h"

// Kaydı yalnızca koşul sağlanıyorsa iletir (1 -> 0 veya 1 -> 1).
// Kaydı DEĞİŞTİRMEZ; aynı kaydı olduğu gibi bir sonraki aşamaya verir.
//
//   PredicateFilterStage f(Predicates::statusAtLeast(400));
class PredicateFilterStage : public Stage<LogRecord, LogRecord> {
private:
    LogPredicate predicate_;
    long long evaluated_;
    long long passed_;

public:
    explicit PredicateFilterStage(const LogPredicate& predicate)
        : predicate_(predicate), evaluated_(0), passed_(0) {
        if (!predicate_)
            throw StageException("PredicateFilterStage: koşul (predicate) boş olamaz");
    }

    void process(const LogRecord& input, Emitter<LogRecord>& out) override {
        ++evaluated_;
        if (!predicate_(input)) return;
        ++passed_;
        out.emit(input);
    }

    long long evaluatedCount() const { return evaluated_; }   // koşulun kaç kez sınandığı
    long long passedCount() const { return passed_; }
    long long droppedCount() const { return evaluated_ - passed_; }
};

#endif
