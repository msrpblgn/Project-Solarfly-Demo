/* Generated from skills_menu.aseprite — do not hand-edit. */
#ifndef SKILL_CARD_REF_ASSETS_INC
#define SKILL_CARD_REF_ASSETS_INC

#define SKILL_CARD_NAME_FONT_H 6
#define SKILL_CARD_NAME_FONT_SCALE 2
#define SKILL_CARD_COMPACT_FONT_H 6

static const u16 s_neutralElementMask[12] = {
    0x0F0, 0x30C, 0x4F2, 0x5FA, 0xBFD, 0xBFD, 0xBFD, 0xBFD, 0x5FA, 0x4F2, 0x30C, 0x0F0
};

typedef struct SkillCardNameGlyph {
    u8 width;
    u8 advance;
    u8 rows[6]; /* bit0 = leftmost */
} SkillCardNameGlyph;

/* Extracted name glyphs from 'Hammer Fist' sample (9). */
static const SkillCardNameGlyph s_nameGlyph_70_F = {4, 5, {15, 1, 7, 1, 1, 1}};
static const SkillCardNameGlyph s_nameGlyph_72_H = {4, 5, {9, 9, 15, 9, 9, 9}};
static const SkillCardNameGlyph s_nameGlyph_97_a = {4, 5, {0, 6, 8, 14, 9, 14}};
static const SkillCardNameGlyph s_nameGlyph_101_e = {4, 5, {0, 6, 9, 15, 1, 6}};
static const SkillCardNameGlyph s_nameGlyph_105_i = {1, 2, {1, 0, 1, 1, 1, 1}};
static const SkillCardNameGlyph s_nameGlyph_109_m = {7, 8, {0, 63, 73, 73, 73, 73}};
static const SkillCardNameGlyph s_nameGlyph_114_r = {3, 4, {0, 5, 3, 1, 1, 1}};
static const SkillCardNameGlyph s_nameGlyph_115_s = {3, 4, {0, 6, 1, 2, 4, 3}};
static const SkillCardNameGlyph s_nameGlyph_116_t = {2, 3, {1, 3, 1, 1, 1, 2}};

static const SkillCardNameGlyph *SkillCard_LookupExtractedNameGlyph(char c)
{
    switch (c) {
    case 'F': return &s_nameGlyph_70_F;
    case 'H': return &s_nameGlyph_72_H;
    case 'a': return &s_nameGlyph_97_a;
    case 'e': return &s_nameGlyph_101_e;
    case 'i': return &s_nameGlyph_105_i;
    case 'm': return &s_nameGlyph_109_m;
    case 'r': return &s_nameGlyph_114_r;
    case 's': return &s_nameGlyph_115_s;
    case 't': return &s_nameGlyph_116_t;
    default: return 0;
    }
}

typedef struct SkillCardCompactGlyph {
    u8 width;
    u8 advance;
    u8 height;
    u8 rows[7]; /* bit0 = leftmost */
} SkillCardCompactGlyph;

static const SkillCardCompactGlyph s_compactExt_56_8 = {4, 5, 6, {6, 9, 6, 9, 9, 6, 0}};
static const SkillCardCompactGlyph s_compactExt_67_C = {4, 5, 6, {14, 1, 1, 1, 1, 14, 0}};
static const SkillCardCompactGlyph s_compactExt_83_S = {4, 5, 6, {14, 1, 6, 8, 8, 7, 0}};
static const SkillCardCompactGlyph s_compactExt_111_o = {4, 5, 7, {0, 6, 9, 9, 9, 6, 0}};
static const SkillCardCompactGlyph s_compactExt_112_p = {4, 5, 7, {0, 7, 9, 9, 7, 1, 1}};
static const SkillCardCompactGlyph s_compactExt_114_r = {3, 4, 7, {0, 5, 3, 1, 1, 1, 0}};
static const SkillCardCompactGlyph s_compactExt_116_t = {2, 3, 7, {1, 3, 1, 1, 1, 2, 0}};
static const SkillCardCompactGlyph s_compactExt_117_u = {4, 5, 7, {0, 9, 9, 9, 9, 14, 0}};

static const SkillCardCompactGlyph *SkillCard_LookupExtractedCompactGlyph(char c)
{
    switch (c) {
    case '8': return &s_compactExt_56_8;
    case 'C': return &s_compactExt_67_C;
    case 'S': return &s_compactExt_83_S;
    case 'o': return &s_compactExt_111_o;
    case 'p': return &s_compactExt_112_p;
    case 'r': return &s_compactExt_114_r;
    case 't': return &s_compactExt_116_t;
    case 'u': return &s_compactExt_117_u;
    default: return 0;
    }
}

#endif
