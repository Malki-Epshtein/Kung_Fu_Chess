#pragma once
#include <string>

struct RoomDialogResult {
    enum class Action { Create, Join, Cancel };
    Action      action = Action::Cancel;
    std::string roomName;
};

// Opens a native Win32 dialog (textbox + Create/Join/Cancel buttons) and
// blocks until the user picks one. Cancel - or closing the dialog's
// window - returns Action::Cancel with an empty roomName. Stage G2b: this
// only collects the choice, it does not talk to the server yet (that's
// G2c). OpenCV has no real text-input widget, hence a real Win32 window
// here instead of drawing this on the OpenCV canvas.
RoomDialogResult showRoomDialog();

// Shows a native error popup (e.g. "room not found", "room already
// exists") - the server-side rejection already happens correctly; this is
// what actually surfaces it to the user instead of only the console log.
// Isolated here (not in HomeScreenView.cpp) because it needs <windows.h>,
// which conflicts with OpenCV's own headers if included in the same file.
void showRoomError(const std::string& message);
