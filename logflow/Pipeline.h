#ifndef PIPELINE_H
#define PIPELINE_H

#include "Stage.h"
#include "Sink.h"
#include "Emitter.h"
#include <exception>
#include <functional>
#include <vector>

// Emitter: gelen öğeyi Sink'e verir.
template<typename T>
class SinkEmitterAdapter : public Emitter<T> {
private:
    Sink<T>* sink;
public:
    explicit SinkEmitterAdapter(Sink<T>* s) : sink(s) {}
    void emit(const T& item) override { sink->consume(item); }
};

// Emitter: gelen öğeyi bir Stage'e verir, Stage'in çıktısını bir sonrakine iletir.
template<typename I, typename O>
class StageEmitterAdapter : public Emitter<I> {
private:
    Stage<I, O>* stage;
    Emitter<O>& next;
public:
    StageEmitterAdapter(Stage<I, O>* st, Emitter<O>& n) : stage(st), next(n) {}
    void emit(const I& item) override { stage->process(item, next); }
};

// Bir aşamanın (veya sink'in) open()/close() çifti.
struct LifecycleHook {
    std::function<void()> open;
    std::function<void()> close;
};

// Hazır, çalıştırılabilir bir işlem hattı. PipelineBuilder tarafından üretilir.
//
// Yaşam döngüsü sırası (hooks akış sırasıyla gelir: kaynağa yakın -> sink):
//   open  : sink önce, kaynağa en yakın aşama en son   (hedef hazır olmadan veri akmasın)
//   close : kaynağa en yakın aşama önce, sink en son   (arabellekteki veri hedefe boşalsın)
// Bir open() hata verirse, o ana kadar açılanlar kapatılır. Akış sırasında hata
// olursa da close() yine çağrılır (kaynak sızıntısı ve veri kaybı olmasın).
class Pipeline {
private:
    std::function<void()> flow_;            // kaynağı çalıştırır, veriyi zincire akıtır
    std::vector<LifecycleHook> hooks_;

    // hooks_[first..son] arasını akış sırasıyla kapatır; ilk hatayı döndürür.
    std::exception_ptr closeFrom(size_t first) {
        std::exception_ptr firstError;
        for (size_t i = first; i < hooks_.size(); ++i) {
            try { hooks_[i].close(); }
            catch (...) { if (!firstError) firstError = std::current_exception(); }
        }
        return firstError;
    }

public:
    Pipeline(const std::function<void()>& flow, const std::vector<LifecycleHook>& hooks)
        : flow_(flow), hooks_(hooks) {}

    void run() {
        size_t firstOpened = hooks_.size();
        try {
            while (firstOpened > 0) {
                hooks_[firstOpened - 1].open();
                --firstOpened;
            }
            flow_();
        } catch (...) {
            closeFrom(firstOpened);         // asıl hata korunur; kapatma hataları yutulur
            throw;
        }
        std::exception_ptr closeError = closeFrom(firstOpened);
        if (closeError) std::rethrow_exception(closeError);
    }
};

#endif
