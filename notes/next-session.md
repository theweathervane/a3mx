# Next session

The full working notes are the project's notes (notes/next-session.md and notes/widescreen.md in the
claude.ai project). In short, as of 0.99.1:

- WIDE FORCED is native: every function Rickow's 16:9 patch changed is decompiled (0.99.0) and the
  patch is no longer downloaded or translated in (0.99.1). docs/WIDESCREEN.md lists where it lives.
- The WIDE scenarios compare the fighters with WIDE FORCED off and on (`wide` in a scenario,
  tests/parity.py); drawing is checked by eye.
- Not reached by any scenario yet (native, straight from the patched code): 0x088D6D3C, the network
  menu rows with ids 0, 9 and 10, and Cammy's CPU routine states 0x0895FD00 / 0x0895FDB0 (her routines
  23 / 24 through the CPU's script commands; a CPU Cammy in Free Battle only ran routine 4).
- Open for WIDE FORCED: the MODE MENU's left side (widen its drift objects' layout or pillarbox),
  the caves stages' placeholders against the last layer's black strips, the art pack's whole layers.
