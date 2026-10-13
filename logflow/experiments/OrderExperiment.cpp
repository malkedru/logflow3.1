// Bileşilebilirlik deneyi (Artım 3, görev 4): aşama SIRASI sonucu ve maliyeti nasıl etkiler?
//
//   Deney A: iki filtrenin sırası            (durum >= 400)  ve  (yol /api ile başlar)
//   Deney B: filtre ile projeksiyonun sırası
//   Deney C: projeksiyon, filtrenin ihtiyaç duyduğu alanı atarsa (sıra artık sonucu değiştirir)
//
// Aynı veri, aynı aşama türleri; yalnızca PipelineBuilder'daki via() sırası değişir.
// Ölçütler: (1) çıktı aynı mı?  (2) iş miktarı: koşul sınama ve projeksiyon sayıları
// (kesin, her çalıştırmada aynı)  (3) süre: en iyi 5 çalıştırma (makineye bağlı).
//
//   make experiment            veya        ./build/OrderExperiment [kayit_sayisi]
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "../Emitter.h"
#include "../ParserStage.h"
#include "../PipelineBuilder.h"
#include "../PredicateFilterStage.h"
#include "../ProjectionStage.h"
#include "../Sink.h"
#include "../Source.h"

// ---- Deney verisi -----------------------------------------------------------

// Bellekteki satırları akıtır (üretim süresi ölçüme girmesin diye önceden hazırlanır).
class LineListSource : public Source<std::string> {
public:
    std::vector<std::string> lines;
    void produce(Emitter<std::string>& out) override {
        for (size_t i = 0; i < lines.size(); ++i) out.emit(lines[i]);
    }
};

// Sabit tohumlu sahte rastgele sayı üreteci: her çalıştırmada aynı veri.
static unsigned nextRandom(unsigned& state) {
    state = state * 1664525u + 1013904223u;
    return state >> 8;
}

// Gerçekçi bir dağılım: ~%10 hata (>= 400), ~%30 /api isteği; ikisi bağımsız.
static std::vector<std::string> generateLines(size_t count) {
    static const char* agents[] = {
        "Mozilla/5.0 (X11; Linux x86_64) Firefox/120.0",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Chrome/119.0 Safari/537.36",
        "curl/8.0.1"
    };
    unsigned rng = 12345;
    std::vector<std::string> lines;
    lines.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        unsigned r = nextRandom(rng) % 100;
        int status = r < 85 ? 200 : r < 90 ? 304 : r < 97 ? 404 : 500;
        bool api = nextRandom(rng) % 100 < 30;
        unsigned id = nextRandom(rng) % 1000;

        std::string path = api ? "/api/items/" : "/static/page";
        path += std::to_string(id);
        unsigned sec = static_cast<unsigned>(i % 86400);

        char line[512];
        std::snprintf(line, sizeof(line),
            "10.0.%u.%u - - [05/Mar/2024:%02u:%02u:%02u +0000] \"GET %s HTTP/1.1\" %d %u "
            "\"-\" \"%s\"",
            id % 256, nextRandom(rng) % 256, sec / 3600, (sec / 60) % 60, sec % 60,
            path.c_str(), status, nextRandom(rng) % 90000, agents[nextRandom(rng) % 3]);
        lines.push_back(line);
    }
    return lines;
}

// ---- Ölçüm düzeneği ---------------------------------------------------------

// Çıktıyı biçimlenmiş satırlar olarak toplar (iki sıranın çıktısını karşılaştırmak için).
class CapturingSink : public Sink<LogRecord> {
public:
    RecordFormatter format;
    std::vector<std::string> lines;
    explicit CapturingSink(const RecordFormatter& f) : format(f) {}
    void consume(const LogRecord& r) override { lines.push_back(format(r)); }
};

struct RunResult {
    std::vector<std::string> output;
    long long statusEvals, pathEvals, projected;
    double millis;
};

enum Order { STATUS_PATH_PROJ, PATH_STATUS_PROJ, PROJ_STATUS_PATH };

static const char* describe(Order o) {
    switch (o) {
        case STATUS_PATH_PROJ:       return "durum -> yol -> projeksiyon";
        case PATH_STATUS_PROJ:       return "yol -> durum -> projeksiyon";
        case PROJ_STATUS_PATH:       return "projeksiyon -> durum -> yol";
    }
    return "";
}

