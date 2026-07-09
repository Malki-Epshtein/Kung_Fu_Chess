#include "Controller.h"

void Controller::handleMouseClick(int x, int y) {
    // 1. קבלת מידות ישירות מהלוח (בלי לעבור דרך המנוע)
    int cols = board->getWidth(); // בהנחה שזה מחזיר עמודות
    int rows = board->getHeight(); // בהנחה שזה מחזיר שורות

    // חישוב גבולות בפיקסלים
    int boardWidth = cols * BoardMapper::CELL_SIZE;
    int boardHeight = rows * BoardMapper::CELL_SIZE;

    // 2. בדיקת גבולות (לחיצה מחוץ ללוח מבטלת בחירה)
    if (x < 0 || y < 0 || x >= boardWidth || y >= boardHeight) {
        selectedPos = nullptr;
        return;
    }

    // 3. תרגום הפיקסלים למיקום לוגי
    Position clickedPos = BoardMapper::mapToPosition(x, y);

    // 4. לוגיקת בחירה וביצוע מהלך
    if (selectedPos == nullptr) {
        // קליק ראשון: בוחר כלי אם קיים בלוח
        if (!board->isCellEmpty(clickedPos)) {
            selectedPos = std::make_shared<Position>(clickedPos);
        }
    }
    else {
        // קליק שני: מנסה להזיז את הכלי שנבחר דרך המנוע
        engine.requestMove(*selectedPos, clickedPos);

        // ניקוי הבחירה לאחר ניסיון המהלך
        selectedPos = nullptr;
    }
}
