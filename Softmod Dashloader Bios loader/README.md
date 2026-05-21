# Dashloader Bios Loader

A lightweight XBE designed to be placed before Phoenix Bios Loader Metoo Edition, giving you a recovery window to recover from a bad overclock or something else that locks the system when using a BFM bios. (this is mainly for softmod BFM setups) Yes I know you can boot an alt bios via eject with PBL but if you bork PBL then what. It's 400ms for peace of mind :P.

You must know what you're doing to use this, I take no responsibility if you bork something.
`bios.xbe`, `default.xbe` and `recovery.xbe` must be habibi signed (xbedump.exe) if edited in anyway shape of form. When you build bios loader via the bat file it gets signed automatically.

---

### How it works

- It's set as the first xbe ernie loads
- Checks **Back + Start** being held to launch `recovery.xbe`.
- Loads `bios.xbe` then `recovery.xbe` if one is missing it will fall through to the next.

You have a 100ms window for holding buttons, so when you see the Xlogo screen hold the buttons then and keep them pressed down until you see the recovery text.

---

### Setup

Place the `Bios loader` folder in `C:\` on your Xbox.

If running a softmod you can replace `ernie.xtf` (included) in `C:\xodash` - this will boot `C:\Bios Loader\default.xbe` (Dashloader Bios Loader).

Dashloader Bios Loader will then try running `C:\Bios Loader\bios.xbe` or if you hold **Back + Start** it will then try loading `C:\Bios Loader\recovery.xbe`.
If `bios.xbe` is missing it will try `recovery.xbe` if both are missing you will get a error screen with a 2 min countdown till it reboots.

---

### Dashloader.ini

Placed in `C:\Bios Loader\`. Only the logging setting is used, all other settings are ignored.

### Bools (0 or 1)
- [Logging]
  - Enabled

---

### Why use this?

I made this as I was doing the config editor in XBMC4Gamers for Cerbios (my own choice) and I overclocked too much and soft bricked my Xbox. So I made this so I could get something akin to Cerbios safe mode on a softmodded Xbox.
PBL support loading an alternative bios via eject, so that's also a viable alternative to this. But if you break PBL, like I have also done :/ this can save you.

---

**You're free to do what you want with this, just show credit where it's due.**

---