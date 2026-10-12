#pragma once
#include <stdint.h>

enum class Gesture { None, Tap, Hold };

// ESP32 touch values fall when touched. Hysteresis and debounce prevent
// one physical press from becoming multiple gestures. Time math wraps safely.
class TouchInput {
 public:
  void begin(uint16_t baseline) {
    baseline_ = baseline > 5 ? baseline : 5;
    down_ = candidate_ = held_ = false;
    candidateAt_ = pressedAt_ = 0;
  }

  Gesture update(uint16_t raw, uint32_t now) {
    bool detected = raw < baseline_ * (down_ ? 0.88f : 0.78f);
    if (detected != candidate_) {
      candidate_ = detected;
      candidateAt_ = now;
    }
    Gesture event = Gesture::None;
    if (candidate_ != down_ && uint32_t(now - candidateAt_) >= 30) {
      down_ = candidate_;
      if (down_) {
        pressedAt_ = candidateAt_;
        held_ = false;
      } else if (!held_) {
        // Use physical release time, excluding release debounce.
        uint32_t duration = candidateAt_ - pressedAt_;
        if (duration >= 900) event = Gesture::Hold;
        else if (duration >= 50) event = Gesture::Tap;
      }
    }
    // Do not fire a hold after the user has already released the pad.
    if (down_ && candidate_ && !held_ && uint32_t(now - pressedAt_) >= 900) {
      held_ = true;
      event = Gesture::Hold;
    }
    if (!down_ && !candidate_) {
      // Recover quickly if a finger was present during initial calibration;
      // otherwise follow slow environmental drift only while untouched.
      float gain = raw > baseline_ ? 0.04f : 0.001f;
      baseline_ += (raw - baseline_) * gain;
      if (baseline_ < 5) baseline_ = 5;
    }
    return event;
  }

  bool down() const { return down_; }
  bool activelyDown() const { return down_ && candidate_; }
  uint32_t pressedAt() const { return pressedAt_; }
  uint16_t baseline() const { return uint16_t(baseline_); }

 private:
  float baseline_ = 100;
  bool down_ = false, candidate_ = false, held_ = false;
  uint32_t candidateAt_ = 0, pressedAt_ = 0;
};
