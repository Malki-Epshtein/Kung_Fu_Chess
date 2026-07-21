#include "CaptureEventObserver.h"

void CaptureEventObserver::onPieceCaptured(const Piece& captured) {
    if (onNewCapture)
        onNewCapture(CaptureEvent{ captured.getKind(), captured.getColor(), captured.getCell() });
}
