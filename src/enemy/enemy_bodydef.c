#include "enemy_bodydef.h"
#include "rlgl.h"
#include <math.h>

/* ============================================================================
 * Color Palettes (2 factions: Soviet, American)
 * ============================================================================ */

static const BodyColors COLORS_SOVIET = {
    .suit      = {205, 0, 0, 255},
    .suitDark  = {160, 0, 0, 255},
    .helmet    = {220, 185, 40, 255},
    .accent    = {255, 215, 0, 255},
    .boot      = {25, 25, 22, 255},
    .belt      = {120, 75, 35, 255},
    .buckle    = {255, 215, 0, 255},
    .glove     = {30, 30, 28, 255},
    .backpack  = {140, 10, 10, 255},
    .visor     = {255, 200, 50, 230},
    .hose      = {100, 30, 30, 255},
};

static const BodyColors COLORS_AMERICAN = {
    .suit      = {25, 40, 90, 255},
    .suitDark  = {18, 28, 65, 255},
    .helmet    = {195, 200, 210, 255},
    .accent    = {220, 220, 230, 255},
    .boot      = {140, 115, 75, 255},
    .belt      = {80, 85, 60, 255},
    .buckle    = {200, 205, 215, 255},
    .glove     = {160, 155, 140, 255},
    .backpack  = {30, 45, 80, 255},
    .visor     = {100, 170, 255, 230},
    .hose      = {60, 65, 90, 255},
};

/* ============================================================================
 * Animation Profiles — per-faction spring tuning and motion amplitudes
 * ============================================================================ */

/* Soviet: floppy, aggressive springs — big overshoot, heavy sway */
static const AnimProfile SOVIET_PROFILE = {
    .armSwingDeg    = 42.0f,
    .legSwingDeg    = 45.0f,
    .armBasePitch   = -30.0f,
    .armBaseSpread  = -16.0f,
    .kneeBendFactor = 1.2f,    /* BIG knee bend per stride              */

    .armSwingSpr   = {3.0f,  1.0f},
    .armSpreadSpr  = {2.5f,  0.8f},
    .legSwingSpr   = {3.5f,  0.9f},   /* looser — legs swing independently */
    .legSpreadSpr  = {2.5f,  0.8f},
    .kneeSpr       = {6.0f,  1.2f},   /* springy knees — visible bounce    */
    .hipSwaySpr    = {2.5f,  0.8f},
    .torsoLeanSpr  = {2.0f,  1.2f},
    .torsoTwistSpr = {3.5f,  1.0f},
    .headBobSpr    = {4.0f,  1.2f},

    .hipSwayAmount = 0.22f,
    .torsoLeanDeg  = 16.0f,
    .torsoTwistDeg = 14.0f,
    .headBobDeg    = 10.0f,
    .breathRate    = 0.25f,
    .breathScale   = 0.015f,
    .kneeFlexBase  = -15.0f,  /* always crouched — heavy space suit     */
};

/* American: stiffer, disciplined springs — tighter control, smaller amplitudes */
static const AnimProfile AMERICAN_PROFILE = {
    .armSwingDeg    = 30.0f,
    .legSwingDeg    = 32.0f,
    .armBasePitch   = -28.0f,
    .armBaseSpread  = -12.0f,
    .kneeBendFactor = 0.9f,    /* visible knee bend                     */

    .armSwingSpr   = {5.0f,  1.6f},
    .armSpreadSpr  = {4.0f,  1.4f},
    .legSwingSpr   = {5.0f,  1.4f},   /* moderately loose — disciplined but visible */
    .legSpreadSpr  = {4.0f,  1.4f},
    .kneeSpr       = {8.0f,  1.8f},   /* springy knees — bouncy tactical step */
    .hipSwaySpr    = {4.5f,  1.4f},
    .torsoLeanSpr  = {4.0f,  1.8f},
    .torsoTwistSpr = {5.0f,  1.6f},
    .headBobSpr    = {5.5f,  1.8f},

    .hipSwayAmount = 0.14f,
    .torsoLeanDeg  = 10.0f,
    .torsoTwistDeg = 8.0f,
    .headBobDeg    = 6.0f,
    .breathRate    = 0.20f,
    .breathScale   = 0.010f,
    .kneeFlexBase  = -10.0f,  /* slightly crouched — tactical posture    */
};

