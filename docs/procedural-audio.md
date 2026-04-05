---
title: Procedural Audio
status: Active
owner_area: Audio
created: 2026-04-04
last_updated: 2026-04-04
last_reviewed: 2026-04-04
parent_doc: CLAUDE.md
related_docs:
  - src/weapon/CLAUDE.md
  - docs/rendering-pipeline.md
---

# Procedural Audio

All core game audio is procedurally generated from waveforms at runtime. No audio asset files are required for the base game (enemy death sounds are the exception, loaded from mp3 files).

## Sound Generation Utilities (sound_gen.c/h)

Three foundational functions used across the codebase:

### SoundGenCreateWave(sampleRate, duration)
Allocates a 16-bit mono `Wave` struct with the specified sample rate and duration. Returns zeroed PCM data ready for filling. All sound generators use this as their starting point.

### SoundGenNoiseTone(duration, freq, decay)
Generates a mixed noise + sine tone with exponential decay envelope. Produces a sound combining 60% white noise with 40% tonal content. Used as a building block for gunshot and impact sounds.

### SoundGenDegradeRadio(wav)
Degrades an existing Wave to scratchy radio quality through:
- High-pass filter (removes bass via running average subtraction)
- Overdrive clipping (amplify then hard-clip at configurable threshold)
- White noise crackle overlay
- Random pop artifacts (configurable chance and amplitude)

Applied to enemy death sound mp3 files after loading and downsampling to 8kHz.

## Weapon Sounds (weapon/weapon_sound.c/h)

Six procedural sound generators, each returning a loaded `Sound`:

| Function | Sound | Technique |
|----------|-------|-----------|
| `GenSoundGunshot()` | MP40 gunfire | `SoundGenNoiseTone` at 120Hz, 25.0 decay, 0.15s |
| `GenSoundBeamBlast()` | Raketenfaust death beam | 2.5s sustained: 55Hz rumble + 280Hz FM screech + noise sizzle, pulsing envelope |
| `GenSoundJackhammer()` | Pneumatic melee strike | 0.35s: 80Hz impact thud + noise crunch + rising-pitch servo whine + pneumatic hiss |
| `GenSoundReload()` | Magazine reload | 0.5s: two metallic clicks (3kHz, 2kHz sine bursts) + slide noise |
| `GenSoundExplosion()` | Projectile explosion | 0.6s: 50Hz boom + noise + 25Hz rumble, all with exponential decay |
| `GenSoundEmpty()` | Empty mag click | 0.05s: 4kHz sine with rapid 80x decay |

## Erika March Music (audio.c/h)

The background march is synthesized from MIDI note data at startup:

1. A melody array defines frequency/duration pairs in eighth-note units, covering verse, bridge, chorus, and return sections
2. At 240 BPM (0.125s per eighth note), the full march is rendered to a 22050Hz Wave
3. Synthesis layers:
   - **Brass**: asymmetric square wave (42% duty cycle) + 3rd and 5th harmonic overtones, with per-note attack/decay envelope
   - **Snare**: noise bursts on beats 2 and 4
   - **Bass drum**: 80Hz sine burst on beats 1 and 3
   - **Radio static**: constant low-level white noise + occasional crackle pops
4. Mixed and hard-clipped at 95%
5. Exported to `sounds/march.wav`, then loaded as a looping `MusicStream`

## Enemy Death Sounds

Loaded from mp3 files in `sounds/soviet_death_sounds/` and `sounds/american_death_sounds/` during `EcsEnemyResourcesInit()`:

1. Load mp3 via raylib's `LoadWave()`
2. Downsample to 8kHz mono via `WaveFormat()`
3. Apply `SoundGenDegradeRadio()` for scratchy radio quality
4. Load as `Sound` with reduced volume (`AUDIO_DEATH_VOLUME`)

Death sounds play through a radio transmission system: only one plays at a time, a configurable chance gate controls frequency, and each faction has a play count limit (3 per game) to avoid repetition.

## Lander Sounds

Procedurally generated in `LanderManagerInit()`:

| Sound | Technique |
|-------|-----------|
| Impact thud | 40Hz sine + noise, exponential decay over 0.8s |
| Explosion | 55Hz sine + noise, exponential decay over 1.0s |
| Air raid klaxon | 3-second sawtooth siren sweeping 100-300Hz, 3 wail cycles with 2nd harmonic |

## Audio Configuration

Key constants from `config.h`:
- `AUDIO_SAMPLE_RATE` -- standard sample rate for procedural sounds
- `AUDIO_RADIO_SAMPLE_RATE` -- 8kHz target for radio-degraded sounds
- `AUDIO_RADIO_OVERDRIVE`, `AUDIO_RADIO_CLIP`, `AUDIO_RADIO_CRACKLE` -- radio degradation parameters
- `AUDIO_RADIO_POP_CHANCE`, `AUDIO_RADIO_POP_AMP` -- random pop artifact settings
- `AUDIO_MARCH_VOLUME` -- default music volume
- `AUDIO_DEATH_VOLUME` -- death sound volume multiplier

## Key Files

| File | Role |
|------|------|
| `src/sound_gen.c/h` | Core wave creation, noise/tone generator, radio degradation |
| `src/weapon/weapon_sound.c/h` | Six weapon sound generators |
| `src/audio.c/h` | Erika march synthesis, music stream management |
| `src/enemy/enemy_draw_ecs.c` | Death sound loading and radio degradation |
| `src/lander.c` | Lander sound generation (impact, explosion, klaxon) |
| `src/config.h` | All audio tuning constants |
