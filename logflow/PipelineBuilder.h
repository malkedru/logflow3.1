#ifndef PIPELINEBUILDER_H
#define PIPELINEBUILDER_H

#include "Pipeline.h"
#include "Source.h"
#include "Stage.h"
#include "Sink.h"
#include <functional>
#include <vector>

// Akıcı (fluent) hat kurucu:
//
//   Pipeline p = PipelineBuilder::from(source)
//                    .via(parser)
//                    .via(filter)
//                    .via(projection)
//                    .to(sink);
//   p.run();
//
// Aşamaların tipleri DERLEME ZAMANINDA denetlenir: via(), yalnızca girdi tipi
// zincirin o anki çıktı tipine uyan bir Stage kabul eder; sink de aynı şekilde.
// Sıra ve sayı serbesttir. Builder aşamaları SAHİPLENMEZ (kopyalamaz): aşama ve
// sink nesneleri, Pipeline::run() bitene kadar yaşamalıdır.

// Zincirin kurulmakta olan hâli; T = şu ana kadar akıtılan öğe tipi.
// Doğrudan oluşturulmaz, PipelineBuilder::from() ile başlanır.
template<typename T>
class PipelineStep {
private:
    typedef std::function<void(Emitter<T>&)> Feed;   // zinciri verilen emitter'a bağlar

    Feed feed_;
    std::vector<LifecycleHook> hooks_;

    PipelineStep(const Feed& feed, const std::vector<LifecycleHook>& hooks)
        : feed_(feed), hooks_(hooks) {}

    template<typename U> friend class PipelineStep;
    friend class PipelineBuilder;

public:
    // Zincire bir aşama ekler: Stage<T, O> -> artık O tipi akar.
    template<typename O>
    PipelineStep<O> via(Stage<T, O>& stage) const {
        Feed upstream = feed_;
        typename PipelineStep<O>::Feed feed = [upstream, &stage](Emitter<O>& out) {
            StageEmitterAdapter<T, O> adapter(&stage, out);
            upstream(adapter);
        };

        std::vector<LifecycleHook> hooks = hooks_;
        LifecycleHook hook;
        hook.open  = [&stage]() { stage.open(); };
        hook.close = [&stage]() { stage.close(); };
        hooks.push_back(hook);

        return PipelineStep<O>(feed, hooks);
    }

    // Zinciri bir Sink ile bitirir ve çalıştırılabilir Pipeline'ı döndürür.
    Pipeline to(Sink<T>& sink) const {
        Feed upstream = feed_;
        std::function<void()> flow = [upstream, &sink]() {
            SinkEmitterAdapter<T> adapter(&sink);
            upstream(adapter);
        };

        std::vector<LifecycleHook> hooks = hooks_;
        LifecycleHook hook;
        hook.open  = [&sink]() { sink.open(); };
        hook.close = [&sink]() { sink.close(); };
        hooks.push_back(hook);

        return Pipeline(flow, hooks);
    }
};

// Giriş noktası: zinciri bir Source ile başlatır.
class PipelineBuilder {
public:
    template<typename T>
    static PipelineStep<T> from(Source<T>& source) {
        typename PipelineStep<T>::Feed feed = [&source](Emitter<T>& out) {
            source.produce(out);
        };
        return PipelineStep<T>(feed, std::vector<LifecycleHook>());
    }
};

#endif
