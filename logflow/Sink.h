#ifndef SINK_H
#define SINK_H

// Veriyi tüketen son halka.
// open()/close() yaşam döngüsü kancalarıdır (Stage ile aynı sözleşme):
// Pipeline, veri akmadan önce open(), akış bittikten (veya hata verdikten)
// sonra close() çağırır. Kaynak tutan sink'ler (ör. FileSink) bunları kullanır.
template<typename I>
class Sink {
public:
    virtual ~Sink() {}
    virtual void consume(const I& item) = 0;

    virtual void open() {}
    virtual void close() {}
};

#endif
