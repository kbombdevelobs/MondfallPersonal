#ifndef ENEMY_BODYDEF_H
#define ENEMY_BODYDEF_H

#include "raylib.h"
#include "enemy_components.h"

/* ============================================================================
 * Data-driven body definition system for enemy rendering.
 *
 * To add a new enemy type: define a new BodyDef + BodyColors + GunDef(s) +
 * RankOverlayDef(s), then add accessors.  The draw code and spring system
 * work automatically with any BodyDef.
 * ============================================================================ */

typedef enum { BP_CUBE, BP_CUBE_WIRE, BP_SPHERE, BP_MODEL } BodyPartShape;

typedef enum {
    COL_SUIT, COL_SUIT_DARK, COL_HELMET, COL_ACCENT, COL_BOOT,
    COL_BELT, COL_BUCKLE, COL_GLOVE, COL_BACKPACK, COL_VISOR,
    COL_HOSE, COL_FIXED
} ColorRole;

enum { MDL_VISOR = 0, MDL_ARM = 1, MDL_BOOT = 2, MDL_COUNT = 3 };

typedef struct {
    BodyPartShape shape;
    ColorRole     color;
    Color         fixedColor;   /* used when color == COL_FIXED          */
    Vector3       pos;          /* local offset within limb group        */
    Vector3       size;         /* cube x/y/z; sphere radius in .x       */
    int           modelId;      /* for BP_MODEL                          */
    bool          dim;          /* halve RGB (wireframe borders)         */
    int           factionOnly;  /* -1=all, 0=SOVIET only, 1=AMERICAN     */
} BodyPart;

#define MAX_GROUP_PARTS 16

typedef struct {
    BodyPart parts[MAX_GROUP_PARTS];
    int      count;
} PartGroup;

typedef struct {
    Color suit, suitDark, helmet, accent, boot;
    Color belt, buckle, glove, backpack, visor, hose;
} BodyColors;

typedef struct {
    float stiffness;
    float damping;
} SpringTune;

/* Animation profile — faction-specific spring tuning and motion amplitudes */
typedef struct {
    float armSwingDeg;       /* arm swing amplitude (degrees)          */
    float legSwingDeg;
    float armBasePitch;      /* resting arm pitch (-30)                */
    float armBaseSpread;     /* resting outward angle (-16 for right)  */
    float kneeBendFactor;    /* knee bend as fraction of leg swing     */

    SpringTune armSwingSpr;
    SpringTune armSpreadSpr;
    SpringTune legSwingSpr;
    SpringTune legSpreadSpr;
    SpringTune kneeSpr;
    SpringTune hipSwaySpr;
    SpringTune torsoLeanSpr;
    SpringTune torsoTwistSpr;
    SpringTune headBobSpr;

    float hipSwayAmount;     /* lateral sway (world units)             */
    float torsoLeanDeg;      /* max forward lean at full speed         */
    float torsoTwistDeg;     /* counter-rotation amplitude             */
    float headBobDeg;        /* vertical nod amplitude                 */
    float breathRate;        /* idle breathing (Hz)                    */
    float breathScale;       /* torso pulse magnitude                  */
    float kneeFlexBase;      /* constant knee bend (neg = crouched)    */
} AnimProfile;

/* Complete body definition (faction-independent geometry) */
typedef struct {
    PartGroup torso;
    PartGroup head;          /* drawn at headPivotY                    */
    PartGroup backpack;
    PartGroup armParts;      /* one arm template (mirrored for left)   */
    PartGroup legUpper;      /* one thigh template (mirrored)          */
    PartGroup legLower;      /* one shin+boot template (mirrored)      */

    Vector3   armPivot;      /* shoulder joint                         */
    Vector3   legPivot;      /* hip joint                              */
    Vector3   kneePivot;     /* relative to leg pivot                  */
    Vector3   gunHandOffset; /* gun grip in arm-local space            */
    float     headPivotY;
} BodyDef;

typedef struct {
    PartGroup parts;
    Vector3   pivot;         /* gun attachment point                   */
    float     muzzleZ;       /* barrel tip Z for muzzle flash          */
    float     restPitchOffset; /* idle arm pitch adjustment per weapon */
} GunDef;

typedef struct {
    PartGroup parts;         /* static parts drawn at root             */
    bool      hasCoatTail;
    Color     coatColor;
    Color     coatTrimColor;
    bool      hasSamBrowne;
    Color     strapColor;
    bool      hasEpaulettes;
    Color     epauletteColor;
} RankOverlayDef;

/* Accessors */

/// Return the shared astronaut body geometry definition (torso, head, backpack, arms, legs, pivots).
const BodyDef        *BodyDefGetAstronaut(void);
/// Return the faction-specific animation profile (spring tuning, motion amplitudes, breathing).
const AnimProfile    *AnimProfileGet(EnemyType faction);
/// Return the faction-specific gun definition (weapon parts, muzzle position, rest pitch offset).
const GunDef         *GunDefGet(EnemyType faction);
/// Return the faction-specific color palette for all body part roles (suit, helmet, visor, etc.).
const BodyColors     *BodyColorsGet(EnemyType faction);
/// Return the rank overlay definition (insignia, coat tail, Sam Browne belt) for a faction+rank, or NULL for troopers.
const RankOverlayDef *RankOverlayGet(EnemyType faction, EnemyRank rank);

/// Resolve a ColorRole enum to a concrete Color from the palette, applying dim and hit-flash modifiers.
Color BodyColorResolve(const BodyColors *pal, ColorRole role, Color fixed, bool dim);

/// Draw all parts in a PartGroup using the given palette, models, hit flash, and faction filter.
void DrawPartGroup(const PartGroup *g, const BodyColors *pal,
                   const Model models[MDL_COUNT], float hitFlash, int factionFilter);

/// Draw a PartGroup at reduced detail (skips BP_CUBE_WIRE parts) for LOD rendering.
void DrawPartGroupLOD(const PartGroup *g, const BodyColors *pal,
                      const Model models[MDL_COUNT], float hitFlash, int factionFilter);

#endif /* ENEMY_BODYDEF_H */
