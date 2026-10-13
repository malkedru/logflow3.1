#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "CommandLine.h"
#include "ConsoleSink.h"
#include "FileLineSource.h"
#include "FileSink.h"
#include "ParserStage.h"
#include "Pipeline.h"
#include "PipelineBuilder.h"
#include "PredicateFilterStage.h"
#include "ProjectionStage.h"

typedef Stage<LogRecord, LogRecord> RecordStage;

// Komut satırı ayarlarına göre filtre ve projeksiyon aşamalarını üretir.
// Sıra: önce filtreler (ucuz ve eleyici), projeksiyon en sonda.
// Verilmeyen ayar için aşama hiç oluşturulmaz.
static std::vector<std::unique_ptr<RecordStage> > buildRecordStages(const Options& opt) {
    std::vector<std::unique_ptr<RecordStage> > stages;
    if (opt.hasMinStatus)
        stages.push_back(std::unique_ptr<RecordStage>(
            new PredicateFilterStage(Predicates::statusAtLeast(opt.minStatus))));
    if (opt.hasPathPrefix)
        stages.push_back(std::unique_ptr<RecordStage>(
            new PredicateFilterStage(Predicates::pathStartsWith(opt.pathPrefix))));
    if (!opt.fields.empty())
        stages.push_back(std::unique_ptr<RecordStage>(new ProjectionStage(opt.fields)));
    return stages;
}

// --output verilmişse FileSink, verilmemişse ConsoleSink.
static std::unique_ptr<Sink<LogRecord> > buildSink(const Options& opt) {
    RecordFormatter format = opt.fields.empty()
        ? RecordFormatter(&ConsoleSink::format)
        : LogFields::tabSeparated(opt.fields);

    if (opt.output.empty())
        return std::unique_ptr<Sink<LogRecord> >(new ConsoleSink(format));
    return std::unique_ptr<Sink<LogRecord> >(new FileSink(opt.output, format));
}

static int runLogFilter(const Options& opt) {
    FileLineSource source(opt.input);
    ParserStage parser;
    std::vector<std::unique_ptr<RecordStage> > stages = buildRecordStages(opt);
    std::unique_ptr<Sink<LogRecord> > sink = buildSink(opt);

    PipelineStep<LogRecord> chain = PipelineBuilder::from(source).via(parser);
    for (size_t i = 0; i < stages.size(); ++i)
        chain = chain.via(*stages[i]);
    Pipeline pipeline = chain.to(*sink);

    pipeline.run();

    // Özet stderr'e yazılır; stdout yalnızca kayıtları içerir (boru hattına uygun).
    std::cerr << "Ayrıştırılan kayıt: " << parser.parsedCount() << std::endl
              << "Hatalı (atlanan) satır: " << parser.badCount() << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    try {
        Options opt = CommandLine::parse(argc, argv);
        if (opt.help) {
            std::cout << CommandLine::usage();
            return 0;
        }
        return runLogFilter(opt);

    } catch (const UsageError& e) {
        std::cerr << "Hata: " << e.what() << std::endl << std::endl
                  << CommandLine::usage();
        return 2;
    } catch (const StageException& e) {
        std::cerr << "Hata: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Beklenmeyen hata: " << e.what() << std::endl;
        return 1;
    }
}
