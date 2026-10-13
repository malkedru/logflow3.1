#ifndef FILESINK_H
#define FILESINK_H

#include "Sink.h"
#include "Stage.h"
#include "LogRecord.h"
#include "ConsoleSink.h"
#include "LogFields.h"
#include <fstream>
#include <string>

// LogRecord'ları bir dosyaya, her kayıt bir satır olacak şekilde yazar.
//
// Yaşam döngüsü:
//   open()    -> dosyayı oluşturur (varsa içeriğini siler); açılamazsa hata verir.
//   consume() -> bir satır yazar; open() çağrılmadıysa hata verir.
//   close()   -> arabelleği boşaltır, dosyayı kapatır; yazma hatası varsa bildirir.
//
// close() çağrılmazsa arabelleğe alınmış son satırlar diske hiç ulaşmayabilir
// ve yazma hataları (disk dolu vb.) fark edilmez. Pipeline close()'u her durumda
// çağırır; FileSink'i pipeline dışında kullanan kod bunu kendisi yapmalıdır.
class FileSink : public Sink<LogRecord> {
private:
    std::string path_;
    RecordFormatter formatter_;
    std::ofstream out_;
    long long written_;

public:
    explicit FileSink(const std::string& path,
                      const RecordFormatter& formatter = &ConsoleSink::format)
        : path_(path), formatter_(formatter), written_(0) {}

    void open() override {
        out_.open(path_.c_str(), std::ios::out | std::ios::trunc);
        if (!out_.is_open())
            throw StageException("Çıktı dosyası açılamadı: " + path_);
        written_ = 0;
    }

    void consume(const LogRecord& item) override {
        if (!out_.is_open())
            throw StageException("FileSink: open() çağrılmadan yazılamaz: " + path_);
        out_ << formatter_(item) << '\n';
        if (!out_)
            throw StageException("Çıktı dosyasına yazılamadı: " + path_);
        ++written_;
    }

    void close() override {
        if (!out_.is_open()) return;     // tekrar çağrılması zararsızdır
        out_.flush();
        bool ok = out_.good();
        out_.close();
        if (!ok || out_.fail())
            throw StageException("Çıktı dosyası düzgün kapatılamadı: " + path_);
    }

    long long writtenCount() const { return written_; }
};

#endif
