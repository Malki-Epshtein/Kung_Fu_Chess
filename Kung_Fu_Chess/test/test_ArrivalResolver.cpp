#include "../doctest.h"
#include "../realtime/ArrivalResolver.h"
#include "../model/Board.h"
#include <memory>

class TestPiece : public Piece {
public:
    TestPiece(int id, Chess::Color color, Chess::Kind kind, Position cell)
        : Piece(id, color, kind, cell, Chess::State::Idle) {}
    std::vector<Position> getValidMoves() const override { return {}; }
};

static std::shared_ptr<Piece> make(int id, Chess::Color color, Chess::Kind kind, Position pos) {
    return std::make_shared<TestPiece>(id, color, kind, pos);
}

namespace {
    struct RecordingCaptureObserver : public CaptureObserver {
        std::vector<int> capturedIds;
        void onPieceCaptured(const Piece& captured) override { capturedIds.push_back(captured.getId()); }
    };
    struct RecordingMoveObserver : public MoveObserver {
        struct Move { int piece_id; Position from; Position to; bool wasCapture; };
        std::vector<Move> moves;
        void onMoveCompleted(const Piece& mover, Position from, Position to, bool wasCapture) override {
            moves.push_back({ mover.getId(), from, to, wasCapture });
        }
    };
}

TEST_CASE("ArrivalResolver::resolve - מהלך רגיל בלי אכילה מזיז את הכלי ומשדר onMoveCompleted") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    RecordingMoveObserver moveObs;
    captureObservers.push_back(&captureObs);
    moveObservers.push_back(&moveObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    CHECK_FALSE(kingCaptured);
    CHECK(captureObs.capturedIds.empty());
    REQUIRE(moveObs.moves.size() == 1);
    CHECK(moveObs.moves[0].piece_id == 1);
    CHECK_FALSE(moveObs.moves[0].wasCapture);
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Rook);
    CHECK(board.isCellEmpty({3, 3}));
    CHECK(cooldownUntilMs[1] == 2000);
}

TEST_CASE("ArrivalResolver::resolve - הגעה ליעד עם כלי אויב אוכלת אותו ומשדרת גם capture וגם move") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {3, 4}), {3, 4});

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    RecordingMoveObserver moveObs;
    captureObservers.push_back(&captureObs);
    moveObservers.push_back(&moveObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    REQUIRE(captureObs.capturedIds.size() == 1);
    CHECK(captureObs.capturedIds[0] == 2);
    REQUIRE(moveObs.moves.size() == 1);
    CHECK(moveObs.moves[0].wasCapture);
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Rook);
}

TEST_CASE("ArrivalResolver::resolve - הגעה ליעד עם מלך אויב מסמנת kingCaptured") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {3, 4}), {3, 4});

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    CHECK(kingCaptured);
}

TEST_CASE("ArrivalResolver::resolve - collisionEnabled: הגעה ליעד עם כלי ידידותי נכשלת בשקט, בלי אכילה") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {3, 4}), {3, 4});

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    RecordingMoveObserver moveObs;
    captureObservers.push_back(&captureObs);
    moveObservers.push_back(&moveObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, true);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    CHECK(captureObs.capturedIds.empty());
    CHECK(moveObs.moves.empty());
    CHECK(board.getPiece({3, 3})->getKind() == Chess::Kind::Rook); // נשאר במקומו
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Pawn); // הידידותי נשאר בשלמותו
    CHECK(board.getPiece({3, 3})->getState() == Chess::State::LongRest);
}

TEST_CASE("ArrivalResolver::resolve - נחיתת קפיצה עוברת ל-ShortRest ולא משדרת שום Observer") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Jump);

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    RecordingMoveObserver moveObs;
    captureObservers.push_back(&captureObs);
    moveObservers.push_back(&moveObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 3}, 0, 1000, 1 }; // קפיצה: from==to
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    CHECK(captureObs.capturedIds.empty());
    CHECK(moveObs.moves.empty());
    CHECK(board.getPiece({3, 3})->getState() == Chess::State::ShortRest);
    CHECK(cooldownUntilMs[1] == 1500);
}

TEST_CASE("ArrivalResolver::resolve - חייל שמגיע לשורה האחרונה מקודם למלכה") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Pawn, {1, 3}), {1, 3});
    board.getPiece({1, 3})->transitionTo(Chess::State::Moving);

    std::vector<Motion> activeMotions;
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {1, 3}, {0, 3}, 0, 1000, 1 }; // שורה 0 = יעד הקידום עבור לבן
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    CHECK(board.getPiece({0, 3})->getKind() == Chess::Kind::Queen);
}

TEST_CASE("ArrivalResolver::resolve - נתפס על ידי קפיצה: כלי אויב נחת על היעד תוך כדי שהמהלך בדרך") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {3, 4}), {3, 4});
    board.getPiece({3, 4})->transitionTo(Chess::State::Jump);

    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {3, 4}, {3, 4}, 0, 1000, 2 }); // הפרש עדיין באוויר

    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    captureObservers.push_back(&captureObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    REQUIRE(captureObs.capturedIds.size() == 1);
    CHECK(captureObs.capturedIds[0] == 1); // הרץ שהגיע נתפס, לא הפרש
    CHECK(board.isCellEmpty({3, 3}));
}

TEST_CASE("ArrivalResolver::resolve - נתפס גם כשהקפיצה כבר נגמרה אבל הכלי עדיין ב-ShortRest") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {3, 4}), {3, 4});
    board.getPiece({3, 4})->transitionTo(Chess::State::Jump);
    board.getPiece({3, 4})->transitionTo(Chess::State::ShortRest); // הקפיצה כבר הסתיימה, אין לה יותר Motion פעיל

    std::vector<Motion> activeMotions; // ריק בכוונה - שום Motion פעיל לפרש יותר
    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    captureObservers.push_back(&captureObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    REQUIRE(captureObs.capturedIds.size() == 1);
    CHECK(captureObs.capturedIds[0] == 1); // הרץ שהגיע נתפס, לא הפרש - למרות שהקפיצה כבר נגמרה
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Knight); // הפרש נשאר במקומו
}

TEST_CASE("ArrivalResolver::resolve - הגנת קפיצה חלה גם על כלי ידידותי שמנסה לנחות, לא רק על אויב") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {3, 3}), {3, 3});
    board.getPiece({3, 3})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Knight, {3, 4}), {3, 4}); // ידידותי, לא אויב
    board.getPiece({3, 4})->transitionTo(Chess::State::Jump);

    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {3, 4}, {3, 4}, 0, 1000, 2 }); // הפרש הידידותי עדיין באוויר

    std::unordered_map<int, int> cooldownUntilMs;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::vector<MoveObserver*> moveObservers;
    RecordingCaptureObserver captureObs;
    captureObservers.push_back(&captureObs);

    ArrivalResolver resolver(board, activeMotions, cooldownUntilMs, capturedPieces, captureObservers, moveObservers, false);

    Motion m{ {3, 3}, {3, 4}, 0, 1000, 1 };
    bool kingCaptured = false;
    resolver.resolve(m, 1000, kingCaptured);

    REQUIRE(captureObs.capturedIds.size() == 1);
    CHECK(captureObs.capturedIds[0] == 1); // הצריח הידידותי שניסה לנחות נתפס, לא הפרש הקופץ
    CHECK(board.getPiece({3, 4})->getKind() == Chess::Kind::Knight); // הפרש הקופץ נשאר במקומו
}
