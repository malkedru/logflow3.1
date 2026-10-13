# Değişiklik Günlüğü

## v3 — Bileşim: Filtreleme, Projeksiyon ve Pipeline Builder

### Yeni dosyalar
- `PipelineBuilder.h` — akıcı, tip güvenli builder (`from().via()...to()`)
- `PredicateFilterStage.h`, `Predicates.h` — koşula göre eleme; hazır koşullar
- `ProjectionStage.h` — alanları alt kümeye indirger (ham satırı ve `attributes`'u atar)
- `FileSink.h` — dosyaya yazar; `open()`/`close()` gerçekten kullanılır
- `LogFields.h` — alan adları ve metin gösterimi (projeksiyon, CLI ve biçimleyici ortak)
- `CommandLine.h` — `--min-status`, `--path-prefix`, `--fields`, `--output`, `--help`
- `Makefile`
- `experiments/OrderExperiment.cpp` — sıralama deneyi
- `tests/`: `TestHarness.h`, `TestSupport.h`, `PredicateFilterStageTests.cpp`,
  `ProjectionStageTests.cpp`, `FileSinkTests.cpp`, `PipelineBuilderTests.cpp`,
  `CommandLineTests.cpp`, `IntegrationTest.cpp`, `fixtures/mini.log`

### Değiştirilen dosyalar
- `Pipeline.h` — `Pipeline<I,O>` (tek aşamalı şablon) yerine tipten bağımsız `Pipeline`;
  yaşam döngüsü yönetimi (sink önce açılır, en son kapanır; hata olsa da `close()` çağrılır)
- `Sink.h` — varsayılan `open()`/`close()` eklendi (`Stage` ile aynı sözleşme)
- `ConsoleSink.h` — isteğe bağlı `RecordFormatter`; zaman damgası biçimi `LogFields`'a taşındı
- `main.cpp` — komut satırı işleme ve builder ile hat kurulumu; özet stderr'e
- `README.md`, `ARCHITECTURE.md` — güncellendi; sıralama deneyi sonuçları eklendi

### Değişmeyen dosyalar
`Emitter.h`, `Source.h`, `Stage.h`, `FileLineSource.h`, `LogRecord.h`, `ParserStage.h`,
`tests/ParserStageTests.cpp`, `tests/CollectingEmitter.h` — üç yeni aşama mevcut
arayüzlere dokunmadan eklendi.

### Uyumsuz değişiklik
- `Pipeline<I,O>(&source, &stage, &sink)` kaldırıldı; yerine `PipelineBuilder` kullanılır.

### Bilinen sınırlar (sonraki haftalar)
- Hatalı satırların doğru ele alınması 5. haftada.
- Aşama listesi bu hafta komut satırından kurulur; 4. haftada yapılandırmaya taşınacak.
- Zaman aralığı koşulu (`Predicates::timestampBetween`) hazır ve testli, ancak henüz
  komut satırı seçeneği yok.
- `--fields` ile ekrana/dosyaya yazılan çıktı başlık satırı içermez (TAB ile ayrılmış).
