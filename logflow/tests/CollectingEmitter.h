#ifndef COLLECTINGEMITTER_H
#define COLLECTINGEMITTER_H

#include "../Emitter.h"
#include <vector>

// Test dublörü (test double): yayılan her öğeyi bir listede toplar.
// Dosya sistemine dokunmadan herhangi bir Stage<I,O> test edilebilir;
// her hafta yeniden kullanılacaktır.
template<typename T>
class CollectingEmitter : public Emitter<T> {
public:
    std::vector<T> items;
    void emit(const T& item) override { items.push_back(item); }
};

#endif
