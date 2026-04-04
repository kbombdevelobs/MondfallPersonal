#ifndef WEAPON_H
#define WEAPON_H

#include "raylib.h"
#include "raymath.h"

typedef enum {
    WEAPON_MOND_MP40,
    WEAPON_RAKETENFAUST,
    WEAPON_JACKHAMMER,
    WEAPON_COUNT
} WeaponType;

typedef struct { Vector3 start, end; Color color; float life; } BeamTrail;

typedef struct {
    Vector3 position;
    float timer;      // counts up
    float duration;    // total life
    float radius;      // max radius
    bool active;
} Explosion;

#define MAX_BEAM_TRAILS 20
#define MAX_PROJECTILES 30
#define MAX_EXPLOSIONS 10

typedef struct {
    Vector3 position, velocity;
    float damage, radius, life;
    bool active;
    Color color;
} Projectile;

typedef struct {
    WeaponType current;
    int mp40Mag, mp40MagSize, mp40Reserve;
    float mp40FireRate, mp40Timer, mp40Damage;
    int raketenMag, raketenMagSize, raketenReserve;
    float raketenFireRate, raketenTimer, raketenDamage, raketenRadius;
    bool raketenFiring;       // currently firing the beam
    float raketenBeamTimer;   // how long beam has been active
    float raketenBeamDuration;// total beam time (2s)
    float raketenCooldown;    // cooldown after firing
    Vector3 raketenBeamStart; // beam origin (world)
    Vector3 raketenBeamEnd;   // beam endpoint (world)
    float jackhammerDamage, jackhammerRange, jackhammerFireRate, jackhammerTimer, jackhammerAnim;
    bool reloading;
    float reloadTimer, reloadDuration;
    float weaponBob, weaponBobTimer, muzzleFlash, recoil;
    BeamTrail beams[MAX_BEAM_TRAILS];
    int beamCount;
    Projectile projectiles[MAX_PROJECTILES];
    Explosion explosions[MAX_EXPLOSIONS];
    // Sound
    Sound sndExplosion;
    Sound sndMp40Fire, sndRaketenFire, sndJackhammerHit, sndReload, sndEmpty;
    bool soundLoaded;
} Weapon;

void WeaponInit(Weapon *w);
void WeaponUnload(Weapon *w);
void WeaponUpdate(Weapon *w, float dt);
bool WeaponFire(Weapon *w, Vector3 origin, Vector3 direction);
void WeaponReload(Weapon *w);
void WeaponSwitch(Weapon *w, WeaponType type);
void WeaponDrawFirst(Weapon *w, Camera3D camera);
void WeaponDrawWorld(Weapon *w);
void WeaponSpawnExplosion(Weapon *w, Vector3 pos, float radius);
Vector3 WeaponGetBarrelWorldPos(Weapon *w, Camera3D camera);
const char *WeaponGetName(Weapon *w);
int WeaponGetAmmo(Weapon *w);
int WeaponGetMaxAmmo(Weapon *w);
int WeaponGetReserve(Weapon *w);
bool WeaponIsReloading(Weapon *w);
float WeaponReloadProgress(Weapon *w);

#endif
