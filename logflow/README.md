# LogFlow - Pipeline Mimarisi (Artım 3: Bileşim - Filtreleme, Projeksiyon, Pipeline Builder)

## Proje Hakkında
LogFlow, bir erişim günlüğünü (Ortak Günlük Biçimi / CLF) okuyan, ayrıştıran,
filtreleyen, istenen alanlara indirgeyen ve bir dosyaya (ya da ekrana) yazan,
**yapılandırılabilir bir komut satırı log filtresidir.** Artım 3'te aşamalar
`PipelineBuilder` ile herhangi bir sırada ve sayıda birleştirilir.

## Kullanım

```
logflow <girdi.log> [--min-status N] [--path-prefix ÖNEK] [--fields a,b,c] [--output DOSYA]
```

| Seçenek | Anlamı |
|---------|--------|
| `--min-status N` | yalnızca `status >= N` olan kayıtlar (100-599) |
| `--path-prefix ÖNEK` | yalnızca yolu `ÖNEK` ile başlayan kayıtlar |
| `--fields a,b,c` | yalnızca bu alanlar yazılır (verilen sırayla, TAB ile ayrılmış) |
| `--output DOSYA` | `DOSYA`'ya yazar; verilmezse ekrana |
| `--help` | yardım |

Alanlar: `timestamp, clientIp, method, path, status, bytes, userAgent`.
Verilmeyen seçenek için ilgili aşama hatta hiç eklenmez. Özet (ayrıştırılan/atlanan
satır sayısı) stderr'e yazılır; stdout yalnızca kayıtları içerir.

```
$ ./logflow data/access-small.log --min-status 400 --fields timestamp,clientIp,status,path
2024-01-01T10:15:34Z	192.168.1.4	404	/logo.png
```

Çıkış kodları: `0` başarılı, `1` çalışma hatası (dosya açılamadı vb.), `2` hatalı kullanım.

## Dosya Yapısı

```
LogFlow/
├── Emitter.h  Source.h  Stage.h  FileLineSource.h     # DEĞİŞMEDİ
├── LogRecord.h  ParserStage.h                         # DEĞİŞMEDİ
├── Sink.h                # DEĞİŞTİ: varsayılan open()/close() eklendi
├── Pipeline.h            # DEĞİŞTİ: tipten bağımsız çalıştırıcı + yaşam döngüsü
├── ConsoleSink.h         # DEĞİŞTİ: biçimleyici alabilir
├── main.cpp              # DEĞİŞTİ: komut satırı + builder ile kurulum
├── PipelineBuilder.h     # YENİ: akıcı, tip güvenli builder
├── PredicateFilterStage.h# YENİ: koşulu sağlamayan kaydı eler
├── Predicates.h          # YENİ: statusAtLeast, pathStartsWith, timestampBetween
├── ProjectionStage.h     # YENİ: alt küme alanlara indirger
├── FileSink.h            # YENİ: dosyaya yazar (open/close)
├── LogFields.h           # YENİ: alan adları ve metin gösterimi
├── CommandLine.h         # YENİ: argüman işleme
├── Makefile              # YENİ
├── data/access-small.log
├── experiments/OrderExperiment.cpp   # YENİ: sıralama deneyi
├── tests/
│   ├── TestHarness.h  TestSupport.h  CollectingEmitter.h
│   ├── ParserStageTests.cpp                          # v2
│   ├── PredicateFilterStageTests.cpp                 # YENİ
│   ├── ProjectionStageTests.cpp                      # YENİ
│   ├── FileSinkTests.cpp                             # YENİ
│   ├── PipelineBuilderTests.cpp                      # YENİ
│   ├── CommandLineTests.cpp                          # YENİ
│   ├── IntegrationTest.cpp                           # YENİ
│   └── fixtures/mini.log                             # YENİ: test verisi
├── CHANGELOG.md
└── ARCHITECTURE.md       # mimari + sıralama deneyi sonuçları
```

## Derleme ve Çalıştırma

```
make                 # logflow programını derler
make test            # tüm testleri derler ve çalıştırır (proje kökünden!)
make experiment      # filtre sırası deneyini çalıştırır
```

Make olmadan: `g++ -std=c++11 -Wall -Wextra -o logflow main.cpp`.
Her test ayrı bir programdır (`g++ -std=c++11 -o t tests/FileSinkTests.cpp`);
`IntegrationTest` fixture'a göreli yoldan ulaşır, bu yüzden proje kökünden çalıştırılır
(ya da yol argüman olarak verilir: `./IntegrationTest tests/fixtures/mini.log`).
**Dev-C++:** Compiler Options'a `-std=c++11` ekleyin; her `.cpp` ayrı projedir.

## Pipeline Nasıl Kurulur?

```cpp
FileLineSource source("access.log");
ParserStage parser;
PredicateFilterStage errors(Predicates::statusAtLeast(400));
ProjectionStage projection(fields);        // fields: std::vector<std::string>
FileSink sink("out.tsv", LogFields::tabSeparated(fields));

Pipeline p = PipelineBuilder::from(source)
                 .via(parser)
                 .via(errors)
                 .via(projection)
                 .to(sink);
p.run();                                   // aşama nesneleri run() bitene kadar yaşamalı
```

`via()` tipleri derleme zamanında denetler: yanlış tipte bir aşama eklemek derlenmez.
Ayrıntı: `ARCHITECTURE.md`.

## Testler

7 test programı, **142 doğrulama, hepsi geçiyor** (v2'nin 37 doğrulaması dahil):
her yeni aşama için ayrı birim testleri, `PipelineBuilder`/yaşam döngüsü testleri,
komut satırı testleri ve fixture üzerinde 3 aşamalı hattı çalıştırıp **çıktı dosyasının
içeriğini doğrulayan** entegrasyon testi.

## Sıralama Deneyi

Aynı veri, filtre aşamaları iki sırada: sonuç **aynı**, iş miktarı **farklı**.
Tam sonuçlar ve yorum: `ARCHITECTURE.md` → "Sıralama Deneyi".
