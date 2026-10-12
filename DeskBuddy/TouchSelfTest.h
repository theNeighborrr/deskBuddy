#pragma once
#include "TouchInput.h"

// Run the actual gesture logic against noise, boundary timing and rollover.
// These tests do not substitute for trying the physical pad.
inline unsigned touchSelfTest() {
  unsigned failures = 0;
  TouchInput t;
  auto check = [&](Gesture got, Gesture expected) { if (got != expected) ++failures; };
  t.begin(100);
  check(t.update(30, 100), Gesture::None);
  check(t.update(100, 110), Gesture::None); // 10 ms glitch
  check(t.update(100, 150), Gesture::None);
  check(t.update(30, 200), Gesture::None);
  check(t.update(30, 240), Gesture::None);
  check(t.update(100, 320), Gesture::None);
  check(t.update(100, 360), Gesture::Tap);
  check(t.update(100, 400), Gesture::None);

  t.begin(100);
  check(t.update(30, 100), Gesture::None);
  check(t.update(30, 140), Gesture::None);
  check(t.update(30, 1000), Gesture::Hold);
  check(t.update(30, 3000), Gesture::None); // one hold per press
  check(t.update(100, 3100), Gesture::None);
  check(t.update(100, 3140), Gesture::None); // no tap following hold

  t.begin(100);
  t.update(30, 100); t.update(30, 140);
  check(t.update(100, 990), Gesture::None); // release just before hold threshold
  check(t.update(100, 1030), Gesture::Tap);

  t.begin(100);
  t.update(30, 0xfffffff0U); t.update(30, 0x20);
  t.update(100, 0x60);
  check(t.update(100, 0x90), Gesture::Tap); // millis rollover

  t.begin(100);
  t.update(30, 100); t.update(30, 140);
  check(t.update(83, 200), Gesture::None); // hysteresis keeps press active
  if (!t.down()) ++failures;
  t.update(100, 250);
  if (t.activelyDown()) ++failures; // long setup hold must stop at physical release
  check(t.update(100, 290), Gesture::Tap);

  t.begin(30); // finger present at boot: releasing restores idle baseline
  for (uint32_t i = 0; i < 200; ++i) check(t.update(100, i * 10), Gesture::None);
  if (t.baseline() < 95) ++failures;
  return failures;
}
