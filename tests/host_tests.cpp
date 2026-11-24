#include "../src/game/game.hpp"
#include "../src/input/button.hpp"
#include "../src/display/oled.hpp"
#include "../src/display/software_i2c.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace reaction;
struct pin final : lab1::input_pin {
  bool high=true;
  bool driver_configure(settings const&) override { return true; }
  bool driver_level() override { return high; }
};
struct bus final : lab4::i2c {
  std::vector<std::vector<std::uint8_t>> packets;
  bool fail=false;
  std::uint8_t address=0x3c;
  bool driver_configure(settings const&) override { return true; }
  bool driver_transaction(std::uint8_t a,std::span<std::uint8_t const> out,
      std::span<std::uint8_t>) override {
    if (fail || a!=address) return false;
    packets.emplace_back(out.begin(),out.end()); return true;
  }
};
struct output final : lab1::output_pin {
  bool high=true, stuck=false, reject=false;
  bool driver_configure(settings const&) override { return !reject; }
  void driver_level(bool value) override { high=value; }
  bool driver_level() override { return high && !stuck; }
};
struct clock_source final : lab2::steady_clock {
  std::uint64_t ticks=0;
  std::uint32_t driver_frequency() override { return 1'000'000; }
  std::uint64_t driver_uptime() override { return ticks++; }
};
int main() {
  pin p; button b(p); button_event e{};
  p.high=false; assert(!b.poll(100,e));
  p.high=true; assert(!b.poll(105,e));
  p.high=false; assert(!b.poll(108,e));
  assert(!b.poll(127,e)); assert(b.poll(128,e));
  assert(e.pressed && e.time_ms==108); assert(!b.poll(130,e));
  p.high=true; assert(!b.poll(140,e)); assert(b.poll(160,e)); assert(!e.pressed);
  game g; g.press(0,2000); assert(g.current.screen==state::release);
  g.release(20); assert(!g.due(2019)); assert(g.due(2020));
  g.cue_visible(2100); g.press(2345,2000);
  assert(g.current.screen==state::result && g.current.reaction_ms==245 && g.current.best_ms==245);
  g.press(3000,2000); g.release(3020); g.press(3100,2000);
  assert(g.current.screen==state::early);
  g.press(4000,2000); g.release(4020); g.cue_visible(6100); g.press(6099,2000);
  assert(g.current.screen==state::early); // Debounced edge from before GO.
  g.press(7000,2000); g.release(7020); g.cue_visible(9100);
  g.tick(19100); assert(g.current.screen==state::go);
  g.press(19099,2000); assert(g.current.screen==state::result);
  g.press(20000,2000); g.release(20020); g.cue_visible(22000);
  g.tick(32025); assert(g.current.screen==state::timeout);
  game wrap; wrap.press(0xfffffff0U,2000); wrap.release(0xfffffff5U);
  assert(wrap.due(1990)); wrap.cue_visible(0xfffffff0U); wrap.press(84,2000);
  assert(wrap.current.reaction_ms==100);
  bus wire; oled screen(wire); assert(screen.ready());
  wire.packets.clear(); screen.clear(); screen.text(0,0,"GO",3); assert(screen.flush());
  unsigned data=0; bool nonzero=false;
  for (auto const& packet:wire.packets) if (packet[0]==0x40) {
    data+=unsigned(packet.size()-1);
    for (std::size_t i=1;i<packet.size();++i) nonzero |= packet[i]!=0;
  }
  assert(data==1024 && nonzero);
  wire.fail=true; assert(!screen.flush()); assert(!screen.ready());
  assert(!screen.recover()); assert(!screen.ready());
  wire.fail=false; assert(screen.recover()); assert(screen.ready());
  bus absent; absent.fail=true; oled missing(absent); assert(!missing.ready());
  absent.fail=false; assert(missing.recover()); assert(missing.ready());
  bus alternate; alternate.address=0x3d; oled alternate_screen(alternate);
  assert(alternate_screen.ready()); // Constructor probes fallback address.
  output scl,sda; clock_source clock;
  software_i2c link(scl,sda,clock,{50000}); assert(link.ready());
  std::uint8_t command=0;
  assert(!link.transaction(0x3c,{&command,1},{})); // No slave ACK in this fake.
  assert(link.ready()); // NACK must not disable the fallback address probe.
  scl.stuck=true;
  auto before=clock.ticks;
  assert(!link.recover()); assert(!link.ready());
  assert(clock.ticks-before>=2000 && clock.ticks-before<10000);
  scl.stuck=false; assert(link.recover()); assert(link.ready());
  software_i2c invalid(scl,sda,clock,{0}); assert(!invalid.ready());
  output bad_scl; bad_scl.reject=true;
  software_i2c rejected(bad_scl,sda,clock,{50000}); assert(!rejected.ready());
  std::puts("PASS: game/debounce, constructor initialization, OLED fallback/failure/recovery, I2C NACK/stuck-line/config errors");
}
