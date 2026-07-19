#include "../doctest.h"
#include "../server/realtime/CollisionResolver.h"
#include "../shared/model/Board.h"
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

// ==================== applyNearMiss ====================

TEST_CASE("CollisionResolver::applyNearMiss - כלי ידידותי שמגיע מאוחר יותר לתא המשותף נעצר משבצת אחת לפניו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Queen, {5, 3}), {5, 3});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 }); // מגיע ל-(0,3) ב-t=3000

    Motion second{ {5, 3}, {0, 3}, 0, 5000, 2 }; // בלי קיצור, היה מגיע ל-(0,3) ב-t=5000
    bool started = resolver.applyNearMiss(second);

    CHECK(started);
    CHECK(second.to == Position{1, 3});
    CHECK(second.arrival_time_ms == 4000);
}

TEST_CASE("CollisionResolver::applyNearMiss - אם הצעד הראשון כבר תפוס עי מסלול ידידותי, המהלך נדחה לגמרי") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Queen, {1, 1}), {1, 1});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 }); // עובר דרך (0,1) ב-t=1000

    Motion second{ {1, 1}, {0, 1}, 0, 1000, 2 }; // הצעד הראשון שלו הוא בדיוק (0,1), גם ב-t=1000
    bool started = resolver.applyNearMiss(second);

    CHECK_FALSE(started);
}

TEST_CASE("CollisionResolver::applyNearMiss - פרש פטור לגמרי מהבדיקה") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Knight, {1, 1}), {1, 1});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });

    Motion knightMotion{ {1, 1}, {0, 1}, 0, 1000, 2 };
    bool started = resolver.applyNearMiss(knightMotion);

    CHECK(started);
    CHECK(knightMotion.to == Position{0, 1}); // לא קוצר בכלל
}

// ==================== resolveCollisions ====================

TEST_CASE("CollisionResolver::resolveCollisions - שני כלי אויב חוצים נתיב, מי שהתחיל קודם מנצח") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 3}), {0, 3});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });    // לבן: החל ב-t=0
    activeMotions.push_back({ {0, 3}, {0, 4}, 3000, 4000, 2 }); // שחור: החל ב-t=3000, שניהם מגיעים ל-(0,4) ב-t=4000

    bool kingCaptured = false;
    resolver.resolveCollisions(3000, 4000, kingCaptured);

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({0, 3}));
    REQUIRE(capturedPieces.size() == 1);
    CHECK(capturedPieces[0]->getId() == 2);
    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].piece_id == 1);
}

TEST_CASE("CollisionResolver::resolveCollisions - אם המפסיד הוא מלך, מסמן kingCaptured") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::King, {0, 3}), {0, 3});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });
    activeMotions.push_back({ {0, 3}, {0, 4}, 3000, 4000, 2 });

    bool kingCaptured = false;
    resolver.resolveCollisions(3000, 4000, kingCaptured);

    CHECK(kingCaptured);
}

TEST_CASE("CollisionResolver::resolveCollisions - כלים באותו צבע לא מתנגשים") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {0, 3}), {0, 3});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });
    activeMotions.push_back({ {0, 3}, {0, 4}, 3000, 4000, 2 });

    bool kingCaptured = false;
    resolver.resolveCollisions(3000, 4000, kingCaptured);

    CHECK(capturedPieces.empty());
    CHECK(activeMotions.size() == 2);
}

TEST_CASE("CollisionResolver::resolveCollisions - שני כלי אויב מחליפים משבצות סמוכות (swap), בעל המזהה הגבוה מפסיד") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 1}), {0, 1});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 1}, 0, 1000, 1 }); // לבן זז לתא של השחור
    activeMotions.push_back({ {0, 1}, {0, 0}, 0, 1000, 2 }); // שחור זז לתא של הלבן, אותו רגע

    bool kingCaptured = false;
    resolver.resolveCollisions(0, 1000, kingCaptured);

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({0, 1}));
    REQUIRE(capturedPieces.size() == 1);
    CHECK(capturedPieces[0]->getId() == 2);
    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].piece_id == 1);
}

