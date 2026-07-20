#pragma once

// Opens the Home Screen window (Play/Room buttons) and blocks until the
// user closes it (ESC or the window's close button). Stage G2a: clicking
// Play or Room only logs the choice for now - neither leads anywhere yet
// (that wiring arrives in G2b/G2c). Needs OpenCV, so - like
// GraphicalApplication/ImageView - this stays out of the Win32 test build.
void runHomeScreen();
