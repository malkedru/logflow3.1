#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "LogFields.h"
#include <stdexcept>
#include <string>
#include <vector>

// Hatalı komut satırı kullanımı.
class UsageError : public std::runtime_error {
public:
    explicit UsageError(const std::string& message) : std::runtime_error(message) {}
};

// Komut satırından okunan ayarlar. Filtre/projeksiyon ayarları isteğe bağlıdır:
// verilmeyen ayar için ilgili aşama hatta hiç eklenmez.
struct Options {
    std::string input;
    std::string output;                   // boşsa ekrana yazılır
    bool help;
    bool hasMinStatus;
    int minStatus;
    bool hasPathPrefix;
    std::string pathPrefix;
    std::vector<std::string> fields;      // boşsa tüm alanlar (projeksiyon yok)

    Options() : help(false), hasMinStatus(false), minStatus(0), hasPathPrefix(false) {}
};

class CommandLine {
private:
    static std::string takeValue(const std::vector<std::string>& args, size_t& i) {
        if (i + 1 >= args.size())
            throw UsageError(args[i] + " için bir değer gerekli");
        return args[++i];
    }

    static int parseStatus(const std::string& text) {
        if (text.size() != 3 || text.find_first_not_of("0123456789") != std::string::npos)
            throw UsageError("--min-status 100-599 arasında bir sayı olmalı: " + text);
        int value = (text[0] - '0') * 100 + (text[1] - '0') * 10 + (text[2] - '0');
        if (value < 100 || value > 599)
            throw UsageError("--min-status 100-599 arasında bir sayı olmalı: " + text);
        return value;
    }

    static std::vector<std::string> parseFields(const std::string& text) {
        std::vector<std::string> fields;
        size_t start = 0;
        while (true) {
            size_t comma = text.find(',', start);
            std::string name = text.substr(start, comma == std::string::npos
                                                      ? std::string::npos : comma - start);
            if (!LogFields::isKnown(name))
                throw UsageError("--fields: bilinmeyen alan: '" + name + "'");
            fields.push_back(name);
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        return fields;
    }

public:
    static std::string usage() {
        std::string text =
            "Kullanım: logflow <girdi.log> [seçenekler]\n"
            "\n"
            "Seçenekler:\n"
            "  --min-status N        yalnızca durum kodu >= N olan kayıtlar (100-599)\n"
            "  --path-prefix ÖNEK    yalnızca yolu ÖNEK ile başlayan kayıtlar\n"
            "  --fields a,b,c        yalnızca bu alanlar yazılır (sırayla, TAB ile ayrılmış)\n"
            "  --output DOSYA        sonucu DOSYA'ya yazar (verilmezse ekrana)\n"
            "  --help                bu yardımı gösterir\n"
            "\n"
            "Alanlar: ";
        const std::vector<std::string>& names = LogFields::names();
        for (size_t i = 0; i < names.size(); ++i)
            text += (i ? ", " : "") + names[i];
        text += "\n\nÖrnek:\n"
                "  logflow data/access-small.log --min-status 400 --path-prefix /api"
                " --fields timestamp,clientIp,status --output out.tsv\n";
        return text;
    }

    // argv[0] program adıdır; atlanır.
    static Options parse(int argc, const char* const argv[]) {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) args.push_back(argv[i]);

        Options opt;
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") {
                opt.help = true;
            } else if (arg == "--min-status") {
                opt.minStatus = parseStatus(takeValue(args, i));
                opt.hasMinStatus = true;
            } else if (arg == "--path-prefix") {
                opt.pathPrefix = takeValue(args, i);
                opt.hasPathPrefix = true;
            } else if (arg == "--fields") {
                opt.fields = parseFields(takeValue(args, i));
            } else if (arg == "--output") {
                opt.output = takeValue(args, i);
            } else if (arg.compare(0, 2, "--") == 0) {
                throw UsageError("Bilinmeyen seçenek: " + arg);
            } else if (opt.input.empty()) {
                opt.input = arg;
            } else {
                throw UsageError("Fazladan argüman: " + arg);
            }
        }

        if (!opt.help && opt.input.empty())
            throw UsageError("Girdi dosyası belirtilmedi");
        return opt;
    }
};

#endif