TEST_CASE("CollisionResolver::resolveCollisions - שני כלי אויב חוצים אותו תא בזמנים לא-מסונכרנים, המאוחר מפסיד") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Rook, {0, 4}), {0, 4});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    // לבן: החל ב-t=0, יגיע ל-(0,3) בחלון [3000,4000).
    activeMotions.push_back({ {0, 0}, {0, 4}, 0, 4000, 1 });
    // שחור: החל ב-t=1500, יגיע ל-(0,3) בחלון [2500,3500) - אותו תא, זמנים חופפים אך לא זהים.
    activeMotions.push_back({ {0, 4}, {0, 1}, 1500, 4500, 2 });

    bool kingCaptured = false;
    resolver.resolveCollisions(2500, 3500, kingCaptured);

    CHECK_FALSE(kingCaptured);
    CHECK(board.isCellEmpty({0, 4}));
    REQUIRE(capturedPieces.size() == 1);
    CHECK(capturedPieces[0]->getId() == 2);
    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].piece_id == 1);
}

TEST_CASE("CollisionResolver::resolveCollisions - פרשים פטורים גם מזיהוי swap וגם מחפיפת שהייה") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Knight, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {2, 1}), {2, 1});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {2, 1}, 0, 2000, 1 });
    activeMotions.push_back({ {2, 1}, {0, 0}, 0, 2000, 2 });

    bool kingCaptured = false;
    resolver.resolveCollisions(0, 2000, kingCaptured);

    CHECK(capturedPieces.empty());
    CHECK(activeMotions.size() == 2);
}

// ==================== resolveMovingFriendlyBlocks ====================

TEST_CASE("CollisionResolver::resolveMovingFriendlyBlocks - שני כלים ידידותיים מתכנסים לאותו תא, המאוחר נעצר משבצת לפניו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {0, 6}), {0, 6});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });    // מתחיל ראשון
    activeMotions.push_back({ {0, 6}, {0, 3}, 2500, 5500, 2 }); // מתחיל מאוחר יותר, מגיע ל-(0,4) אחרי הלבן

    resolver.resolveMovingFriendlyBlocks(4000, 4500);

    REQUIRE(activeMotions.size() == 2);
    CHECK(activeMotions[0].piece_id == 1);
    CHECK(activeMotions[0].to == Position{0, 5}); // לא נפגע
    CHECK(activeMotions[1].piece_id == 2);
    CHECK(activeMotions[1].to == Position{0, 5}); // נעצר משבצת לפני ה-(0,4) התפוס
    CHECK(activeMotions[1].arrival_time_ms == 3500);
}

TEST_CASE("CollisionResolver::resolveMovingFriendlyBlocks - שני כלים ידידותיים מחליפים משבצות סמוכות, המאוחר/גבוה-id נעצר ל-LongRest") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {0, 1}), {0, 1});
    board.getPiece({0, 1})->transitionTo(Chess::State::Moving); // המפסיד חייב להיות Moving כדי ש-Moving->LongRest יהיה מעבר חוקי
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 1}, 0, 1000, 1 });
    activeMotions.push_back({ {0, 1}, {0, 0}, 0, 1000, 2 });

    resolver.resolveMovingFriendlyBlocks(0, 1000);

    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].piece_id == 1);
    CHECK(board.getPiece({0, 1})->getState() == Chess::State::LongRest);
    CHECK(cooldownUntilMs[2] == 2000);
}

TEST_CASE("CollisionResolver::resolveMovingFriendlyBlocks - שני כלים ידידותיים שהמסלולים שלהם לא נחצים כלל לא נעצרים") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {5, 0}), {5, 0});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {0, 3}, 0, 3000, 1 });
    activeMotions.push_back({ {5, 0}, {5, 3}, 0, 3000, 2 });

    resolver.resolveMovingFriendlyBlocks(0, 3000);

    REQUIRE(activeMotions.size() == 2);
    CHECK(activeMotions[0].to == Position{0, 3});
    CHECK(activeMotions[1].to == Position{5, 3});
}

TEST_CASE("CollisionResolver::resolveMovingFriendlyBlocks - פרש ידידותי פטור ואינו נעצר") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Knight, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Rook, {2, 1}), {2, 1});
    std::vector<Motion> activeMotions;
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    activeMotions.push_back({ {0, 0}, {2, 1}, 0, 2000, 1 });
    activeMotions.push_back({ {2, 1}, {0, 0}, 0, 2000, 2 });

    resolver.resolveMovingFriendlyBlocks(0, 2000);

    REQUIRE(activeMotions.size() == 2);
    CHECK(activeMotions[0].to == Position{2, 1});
    CHECK(activeMotions[1].to == Position{0, 0});
}