static RunResult runOnce(LineListSource& source, Order order,
                         const std::vector<std::string>& fields) {
    ParserStage parser;
    PredicateFilterStage status(Predicates::statusAtLeast(400));
    PredicateFilterStage path(Predicates::pathStartsWith("/api"));
    ProjectionStage projection(fields);
    CapturingSink sink(LogFields::tabSeparated(fields));

    PipelineStep<LogRecord> chain = PipelineBuilder::from(source).via(parser);
    switch (order) {
        case STATUS_PATH_PROJ:       chain = chain.via(status).via(path).via(projection); break;
        case PATH_STATUS_PROJ:       chain = chain.via(path).via(status).via(projection); break;
        case PROJ_STATUS_PATH:       chain = chain.via(projection).via(status).via(path); break;
    }
    Pipeline pipeline = chain.to(sink);

    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    pipeline.run();
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

    RunResult r;
    r.output = sink.lines;
    r.statusEvals = status.evaluatedCount();
    r.pathEvals = path.evaluatedCount();
    r.projected = projection.projectedCount();
    r.millis = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return r;
}

// En iyi süre (ısınma/gürültüye karşı), iş sayıları ilk çalıştırmadan.
static RunResult measure(LineListSource& source, Order order,
                         const std::vector<std::string>& fields, int repeats = 5) {
    RunResult best = runOnce(source, order, fields);
    for (int i = 1; i < repeats; ++i) {
        RunResult r = runOnce(source, order, fields);
        if (r.millis < best.millis) best.millis = r.millis;
    }
    return best;
}

static void printRow(Order order, const RunResult& r) {
    std::cout << "  " << std::left << std::setw(32) << describe(order) << std::right
              << std::setw(10) << r.statusEvals << std::setw(10) << r.pathEvals
              << std::setw(11) << r.projected << std::setw(9) << r.output.size()
              << std::setw(10) << std::fixed << std::setprecision(1) << r.millis << " ms\n";
}

static void printHeader() {
    std::cout << "  " << std::left << std::setw(32) << "sira" << std::right
              << std::setw(10) << "durum" << std::setw(10) << "yol"
              << std::setw(11) << "projekte" << std::setw(9) << "cikti"
              << std::setw(13) << "sure" << "\n"
              << "  " << std::left << std::setw(32) << "" << std::right
              << std::setw(10) << "sinama" << std::setw(10) << "sinama" << "\n";
}

int main(int argc, char* argv[]) {
    size_t count = argc > 1 ? static_cast<size_t>(std::atol(argv[1])) : 500000;
    LineListSource source;
    source.lines = generateLines(count);
    std::cout << "Veri: " << count << " satir; ~%10 hata (>= 400), ~%30 /api\n\n";

    std::vector<std::string> keep;      // filtrelerin ihtiyaç duyduğu alanlar KORUNUR
    keep.push_back("timestamp"); keep.push_back("clientIp");
    keep.push_back("status");    keep.push_back("path");

    // Deney A: iki filtrenin sırası
    RunResult sp = measure(source, STATUS_PATH_PROJ, keep);
    RunResult ps = measure(source, PATH_STATUS_PROJ, keep);
    std::cout << "Deney A - iki filtrenin sirasi (projeksiyon en sonda)\n";
    printHeader(); printRow(STATUS_PATH_PROJ, sp); printRow(PATH_STATUS_PROJ, ps);
    std::cout << "  cikti ayni mi: " << (sp.output == ps.output ? "EVET" : "HAYIR") << "\n\n";

    // Deney B: filtre / projeksiyon sırası
    RunResult proj = measure(source, PROJ_STATUS_PATH, keep);
    std::cout << "Deney B - projeksiyon filtreden once mi, sonra mi?\n";
    printHeader(); printRow(STATUS_PATH_PROJ, sp); printRow(PROJ_STATUS_PATH, proj);
    std::cout << "  cikti ayni mi: " << (sp.output == proj.output ? "EVET" : "HAYIR") << "\n\n";

    // Deney C: projeksiyon filtrenin alanını (status) atarsa
    std::vector<std::string> noStatus;
    noStatus.push_back("timestamp"); noStatus.push_back("clientIp"); noStatus.push_back("path");
    RunResult safe = runOnce(source, STATUS_PATH_PROJ, noStatus);
    RunResult unsafe = runOnce(source, PROJ_STATUS_PATH, noStatus);
    std::cout << "Deney C - projeksiyon 'status' alanini atarsa (--fields timestamp,clientIp,path)\n";
    printHeader(); printRow(STATUS_PATH_PROJ, safe); printRow(PROJ_STATUS_PATH, unsafe);
    std::cout << "  cikti ayni mi: " << (safe.output == unsafe.output ? "EVET" : "HAYIR") << "\n";

    bool ok = sp.output == ps.output && sp.output == proj.output && safe.output != unsafe.output;
    std::cout << "\n" << (ok ? "Beklenen sonuclar dogrulandi." : "BEKLENMEYEN SONUC!") << std::endl;
    return ok ? 0 : 1;
}