/* ============================================================================
 * Astronaut Body Definition (geometry only — animation in profiles)
 * ============================================================================ */

static const BodyDef ASTRONAUT_BODY = {
    /* --- TORSO (13 parts) --- */
    .torso = { .count = 13, .parts = {
        {BP_CUBE,      COL_SUIT,      {0},              {0,0,0},              {0.9f,1.5f,0.55f},       0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,0.2f,0.02f},       {0.7f,0.6f,0.5f},        0, false, -1},
        {BP_CUBE_WIRE, COL_SUIT_DARK, {0},              {0,0,0},              {0.91f,1.51f,0.56f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,0.72f,0},          {0.55f,0.1f,0.45f},       0, false, -1},
        {BP_CUBE,      COL_BELT,      {0},              {0,-0.55f,0},         {0.92f,0.1f,0.57f},       0, false, -1},
        {BP_CUBE_WIRE, COL_BELT,      {0},              {0,-0.55f,0},         {0.93f,0.11f,0.58f},      0, true,  -1},
        {BP_CUBE,      COL_BUCKLE,    {0},              {0,-0.55f,0.29f},     {0.12f,0.09f,0.02f},      0, false, -1},
        {BP_CUBE_WIRE, COL_BUCKLE,    {0},              {0,-0.55f,0.29f},     {0.13f,0.1f,0.03f},       0, true,  -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0.5f,0.6f,0},        {0.18f,0.2f,0.4f},        0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {-0.5f,0.6f,0},       {0.18f,0.2f,0.4f},        0, false, -1},
        {BP_CUBE,      COL_ACCENT,    {0},              {0.5f,0.62f,0.15f},   {0.15f,0.12f,0.02f},      0, false, -1},
        {BP_CUBE,      COL_ACCENT,    {0},              {-0.5f,0.62f,0.15f},  {0.15f,0.12f,0.02f},      0, false, -1},
        {BP_CUBE,      COL_ACCENT,    {0},              {0.2f,0.35f,0.28f},   {0.12f,0.12f,0.02f},      0, false, -1},
    }},

    /* --- HEAD (8 parts, positions relative to headPivotY) --- */
    .head = { .count = 8, .parts = {
        {BP_SPHERE,    COL_HELMET,    {0},              {0,0,0},              {0.48f,0,0},              0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,-0.3f,0},          {0.65f,0.08f,0.55f},      0, false, -1},
        {BP_MODEL,     COL_VISOR,     {0},              {0,0.02f,0.32f},      {0},                      MDL_VISOR, false, -1},
        {BP_CUBE_WIRE, COL_FIXED,     {160,158,152,200},{0,0.02f,0.34f},      {0.42f,0.32f,0.04f},      0, false, -1},
        {BP_SPHERE,    COL_ACCENT,    {0},              {0,0.45f,0},          {0.06f,0,0},              0, false, 0},
        {BP_CUBE,      COL_ACCENT,    {0},              {0,0.4f,0},           {0.12f,0.04f,0.04f},      0, false, 1},
        {BP_CUBE,      COL_ACCENT,    {0},              {0,0.4f,0},           {0.04f,0.12f,0.04f},      0, false, 1},
        {BP_CUBE,      COL_ACCENT,    {0},              {0.12f,0.45f,0},      {0.03f,0.12f,0.03f},      0, false, -1},
    }},

    /* --- BACKPACK (7 parts) --- */
    .backpack = { .count = 7, .parts = {
        {BP_CUBE,      COL_BACKPACK,  {0},              {0,0.1f,-0.42f},      {0.62f,0.9f,0.28f},       0, false, -1},
        {BP_CUBE_WIRE, COL_SUIT_DARK, {0},              {0,0.1f,-0.42f},      {0.63f,0.91f,0.29f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,-0.1f,-0.57f},     {0.45f,0.05f,0.01f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,0.2f,-0.57f},      {0.45f,0.05f,0.01f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,0.5f,-0.57f},      {0.45f,0.05f,0.01f},      0, false, -1},
        {BP_CUBE,      COL_HOSE,      {0},              {0.18f,0.65f,-0.25f}, {0.05f,0.5f,0.05f},       0, false, -1},
        {BP_CUBE,      COL_HOSE,      {0},              {-0.18f,0.65f,-0.25f},{0.05f,0.5f,0.05f},       0, false, -1},
    }},

    /* --- ARM (2 parts — one template, drawn mirrored) --- */
    .armParts = { .count = 2, .parts = {
        {BP_MODEL,     COL_FIXED,     {255,255,255,255},{0,0,0},              {0},                      MDL_ARM, false, -1},
        {BP_CUBE,      COL_GLOVE,     {0},              {0,-0.42f,0},         {0.24f,0.16f,0.24f},      0, false, -1},
    }},

    /* --- LEG UPPER (3 parts — one template, drawn mirrored) --- */
    .legUpper = { .count = 3, .parts = {
        {BP_CUBE,      COL_SUIT,      {0},              {0,-0.05f,0},         {0.3f,0.45f,0.3f},        0, false, -1},
        {BP_CUBE_WIRE, COL_SUIT_DARK, {0},              {0,-0.05f,0},         {0.31f,0.46f,0.31f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},              {0,-0.3f,0},          {0.22f,0.08f,0.22f},      0, false, -1},
    }},

    /* --- LEG LOWER (2 parts — shin + boot) --- */
    .legLower = { .count = 2, .parts = {
        {BP_CUBE,      COL_SUIT,      {0},              {0,-0.2f,0},          {0.28f,0.4f,0.28f},       0, false, -1},
        {BP_MODEL,     COL_BOOT,      {0},              {0,-0.44f,0.04f},     {0},                      MDL_BOOT, false, -1},
    }},

    /* --- Pivots --- */
    .armPivot      = {0.52f, 0.35f, 0.1f},
    .legPivot      = {0.22f, -0.85f, 0},
    .kneePivot     = {0, -0.3f, 0},
    .gunHandOffset = {-0.25f, -0.32f, 0.28f},
    .headPivotY    = 1.1f,
};

