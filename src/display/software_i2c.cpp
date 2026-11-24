// Owner: Member 3.
#include "software_i2c.hpp"
namespace reaction {
software_i2c::software_i2c(lab1::output_pin& scl, lab1::output_pin& sda,
    lab2::steady_clock& clock, settings const& config)
  : scl_(scl), sda_(sda), clock_(clock), settings_(config) {
  ready_ = configure_bus(settings_);
}
bool software_i2c::recover() {
  ready_ = configure_bus(settings_);
  return ready_;
}
bool software_i2c::driver_configure(settings const& config) {
  settings_ = config;
  return recover();
}
void software_i2c::pause() {
  auto t = clock_.uptime();
  while (clock_.uptime() - t < half_ticks_) {}
}
bool software_i2c::high() {
  scl_.level(true);
  auto t = clock_.uptime();
  while (!scl_.level()) {
    if (clock_.uptime() - t > clock_.frequency() / 500) return false;
  }
  pause(); return true;
}
bool software_i2c::start() {
  sda_.level(true);
  if (!high() || !sda_.level()) return false;
  sda_.level(false); pause(); scl_.level(false); return true;
}
void software_i2c::stop() {
  scl_.level(false); sda_.level(false); pause();
  (void)high(); sda_.level(true); pause();
}
bool software_i2c::configure_bus(settings const& s) {
  if (s.clock_rate < 1000 || s.clock_rate > 100000) return false;
  half_ticks_ = clock_.frequency() / (2 * s.clock_rate);
  // Configure latches high before enabling open-drain outputs.
  scl_.level(true); sda_.level(true);
  if (!scl_.configure({lab1::pin_resistor::none,true}) ||
      !sda_.configure({lab1::pin_resistor::none,true})) return false;
  sda_.level(true);
  // Release a slave left part-way through a byte by a prior MCU reset.
  for (int n=0; n<9; ++n) { scl_.level(false); pause(); if (!high()) return false; }
  stop(); return scl_.level() && sda_.level();
}
bool software_i2c::write(std::uint8_t b) {
  for (int n=0; n<8; ++n) {
    scl_.level(false); sda_.level((b & 0x80)!=0); pause();
    if (!high()) return false;
    scl_.level(false); b <<= 1;
  }
  sda_.level(true); pause(); if (!high()) return false;
  bool ack = !sda_.level(); scl_.level(false); return ack;
}
bool software_i2c::read(std::uint8_t& b, bool ack) {
  b=0; sda_.level(true);
  for (int n=0;n<8;++n) {
    pause(); if (!high()) return false;
    b=std::uint8_t((b<<1) | (sda_.level()?1:0)); scl_.level(false);
  }
  sda_.level(!ack); pause(); if (!high()) return false;
  scl_.level(false); sda_.level(true); return true;
}
bool software_i2c::driver_transaction(std::uint8_t address,
    std::span<std::uint8_t const> out, std::span<std::uint8_t> in) {
  if (!ready_ || address>127 || out.size()>128 || in.size()>128) return false;
  bool ok=start();
  if (ok && (out.size()!=0 || in.size()==0)) {
    ok=write(std::uint8_t(address<<1));
    for (auto b:out) { if (!ok) break; ok=write(b); }
  }
  if (ok && in.size()!=0) {
    if (out.size()!=0) { scl_.level(false); sda_.level(true); pause(); ok=start(); }
    if (ok) ok=write(std::uint8_t((address<<1)|1));
    for (std::size_t i=0;ok && i<in.size();++i) ok=read(in[i],i+1<in.size());
  }
  stop();
  // A slave NACK is not a bus fault (e.g. OLED may use address 0x3D).
  ready_=scl_.level() && sda_.level();
  return ok && ready_;
}
}
