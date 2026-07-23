#include "CaptureEventObserver.h"

void CaptureEventObserver::onPieceCaptured(const Piece& captured, const CaptureImpact& impact) {
    if (onNewCapture)
        onNewCapture(CaptureEvent{
            captured.getKind(), captured.getColor(), captured.getCell(),
            captured.getId(), impact.capturingPieceId, impact.collisionCell, impact.impactProgress });
}
