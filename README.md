# Dashloader v2.0.0

### Features
---

### 1) UI
- You can now see what's being launched etc...
- Supports PAL & NTSC resolutions bar 1080i
- Can be disabled if you don't like it.

### 2) Dashloader.ini
- This is the new home of settings & button mappings.
- It sits next the the xbe file.

### 3) Universal
- Works on hardmods & softmods.
- If it finds my softmod it will launch extras recovery dashes.
- If run as evoxdash.xbe it will skip this dash path so not to loop.

### 4) Better M8+ & Ind-Bios 5003 detection
- Now uses the kernel version to determine what bios.

### 5) Virtual ISO patch for M8+ & Ind-Bios 5003
- Add ISO loading to those bios.

### 6) Universal ISO dismounter
- Works on Softmods, Cerbios & legacy bios.
  (M8+ & Ind-Bios 5003)

### 7) Better logging
- Output is nicer to the eye.

### 8) Better button press handling
- Now wont accept more than 1 button pressed
- Unless I have hardcoded it. (Start+Y for recovery dash loading)

### 9) Updated dashboard path order
You can override these by setting a path to a valid xbe in
- [Dashboard]
- Path

#### E:\
- XBMC-Emustation/Default.xbe
- XBMC4Gamers/Default.xbe
- XBMC4Xbox/Default.xbe
- Dashboard/Default.xbe
- Dash/Default.xbe
- XBMC/Default.xbe
- XBMC.xbe
- Evoxdash.xbe

#### C:\
- XBMC-Emustation/Default.xbe
- XBMC4Gamers/Default.xbe
- XBMC4Xbox/Default.xbe
- Dashboard/Default.xbe
- Dash/Default.xbe
- Evoxdash.xbe

#### F:\
- XBMC-Emustation/Default.xbe
- XBMC4Gamers/Default.xbe
- XBMC4Xbox/Default.xbe
- XBMC/Default.xbe
- Dashboard/Default.xbe
- Dash/Default.xbe

### 10) Dashloader Bios Loader
- Separate XBE for loading PBL Metoo Edition to load BFM bios.
- Hold Back+Start at boot to enable the UI and launch the recovery PBL + BFM bios.
- Looks for PBL + BFM bios in C:\Bios loader\
- Designed to be loaded first before PBL so you can recover.

---
### Extra info
---

If manually editing **dashloader.ini**, take note:

### Strings (Path to xbe)
- [Buttons]
  - A
  - B
  - X
  - Y
  - White
  - Black
  - Back
  - Start

- [Recovery]
  - Path

- [Dashboard]
  - Path

### Bools (0 or 1)
- [UI]
  - Enabled

- [Logging]
  - Enabled

- [VirtualDrive]
  - ISOKernelPatch
  - DismountISOonIGR

### Ints (0 > 9999999)
- [UI]
  - ButtonDelay
  - LaunchDelay

---
### Acknowledgment
Dashloader started out based on [XBMC ShorcutXBE](https://github.com/Rocky5/XBMC4Xbox/tree/main/tools/ShortcutXBE)
It’s now basically a full rewrite, so thanks to the XBMC team for the original code used in versions
**1.0.0 > 1.4.3**

Microsoft for the samples, without those I wouldn't have got video working.

---

**You're free to do what you want with this, just show credit where it's due.**

---