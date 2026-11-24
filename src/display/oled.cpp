// Owner: Member 3.
#include "oled.hpp"
#include <cstring>
namespace reaction {
namespace {
struct glyph { char c; std::uint8_t col[5]; };
constexpr glyph font[] = {
  {' ',{0,0,0,0,0}}, {'!',{0,0,95,0,0}}, {'-',{8,8,8,8,8}}, {'.',{0,96,96,0,0}}, {'/',{32,16,8,4,2}},
  {'0',{62,81,73,69,62}}, {'1',{0,66,127,64,0}}, {'2',{66,97,81,73,70}}, {'3',{33,65,69,75,49}},
  {'4',{24,20,18,127,16}}, {'5',{39,69,69,69,57}}, {'6',{60,74,73,73,48}}, {'7',{1,113,9,5,3}},
  {'8',{54,73,73,73,54}}, {'9',{6,73,73,41,30}}, {'A',{126,17,17,17,126}}, {'B',{127,73,73,73,54}},
  {'C',{62,65,65,65,34}}, {'D',{127,65,65,34,28}}, {'E',{127,73,73,73,65}}, {'F',{127,9,9,9,1}},
  {'G',{62,65,73,73,122}}, {'H',{127,8,8,8,127}}, {'I',{0,65,127,65,0}}, {'J',{32,64,65,63,1}},
  {'K',{127,8,20,34,65}}, {'L',{127,64,64,64,64}}, {'M',{127,2,12,2,127}}, {'N',{127,4,8,16,127}},
  {'O',{62,65,65,65,62}}, {'P',{127,9,9,9,6}}, {'Q',{62,65,81,33,94}}, {'R',{127,9,25,41,70}},
  {'S',{70,73,73,73,49}}, {'T',{1,1,127,1,1}}, {'U',{63,64,64,64,63}}, {'V',{31,32,64,32,31}},
  {'W',{63,64,56,64,63}}, {'X',{99,20,8,20,99}}, {'Y',{7,8,112,8,7}}, {'Z',{97,81,73,69,67}}
};
}
oled::oled(lab4::i2c& bus) : bus_(bus) { (void)recover(); }
void oled::clear() { std::memset(pixels_, 0, sizeof(pixels_)); }
bool oled::power(bool on) { std::uint8_t c = on ? 0xaf : 0xae; return commands({&c, 1}); }
bool oled::commands(std::span<std::uint8_t const> bytes) {
  if (bytes.size() > 31) return false;
  std::uint8_t packet[32] = {0x00};
  std::memcpy(packet + 1, bytes.data(), bytes.size());
  bool ok = bus_.transaction(address_, {packet, bytes.size() + 1}, {});
  if (!ok) ready_ = false;
  return ok;
}
bool oled::recover() {
  ready_ = false; address_ = 0x3c;
  if (!power(false)) { address_ = 0x3d; if (!power(false)) return false; }
  constexpr std::uint8_t init[] = {0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,
    0x8d,0x14,0x20,0x02,0xa1,0xc8,0xda,0x12,0x81,0x7f,0xd9,0xf1,0xdb,0x40,0xa4,0xa6};
  if (!commands(init)) return false;
  clear();
  return ready_ = flush() && power(true);
}
void oled::text(unsigned x, unsigned y, char const* s, unsigned scale) {
  for (; *s; ++s, x += 6 * scale) {
    auto const* g = &font[0];
    for (auto const& f : font) if (f.c == *s) { g = &f; break; }
    for (unsigned c = 0; c < 5; ++c) for (unsigned r = 0; r < 7; ++r)
      if (g->col[c] & (1U << r))
        for (unsigned dx = 0; dx < scale; ++dx) for (unsigned dy = 0; dy < scale; ++dy) {
          unsigned px = x + c * scale + dx, py = y + r * scale + dy;
          if (px < 128 && py < 64) pixels_[px + (py / 8) * 128] |= std::uint8_t(1U << (py % 8));
        }
  }
}
bool oled::flush() {
  for (unsigned page = 0; page < 8; ++page) {
    std::uint8_t pos[] = {std::uint8_t(0xb0 + page), 0x00, 0x10};
    if (!commands(pos)) return false;
    for (unsigned x = 0; x < 128; x += 64) {
      std::uint8_t packet[65]; packet[0] = 0x40;
      std::memcpy(packet + 1, pixels_ + page * 128 + x, 64);
      if (!bus_.transaction(address_, packet, {})) { ready_ = false; return false; }
    }
  }
  return true;
}
}
