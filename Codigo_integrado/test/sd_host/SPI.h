#pragma once
constexpr int VSPI=1;
struct SPIClass { explicit SPIClass(int) {} void begin(int,int,int,int) {} };