// ==================== resolveStationaryBlocks ====================

TEST_CASE("CollisionResolver::resolveStationaryBlocks - כלי אויב נייח באמצע הנתיב נאכל והתנועה נעצרת שם") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {0, 3}), {0, 3});
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 3000);

    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].to == Position{0, 3});
    CHECK(activeMotions[0].arrival_time_ms == 3000);
}

TEST_CASE("CollisionResolver::resolveStationaryBlocks - כלי ידידותי נייח באמצע הנתיב עוצר את התנועה משבצת לפניו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Rook, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {0, 3}), {0, 3});
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 3000);

    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].to == Position{0, 2});
    CHECK(activeMotions[0].arrival_time_ms == 2000);
}

TEST_CASE("CollisionResolver::resolveStationaryBlocks - חסימה כבר בצעד הראשון מסירה את התנועה ומעבירה ל-LongRest") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Queen, {0, 0}), {0, 0});
    board.getPiece({0, 0})->transitionTo(Chess::State::Moving);
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Pawn, {0, 1}), {0, 1});
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 1000);

    CHECK(activeMotions.empty());
    CHECK(board.getPiece({0, 0})->getState() == Chess::State::LongRest);
    CHECK(cooldownUntilMs[1] == 2000);
}

TEST_CASE("CollisionResolver::resolveStationaryBlocks - כלי אויב שקופץ באמצע הנתיב נחשב חוסם ודאי (לא נדחה ל-resolveCollisions)") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Queen, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Knight, {0, 3}), {0, 3});
    board.getPiece({0, 3})->transitionTo(Chess::State::Jump);
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });     // המלכה חולפת דרך (0,3) ב-t=3000
    activeMotions.push_back({ {0, 3}, {0, 3}, 500, 1500, 2 });   // הפרש קופץ במקום, יש לו Motion פעיל
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 3000);

    REQUIRE(activeMotions.size() == 2);
    CHECK(activeMotions[0].to == Position{0, 3});        // ההחלקה קוצרה לתא שבו הפרש קופץ
    CHECK(activeMotions[0].arrival_time_ms == 3000);
}

TEST_CASE("CollisionResolver::resolveStationaryBlocks - כלי ידידותי שקופץ באמצע הנתיב מפנה את ההחלקה עד אליו, לא עוצר משבצת לפניו") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Queen, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::White, Chess::Kind::Knight, {0, 3}), {0, 3}); // ידידותי
    board.getPiece({0, 3})->transitionTo(Chess::State::Jump);
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {0, 5}, 0, 5000, 1 });     // המלכה חולפת דרך (0,3) ב-t=3000
    activeMotions.push_back({ {0, 3}, {0, 3}, 500, 1500, 2 });   // הפרש הידידותי קופץ במקום
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 3000);

    REQUIRE(activeMotions.size() == 2);
    CHECK(activeMotions[0].to == Position{0, 3});        // מופנה עד אליו, לא עוצר משבצת לפניו כמו חוסם רגיל
    CHECK(activeMotions[0].arrival_time_ms == 3000);
}

TEST_CASE("CollisionResolver::resolveStationaryBlocks - פרש פטור לגמרי ועובר דרך חוסם נייח בלי להיפגע") {
    Board board(8, 8);
    board.addPiece(make(1, Chess::Color::White, Chess::Kind::Knight, {0, 0}), {0, 0});
    board.addPiece(make(2, Chess::Color::Black, Chess::Kind::Pawn, {0, 1}), {0, 1});
    std::vector<Motion> activeMotions;
    activeMotions.push_back({ {0, 0}, {2, 1}, 0, 2000, 1 }); // מהלך פרש טיפוסי
    std::vector<std::shared_ptr<Piece>> capturedPieces;
    std::vector<CaptureObserver*> captureObservers;
    std::unordered_map<int, int> cooldownUntilMs;
    CollisionResolver resolver(board, activeMotions, capturedPieces, captureObservers, cooldownUntilMs);

    resolver.resolveStationaryBlocks(0, 2000);

    REQUIRE(activeMotions.size() == 1);
    CHECK(activeMotions[0].to == Position{2, 1}); // לא שונה
    CHECK(board.getPiece({0, 1})->getKind() == Chess::Kind::Pawn); // החוסם לא נפגע כלל
}
