#pragma once
#include <map>
#include <string>
#include <memory>
#include <stdint.h>
#include <string.h>
#include "SPI.h"
#define FILE_WRITE "w"
#define FILE_APPEND "a"
constexpr int CARD_NONE=0;
namespace FakeSD {
extern bool tarjeta, parcial;
extern unsigned montajes;
extern uint64_t libre;
extern std::map<std::string,std::string> archivos;
}
struct HandleFake { std::string nombre; bool abierto; };
class File {
    std::shared_ptr<HandleFake> h;
public:
    File()=default;
    explicit File(const char* ruta) : h(new HandleFake{ruta,true}) {}
    explicit operator bool() const { return h && h->abierto; }
    size_t write(const uint8_t* p,size_t n) {
        if (!*this || !FakeSD::tarjeta) return 0;
        if (FakeSD::parcial && h->nombre.find(".csv")!=std::string::npos && n && p[0]=='1') {
            FakeSD::parcial=false; n/=2;
        }
        FakeSD::archivos[h->nombre].append(reinterpret_cast<const char*>(p),n); return n;
    }
    void flush() {}
    int getWriteError() { return 0; }
    void close() { if (h) h->abierto=false; }
};
struct SDFake {
    bool begin(int,SPIClass&,uint32_t,const char*,uint8_t,bool) { ++FakeSD::montajes; return FakeSD::tarjeta; }
    void end() {}
    int cardType() { return FakeSD::tarjeta ? 1 : CARD_NONE; }
    bool exists(const char* p) { return FakeSD::archivos.count(p); }
    File open(const char* p,const char* modo) {
        if (!FakeSD::tarjeta) return {};
        if (!strcmp(modo,"w")) FakeSD::archivos[p].clear();
        return File(p);
    }
    bool rename(const char* a,const char* b) {
        if (!FakeSD::tarjeta || !exists(a) || exists(b)) return false;
        FakeSD::archivos[b]=FakeSD::archivos.at(a); FakeSD::archivos.erase(a); return true;
    }
    uint64_t totalBytes() { return 1024*1024*1024ULL; }
    uint64_t usedBytes() { return totalBytes()-FakeSD::libre; }
};
extern SDFake SD;
