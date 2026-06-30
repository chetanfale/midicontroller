# MIDI Controller — 4 Bank, 4 Knob, 4 Button

An Arduino-based 4-bank MIDI controller with 4 MIDI CC knobs and 4 push buttons, fully customizable and mappable to whatever the user needs. At full capacity, the system can handle 16 knobs and 16 ON/OFF MIDI signals across its 4 banks, plus one general-purpose trigger pad.

## Hardware

- Arduino Uno R3
- 4x 10K linear potentiometers
- 5x LEDs w/ 220Ω resistors
- 6x push buttons
- 1 breadboard
- A lot of jumper wires

## How It Works


The system relies on the "Learn MIDI" / "Pickup (Soft Takeover)" functionality available in most modern DAWs, including FL Studio.

1. **Bank select** — pressing a bank button (no shift) latches that bank as active and lights its LED. Each bank exposes its own 4 knobs and 4 buttons.
2. **Shift layer** — holding the shift button turns the 4 bank buttons into note-trigger pads instead of bank switches, giving access to 16 distinct trigger pads total (4 banks × 4 buttons).
3. **General pad** — a dedicated trigger separate from the banks, doubling as a second note when combined with shift.
4. **Knob mapping** — turning a knob sends a CC message scaled from the potentiometer's raw 0–1023 reading down to MIDI's 0–127 range, only when the value actually changes.
5. **Learn mode in the DAW** — in FL Studio (or any DAW), you right-click a parameter, set it to "Learn MIDI," then move the corresponding knob or press the corresponding button on the controller. The DAW binds that MIDI CC/note to the parameter automatically.
6. **Serial → MIDI bridge** — the Arduino only speaks over serial, so to get its signals into a DAW you need a serial-to-MIDI bridge. I used **Hairless MIDI Serial Bridge** for this.
7. **Virtual MIDI port** — since there's no physical MIDI port to send to, Hairless needs a virtual MIDI port to write to. I used **loopMIDI** to create one.
8. **DAW visibility** — that loopMIDI port then shows up as a regular input device in the DAW's MIDI settings, ready to be assigned to anything.

## Tradeoffs

| Pros | Tradeoffs |
|---|---|
| Real-time MIDI control | Jitter during the prototyping stage |
| Barely any noticeable latency | Requires Hairless + loopMIDI running in the background |
| Supports up to 16 knobs and 16 buttons | Industrial-grade components made early testing physically tiring |
| Cheap to build | |

## Lessons Learned

- **Pot jitter** — unconnected analog pins floating plus too-sensitive thresholds caused constant false MIDI spam. Fixed with a raw-value gated hysteresis: only send when the scaled value actually changes.
- **Random shutdowns** — a single jumper wire placed one row off on the breadboard was silently bridging 5V to GND, heating the rail and crashing the board. Classic breadboard short.
- **Industrial-grade pots were a mistake** — premium feel, but too stiff for quick, expressive tweaking.
- **Push buttons too heavy** — clean looking, but need more force than feels natural for fast playing. MPC-style pads or rubber caps would fix this.

## Backstory

My MIDI keyboard has no knobs, and tweaking parameters with a mouse inside FL Studio's VST windows never felt right — it requires the window to stay open, offers no simultaneous control across multiple VSTs, and relies on the mouse instead of the tactile feel of turning a physical knob. I wanted something closer to live performance gear like the SP-404, so I built this as a general-purpose controller instead of locking it to one specific workflow.

## Roadmap

1. Build a dedicated VST or Patcher preset that auto-maps the controller to SP-404-style effects — something genuinely useful for hip-hop producers out of the box.
2. Move this off the breadboard onto perfboard or a proper PCB, both for reliability and to stop fighting loose connections.
3. Write a small script (or driver) that auto-launches loopMIDI and Hairless the moment the controller is plugged in, instead of doing it manually every session.

## Repo Structure

```
├── controller.ino          # Arduino sketch
├── tools/                  # Hairless MIDI + loopMIDI (or links if redistribution isn't allowed)
├── diagram/                # Wiring / connection diagram
└── demo/                   # Link to working demo video on YouTube
```
