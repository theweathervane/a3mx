#!/usr/bin/env python3
"""Where each of the game's archives goes under assets/<region>/: one place, used by the extractor,
the tests and the tools (assets/LAYOUT.md has the rules).

Each archive is unpacked into one folder, placed by what it holds rather than by its file name on the
disc: Ryu's fighter archive pl00_u.pac goes to characters/00_ryu/fighter/, Dan's stage st0c.pac to
stages/0c_dan/. Inside a folder, each entry goes in a subfolder for its kind (textures/, palettes/ ...).
The extractor writes the result to assets/<region>/index.json, and that file, not this one, is what
the game reads (src/platform/asset_pac.c), so a folder can be renamed or moved by editing the index.

Usage:
    python tools/asset_layout.py                  print every archive's folder
    python tools/asset_layout.py st0c pl00_u      print these archives' folders
Standard library only.
"""
import re, sys

# The 42 character slots, in the game's order. The names are the game's own (the slot-name table at
# 0x08A0F718, docs/STAGES.md), lower case, punctuation dropped. Slots 0x12, 0x13, 0x14 and 0x25 repeat
# an earlier name but have fighter data of their own: variants not identified yet. The number in front
# keeps every slot's folder unique and in the game's order.
CHARACTERS = [
    'ryu', 'ken', 'akuma', 'charlie', 'chunli', 'adon', 'sodom', 'guy',                 # 00-07
    'birdie', 'rose', 'bison', 'sagat', 'dan', 'sakura', 'rolento', 'dhalsim',          # 08-0f
    'zangief', 'gen', 'chunli', 'gen', 'sodom', 'balrog', 'cammy', 'evil_ryu',          # 10-17
    'honda', 'blanka', 'mika', 'cody', 'vega', 'karin', 'juli', 'juni',                 # 18-1f
    'guile', 'feilong', 'deejay', 'hawk', 'shin_akuma', 'balrog', 'eagle', 'maki',      # 20-27
    'yun', 'ingrid',                                                                    # 28-29
]

# The 38 stages, named as FREE BATTLE's stage row names them (docs/STAGES.md), by stage number.
STAGES = {
    0x00: 'ryu_1', 0x01: 'ken', 0x02: 'akuma_1', 0x03: 'charlie', 0x04: 'chunli', 0x05: 'adon',
    0x06: 'sodom', 0x07: 'guy', 0x08: 'birdie', 0x09: 'rose', 0x0A: 'bison_1', 0x0B: 'sagat',
    0x0C: 'dan', 0x0D: 'sakura', 0x0E: 'rolento', 0x0F: 'dhalsim', 0x10: 'zangief', 0x11: 'gen',
    0x15: 'balrog', 0x16: 'cammy', 0x17: 'ryu_2', 0x18: 'honda', 0x19: 'blanka', 0x1A: 'mika',
    0x1B: 'cody', 0x1C: 'vega', 0x1D: 'karin_1', 0x1E: 'bison_2', 0x20: 'guile', 0x21: 'feilong',
    0x22: 'deejay', 0x23: 'hawk', 0x24: 'akuma_2', 0x25: 'battle_1', 0x26: 'battle_2',
    0x27: 'battle_3', 0x28: 'battle_4', 0x29: 'karin_2',
}

# A character's own archives: disc prefix -> the subfolder of characters/<nn>_<name>/.
CHARACTER_PARTS = {
    'pl': 'fighter',        # fighter data: sprites and animation, palettes, moves
    'op': 'intro',          # the pre-fight intro
    'win': 'win',           # the win screen and quotes
    'end': 'ending',        # the Arcade ending
    'bd': 'bio',            # the bio
    'rk': 'ranking',        # per-character ranking data (what it shows is not checked yet)
}

# Everything else, by the archive's name without its region suffix (_u on the US disc).
OTHER = {
    'logo': 'screens/logo',
    'open': 'screens/opening',
    'title': 'screens/title',
    'msel': 'screens/mode_menu',
    'select': 'screens/select',
    'opt': 'screens/option',
    'memory': 'screens/save_load',
    'world': 'screens/world_tour',
    'wr_scr': 'screens/world_tour_screen',
    'wrload': 'screens/world_tour_loading',
    'netd': 'screens/network_d',      # ad-hoc play; which screens each holds is not checked yet
    'nets': 'screens/network_s',
    'pit': 'screens/pit',             # name as on the disc: what it is is not checked yet
    'rkcom': 'screens/ranking',       # shared by every character's ranking/
    'endcom': 'screens/ending',       # shared by every character's ending/
    'efct': 'effects/common',
    'ascii': 'fonts/ascii',
    'se_mid': 'sound/effects',        # the sound effects bank
    'titlesnd': 'sound/title',        # the title's voices
    # Not identified yet: kept under other/ with their disc names until they are (assets/LAYOUT.md).
    'ake': 'other/ake',               # effects textures
    'zett': 'other/zett',
    'sign': 'other/sign',
    'vram': 'other/vram',
    'wall': 'other/wall',             # a character collage
}

# An entry's kind (tools/pac.py kind_name) -> the subfolder it goes in inside its archive's folder.
KIND_FOLDERS = {
    'texture': 'textures',
    'palette': 'palettes',
    'text': 'text',
    'sound': 'sound',
    'voice': 'voices',
    'fighter-a': 'fighter_data',
    'fighter-sprites': 'fighter_data',
    'fighter-c': 'fighter_data',
}


def kind_folder(kind):
    """The subfolder of an archive's folder that an entry of this kind goes in (data/ for the kinds
    not decoded yet)."""
    return KIND_FOLDERS.get(kind, 'data')


def character(n):
    return 'characters/%02x_%s' % (n, CHARACTERS[n])


def stage(n):
    return 'stages/%02x_%s' % (n, STAGES[n])


def folder(archive):
    """The folder under assets/<region>/ for an archive, by its name on the disc ('pl00_u', 'st0c' or
    'st0c.pac'). Every archive on the US and Japanese discs has one; an unknown name goes to
    other/<name>, so a new one is never lost."""
    name = archive[:-4] if archive.lower().endswith('.pac') else archive
    name = name.lower()
    base = name[:-2] if name.endswith('_u') else name          # the US disc's suffix
    m = re.fullmatch(r'st([0-9a-f]{2})', base)
    if m and int(m.group(1), 16) in STAGES: return stage(int(m.group(1), 16))
    m = re.fullmatch(r'(pl|op|win|end|bd|rk)([0-9a-f]{2})(r?)', base)
    if m and int(m.group(2), 16) < len(CHARACTERS):
        part = CHARACTER_PARTS[m.group(1)] + ('_r' if m.group(3) else '')   # pl29r: a second INGRID
        return '%s/%s' % (character(int(m.group(2), 16)), part)
    if base in OTHER: return OTHER[base]
    return 'other/' + name


def main():
    import glob, os
    names = sys.argv[1:]
    if not names:
        root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'work', 'iso',
                            'PSP_GAME', 'USRDIR', 'usa', 'data')
        names = sorted(os.path.basename(p) for p in glob.glob(os.path.join(root, '*.pac')))
        if not names: raise SystemExit('asset_layout.py: no archives in %s: name some' % root)
    for n in names: print('%-12s %s' % (n, folder(n)))


if __name__ == '__main__':
    main()
