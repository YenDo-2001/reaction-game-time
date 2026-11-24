// Owner: Member 3.
#include "software_i2c.hpp"
namespace reaction {
software_i2c::software_i2c(lab1::output_pin& scl, lab1::output_pin& sda,
    lab2::steady_clock& clock, settings const& config)
  : scl_(scl), sda_(sda), clock_(clock), settings_(config) { (void)recover(); }

void software_i2c::pause() {
  auto t = clock_.uptime();
  while (clock_.uptime() - t < half_ticks_) {}
}
bool software_i2c::high() {
  scl_.level(true);
  auto t = clock_.uptime();
  while (!scl_.level()) if (clock_.uptime() - t > clock_.frequency() / 500) return false;
  pause();
  return true;
}
bool software_i2c::bit(bool out, bool* in) {
  scl_.level(false); sda_.level(out); pause();
  if (!high()) return false;
  if (in) *in = sda_.level();
  scl_.level(false);
  return true;
}
bool software_i2c::start() {
  sda_.level(true);
  if (!high() || !sda_.level()) return false;
  sda_.level(false); pause(); scl_.level(false);
  return true;
}
void software_i2c::stop() {
  scl_.level(false); sda_.level(false); pause();
  (void)high(); sda_.level(true); pause();
}
bool software_i2c::recover() {
  ready_ = false;
  if (settings_.clock_rate < 1000 || settings_.clock_rate > 100000) return false;
  half_ticks_ = clock_.frequency() / (2 * settings_.clock_rate);
  scl_.level(true); sda_.level(true);
  if (!scl_.configure({lab1::pin_resistor::none, true}) ||
      !sda_.configure({lab1::pin_resistor::none, true})) return false;
  for (int n = 0; n < 9; ++n) if (!bit(true)) return false;
  stop();
  return ready_ = scl_.level() && sda_.level();
}
bool software_i2c::write(std::uint8_t b) {
  for (int n = 0; n < 8; ++n, b <<= 1) if (!bit((b & 0x80) != 0)) return false;
  bool nack = true;
  return bit(true, &nack) && !nack;
}
bool software_i2c::read(std::uint8_t& b, bool ack) {
  b = 0;
  for (int n = 0; n < 8; ++n) { bool v = false; if (!bit(true, &v)) return false; b = std::uint8_t((b << 1) | v); }
  if (!bit(!ack)) return false;
  sda_.level(true);
  return true;
}
bool software_i2c::driver_transaction(std::uint8_t address,
    std::span<std::uint8_t const> out, std::span<std::uint8_t> in) {
  if (!ready_ || address > 127 || out.size() > 128 || in.size() > 128) return false;
  bool ok = start();
  if (ok && (!out.empty() || in.empty())) {
    ok = write(std::uint8_t(address << 1));
    for (auto b : out) if (!(ok = ok && write(b))) break;
  }
  if (ok && !in.empty()) {
    if (!out.empty()) { scl_.level(false); sda_.level(true); pause(); ok = start(); }
    if (ok) ok = write(std::uint8_t((address << 1) | 1));
    for (std::size_t i = 0; ok && i < in.size(); ++i) ok = read(in[i], i + 1 < in.size());
  }
  stop();
  ready_ = scl_.level() && sda_.level();
  return ok && ready_;
}
}