/* ============================================================================
 * Gun Definitions (per-faction)
 * ============================================================================ */

static const GunDef GUN_SOVIET = {
    .pivot   = {0.2f, 0.05f, 0.55f},
    .muzzleZ = 0.88f,
    .restPitchOffset = -2.0f,  /* heavy weapon, droops slightly */
    .parts   = { .count = 11, .parts = {
        {BP_CUBE,      COL_FIXED, {40,42,50,255},    {0,0,0},            {0.16f,0.14f,1.0f},      0, false, -1},
        {BP_CUBE_WIRE, COL_FIXED, {25,27,32,255},    {0,0,0},            {0.161f,0.141f,1.01f},    0, false, -1},
        {BP_CUBE,      COL_FIXED, {25,27,32,255},    {0,0.02f,0.6f},     {0.09f,0.09f,0.35f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {40,42,50,255},    {0,0.02f,0.82f},    {0.13f,0.13f,0.07f},     0, false, -1},
        {BP_CUBE_WIRE, COL_FIXED, {25,27,32,255},    {0,0.02f,0.82f},    {0.131f,0.131f,0.071f},  0, false, -1},
        {BP_SPHERE,    COL_FIXED, {40,42,50,255},    {0,-0.16f,0},       {0.13f,0,0},             0, false, -1},
        {BP_SPHERE,    COL_FIXED, {40,42,50,255},    {0,-0.16f,0.12f},   {0.11f,0,0},             0, false, -1},
        {BP_CUBE,      COL_FIXED, {255,40,20,230},   {0.085f,0,0},       {0.008f,0.04f,0.7f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {255,40,20,230},   {-0.085f,0,0},      {0.008f,0.04f,0.7f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {25,27,32,255},    {0,0,-0.4f},        {0.06f,0.12f,0.3f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {40,42,50,255},    {0,-0.02f,-0.55f},  {0.09f,0.08f,0.06f},     0, false, -1},
    }},
};

static const GunDef GUN_AMERICAN = {
    .pivot   = {0.2f, 0.05f, 0.55f},
    .muzzleZ = 0.64f,
    .restPitchOffset = 1.0f,  /* lighter weapon, held higher */
    .parts   = { .count = 10, .parts = {
        {BP_CUBE,      COL_FIXED, {30,35,50,255},     {0,0,0},            {0.14f,0.14f,0.8f},      0, false, -1},
        {BP_CUBE_WIRE, COL_FIXED, {20,23,35,255},     {0,0,0},            {0.141f,0.141f,0.801f},  0, false, -1},
        {BP_CUBE,      COL_FIXED, {30,35,50,255},     {0,0,0.48f},        {0.18f,0.18f,0.12f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {20,23,35,255},     {0,0,0.58f},        {0.26f,0.26f,0.05f},     0, false, -1},
        {BP_CUBE_WIRE, COL_FIXED, {40,80,180,150},    {0,0,0.58f},        {0.261f,0.261f,0.051f},  0, false, -1},
        {BP_CUBE,      COL_FIXED, {50,130,255,230},   {0,0.12f,-0.05f},   {0.08f,0.08f,0.08f},     0, false, -1},
        {BP_SPHERE,    COL_FIXED, {120,200,255,200},  {0,0.12f,-0.05f},   {0.05f,0,0},             0, false, -1},
        {BP_CUBE,      COL_FIXED, {50,130,255,230},   {0.075f,0,0},       {0.008f,0.03f,0.6f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {50,130,255,230},   {-0.075f,0,0},      {0.008f,0.03f,0.6f},     0, false, -1},
        {BP_CUBE,      COL_FIXED, {20,23,35,255},     {0,0,-0.35f},       {0.08f,0.1f,0.15f},      0, false, -1},
    }},
};

/* ============================================================================
 * Rank Overlays (Soviet + American only, NCO and Officer)
 * ============================================================================ */

static const RankOverlayDef NCO_SOVIET = {
    .parts = { .count = 12, .parts = {
        {BP_SPHERE,    COL_FIXED, {90,60,35,255},     {0,1.7f,0},          {0.38f,0,0},              0, false, -1},
        {BP_CUBE,      COL_FIXED, {110,75,45,255},    {0,1.55f,0},         {0.68f,0.16f,0.68f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {80,55,30,255},     {0,1.55f,0},         {0.7f,0.1f,0.7f},         0, false, -1},
        {BP_CUBE,      COL_FIXED, {110,75,45,255},    {0.28f,1.8f,0},      {0.14f,0.28f,0.2f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {110,75,45,255},    {-0.28f,1.8f,0},     {0.14f,0.28f,0.2f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {80,55,30,255},     {0,1.92f,0},         {0.45f,0.03f,0.04f},      0, false, -1},
        {BP_SPHERE,    COL_FIXED, {255,30,20,255},    {0,1.6f,0.36f},      {0.08f,0,0},              0, false, -1},
        {BP_SPHERE,    COL_FIXED, {255,215,0,255},    {0,1.6f,0.38f},      {0.04f,0,0},              0, false, -1},
        {BP_CUBE,      COL_FIXED, {255,215,0,255},    {-0.56f,0.45f,0},    {0.28f,0.12f,0.28f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {255,215,0,255},    {0.56f,0.45f,0},     {0.28f,0.12f,0.28f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},            {0.54f,0.65f,0},     {0.28f,0.25f,0.48f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},            {-0.54f,0.65f,0},    {0.28f,0.25f,0.48f},      0, false, -1},
    }},
};

static const RankOverlayDef NCO_AMERICAN = {
    .parts = { .count = 10, .parts = {
        {BP_SPHERE,    COL_FIXED, {65,90,45,255},     {0,1.72f,0.03f},     {0.32f,0,0},              0, false, -1},
        {BP_CUBE,      COL_FIXED, {45,65,30,255},     {0,1.88f,0.03f},     {0.32f,0.05f,0.32f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {50,72,35,255},     {0,1.6f,0.45f},      {0.4f,0.03f,0.3f},        0, false, -1},
        {BP_CUBE,      COL_FIXED, {50,72,35,255},     {0,1.59f,0.55f},     {0.34f,0.04f,0.1f},       0, false, -1},
        {BP_SPHERE,    COL_FIXED, {45,65,30,255},     {0,1.9f,0.03f},      {0.04f,0,0},              0, false, -1},
        {BP_CUBE,      COL_FIXED, {45,65,30,255},     {0,1.65f,-0.22f},    {0.28f,0.05f,0.04f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {240,240,245,255},  {-0.56f,0.45f,0},    {0.28f,0.12f,0.28f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {240,240,245,255},  {0.56f,0.45f,0},     {0.28f,0.12f,0.28f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},            {0.54f,0.65f,0},     {0.28f,0.25f,0.48f},      0, false, -1},
        {BP_CUBE,      COL_SUIT_DARK, {0},            {-0.54f,0.65f,0},    {0.28f,0.25f,0.48f},      0, false, -1},
    }},
};

static const RankOverlayDef OFFICER_SOVIET = {
    .parts = { .count = 4, .parts = {
        {BP_CUBE,      COL_FIXED, {140,0,0,255},      {0,1.6f,0.3f},       {0.65f,0.04f,0.4f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {140,0,0,255},      {0,1.72f,0},         {0.52f,0.2f,0.52f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {255,215,0,255},    {0,1.62f,0},         {0.56f,0.06f,0.56f},      0, false, -1},
        {BP_SPHERE,    COL_FIXED, {255,215,0,255},    {0,1.72f,0.27f},     {0.07f,0,0},              0, false, -1},
    }},
    .hasSamBrowne    = true,  .strapColor    = {100,60,25,255},
    .hasCoatTail     = true,  .coatColor     = {140,0,0,255}, .coatTrimColor = {255,215,0,255},
    .hasEpaulettes   = true,  .epauletteColor= {255,215,0,255},
};

static const RankOverlayDef OFFICER_AMERICAN = {
    .parts = { .count = 5, .parts = {
        {BP_CUBE,      COL_FIXED, {15,25,60,255},     {0,1.6f,0.3f},       {0.65f,0.04f,0.4f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {15,25,60,255},     {0,1.72f,0},         {0.52f,0.2f,0.52f},       0, false, -1},
        {BP_CUBE,      COL_FIXED, {220,220,230,255},  {0,1.62f,0},         {0.56f,0.06f,0.56f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {220,220,230,255},  {0,1.72f,0.27f},     {0.14f,0.04f,0.04f},      0, false, -1},
        {BP_CUBE,      COL_FIXED, {220,220,230,255},  {0,1.72f,0.27f},     {0.04f,0.1f,0.04f},       0, false, -1},
    }},
    .hasSamBrowne    = true,  .strapColor    = {160,155,140,255},
    .hasCoatTail     = true,  .coatColor     = {15,25,60,255}, .coatTrimColor = {220,220,230,255},
    .hasEpaulettes   = true,  .epauletteColor= {220,220,230,255},
};

/* ============================================================================
 * Accessors
 * ============================================================================ */

const BodyDef *BodyDefGetAstronaut(void) { return &ASTRONAUT_BODY; }

const AnimProfile *AnimProfileGet(EnemyType faction) {
    return (faction == ENEMY_SOVIET) ? &SOVIET_PROFILE : &AMERICAN_PROFILE;
}

const GunDef *GunDefGet(EnemyType faction) {
    return (faction == ENEMY_SOVIET) ? &GUN_SOVIET : &GUN_AMERICAN;
}

const BodyColors *BodyColorsGet(EnemyType faction) {
    return (faction == ENEMY_SOVIET) ? &COLORS_SOVIET : &COLORS_AMERICAN;
}

const RankOverlayDef *RankOverlayGet(EnemyType faction, EnemyRank rank) {
    if (rank == RANK_TROOPER) return NULL;
    if (rank == RANK_NCO) {
        return (faction == ENEMY_SOVIET) ? &NCO_SOVIET : &NCO_AMERICAN;
    }
    if (rank == RANK_OFFICER) {
        return (faction == ENEMY_SOVIET) ? &OFFICER_SOVIET : &OFFICER_AMERICAN;
    }
    return NULL;
}

/* ============================================================================
 * Color Utilities
 * ============================================================================ */

Color BodyColorResolve(const BodyColors *pal, ColorRole role, Color fixed, bool dim) {
    Color c;
    switch (role) {
        case COL_SUIT:      c = pal->suit;      break;
        case COL_SUIT_DARK: c = pal->suitDark;  break;
        case COL_HELMET:    c = pal->helmet;     break;
        case COL_ACCENT:    c = pal->accent;     break;
        case COL_BOOT:      c = pal->boot;       break;
        case COL_BELT:      c = pal->belt;       break;
        case COL_BUCKLE:    c = pal->buckle;     break;
        case COL_GLOVE:     c = pal->glove;      break;
        case COL_BACKPACK:  c = pal->backpack;   break;
        case COL_VISOR:     c = pal->visor;      break;
        case COL_HOSE:      c = pal->hose;       break;
        default:            c = fixed;           break;
    }
    if (dim) {
        c.r /= 2; c.g /= 2; c.b /= 2;
        if (c.a == 255) c.a = 180;
    }
    return c;
}

static Color ApplyFlash(Color c, float hf) {
    if (hf <= 0) return c;
    int addR = (int)(hf * 140);
    int addG = (int)(hf * 70);
    int addB = (int)(hf * 56);
    int r = c.r + addR; if (r > 255) r = 255;
    int g = c.g + addG; if (g > 255) g = 255;
    int b = c.b + addB; if (b > 255) b = 255;
    return (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, c.a};
}

/* ============================================================================
 * DrawPartGroup — render all parts in a group
 * ============================================================================ */

static void DrawPartGroupInternal(const PartGroup *g, const BodyColors *pal,
                                  const Model models[MDL_COUNT], float hitFlash,
                                  int factionFilter, bool skipWire) {
    if (!g || !pal) return;
    int n = g->count;
    if (n > MAX_GROUP_PARTS) n = MAX_GROUP_PARTS;
    for (int i = 0; i < n; i++) {
        const BodyPart *p = &g->parts[i];
        if (p->factionOnly >= 0 && p->factionOnly != factionFilter) continue;
        if (skipWire && p->shape == BP_CUBE_WIRE) continue;

        Color c = BodyColorResolve(pal, p->color, p->fixedColor, p->dim);
        c = ApplyFlash(c, hitFlash);

        switch (p->shape) {
        case BP_CUBE:
            DrawCube(p->pos, p->size.x, p->size.y, p->size.z, c);
            break;
        case BP_CUBE_WIRE:
            DrawCubeWires(p->pos, p->size.x, p->size.y, p->size.z, c);
            break;
        case BP_SPHERE:
            DrawSphere(p->pos, p->size.x, c);
            break;
        case BP_MODEL:
            if (models && p->modelId >= 0 && p->modelId < MDL_COUNT)
                DrawModel(models[p->modelId], p->pos, 1.0f, c);
            break;
        }
    }
}

void DrawPartGroup(const PartGroup *g, const BodyColors *pal,
                   const Model models[MDL_COUNT], float hitFlash, int factionFilter) {
    DrawPartGroupInternal(g, pal, models, hitFlash, factionFilter, false);
}

void DrawPartGroupLOD(const PartGroup *g, const BodyColors *pal,
                      const Model models[MDL_COUNT], float hitFlash, int factionFilter) {
    DrawPartGroupInternal(g, pal, models, hitFlash, factionFilter, true);
}
