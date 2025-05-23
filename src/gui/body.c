#include "body.h"
#include "file_manager.h"
#include "item.h"
#include "macro.h"

// #include <time.h>

#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void createBody(Body* b) {
    Body body = { 0 };
    body.panelRec = (Rectangle){ 0 };
    body.panelContentRec = (Rectangle){ 0, 0, 170, 340 };
    body.panelView = (Rectangle){ 0 };
    body.panelScroll = (Vector2){ 0 };

    body.focusedIndex = -1;
    body.showCheckbox = true;
    body.selectedAll = false;
    body.previousSelectedAll = false;
    for (int i = 0; i < 100; i++) {
        body.selected[i] = false;
    }

    *b = body;
}

void updateBody(Body* body, Rectangle currentZeroPosition, FileManager* fileManager) {
    body->currentZeroPosition = currentZeroPosition;

    body->fileManager = fileManager;

    body->panelRec = (Rectangle){
        body->currentZeroPosition.x + 170 + DEFAULT_PADDING,
        body->currentZeroPosition.y + DEFAULT_PADDING * 2 + 24 * 2,
        body->currentZeroPosition.width - 170 - DEFAULT_PADDING,
        body->currentZeroPosition.height - DEFAULT_PADDING * 2 - 24 * 2 };

    if (body->selectedAll) {
        for (int i = 0; i < 100; i++) {
            body->selected[i] = body->selectedAll;
        }
    }
}

void drawBody(Body* body) {
    Tree cursor = body->fileManager->treeCursor;

    sort_children(&cursor);

    cursor = cursor->first_son;

    float headerHeight = 30;
    float rowHeight = 24;

    float colWidths[5] = { 300, 100, 100, 200 };
    float checkboxWidth = body->showCheckbox ? 20 : 0;
    float totalContentWidth = checkboxWidth + colWidths[0] + colWidths[1] + colWidths[2] + colWidths[3];

    body->panelContentRec.width = totalContentWidth;

    GuiScrollPanel(body->panelRec, NULL, body->panelContentRec, &body->panelScroll, &body->panelView);

    BeginScissorMode(body->panelView.x, body->panelView.y, body->panelView.width, body->panelView.height);

    float startY = body->panelRec.y + headerHeight + body->panelScroll.y;
    float startX = body->panelRec.x + body->panelScroll.x;

    int i = 0;
    while (cursor != NULL) {
        drawTableItem(body, cursor, i, startX, body->panelRec.y + headerHeight + body->panelScroll.y, rowHeight, colWidths);

        if ((i + 1) * rowHeight + headerHeight > body->panelContentRec.height)
            body->panelContentRec.height = (i + 1) * rowHeight + headerHeight;
        i++;
        cursor = cursor->next_brother;
    }

    float headerX = body->panelRec.x + body->panelScroll.x;
    drawTableHeader(body, headerX, body->panelRec.y, colWidths);

    EndScissorMode();
}

void drawTableItem(Body* body, Tree subTree, int index, float startX, float startY, float rowHeight, float colWidths[5]) {
    Item item = subTree->item;
    float checkboxWidth = body->showCheckbox ? 28 : 0;
    float totalContentWidth = checkboxWidth + colWidths[0] + colWidths[1] + colWidths[2] + colWidths[3];

    float rowY = startY + index * rowHeight;
    float rowX = startX;

    Rectangle rowRec = { rowX, rowY, totalContentWidth, rowHeight };

    if (CheckCollisionPointRec(GetMousePosition(), rowRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        body->focusedIndex = index;
        if (GetGestureDetected() == GESTURE_DOUBLETAP && item.type == ITEM_FOLDER) {
            goTo(body->fileManager, subTree);
        }
        if (GetGestureDetected() == GESTURE_DOUBLETAP && item.type == ITEM_FILE) {
            windowsOpenWith(item.path);
        }
    }

    Color bgColor = (body->focusedIndex == index) ? Fade(BLUE, 0.2f) : ((index % 2 == 0) ? WHITE : (Color) { 245, 245, 245, 255 });
    DrawRectangleRec(rowRec, bgColor);

    float colX = rowX;

    if (body->showCheckbox) {
        Rectangle checkBox = {
            colX + 7,
            rowY + (rowHeight - 14) / 2,
            14, 14 };

        // PERBAIKAN: Simpan status sebelumnya untuk deteksi perubahan
        bool previousSelected = subTree->item.selected;
        GuiCheckBox(checkBox, NULL, &subTree->item.selected);

        // PERBAIKAN: Hanya update jika ada perubahan status
        if (subTree->item.selected != previousSelected) {
            if (subTree->item.selected) {
                // Tambahkan ke selected list
                selectFile(body->fileManager, subTree->item);
            }
            else {
                // Hapus dari selected list
                deselectFile(body->fileManager, subTree->item);
            }
        }

        colX += checkboxWidth;
    }

    DrawText(TextFormat("%s", item.name), colX + 8, rowY + 6, 10, DARKGRAY);
    colX += colWidths[0];

    DrawText(item.type == ITEM_FILE ? "file" : "folder", colX + 8, rowY + 6, 10, DARKGRAY);
    colX += colWidths[1];

    if (item.size < KB_SIZE) {
        DrawText(TextFormat("%d B", item.size), colX + 8, rowY + 6, 10, DARKGRAY);
    }
    else if (item.size < MB_SIZE) {
        DrawText(TextFormat("%.2f KB", ((float)item.size / KB_SIZE)), colX + 8, rowY + 6, 10, DARKGRAY);
    }
    else if (item.size < GB_SIZE) {
        DrawText(TextFormat("%.2f MB", ((float)item.size / MB_SIZE)), colX + 8, rowY + 6, 10, DARKGRAY);
    }
    else {
        DrawText(TextFormat("%.2f GB", ((float)item.size / GB_SIZE)), colX + 8, rowY + 6, 10, DARKGRAY);
    }

    colX += colWidths[2];

    struct tm* local = localtime(&item.updated_at);

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", local);

    DrawText(TextFormat("%s", buffer), colX + 8, rowY + 6, 10, DARKGRAY);
}

void drawTableHeader(Body* body, float x, float y, float colWidths[]) {
    int fontSize = 10;
    int headerHeight = 30;

    float colX = x;

    DrawRectangleRec((Rectangle) { x, y, colWidths[0] + colWidths[1] + colWidths[2] + colWidths[3] + (body->showCheckbox ? 28 : 0), headerHeight }, LIGHTGRAY);
    DrawRectangleLinesEx((Rectangle) { x, y, colWidths[0] + colWidths[1] + colWidths[2] + colWidths[3] + (body->showCheckbox ? 28 : 0), headerHeight }, 1, DARKGRAY);

    if (body->showCheckbox) {
        Rectangle checkRect = {
            colX + 7,
            y + (headerHeight - 14) / 2,
            14, 14 };
        GuiCheckBox(checkRect, NULL, &body->selectedAll);
        if (body->selectedAll) {
            // insert linkedlist
            // nama var tree current itemnya = subTree
            // Tree cursor = body->fileManager->treeCursor;
            // cursor = cursor->first_son;
            // while (cursor != NULL) {
            //     insert_last(&body->fileManager->selectedItem, cursor);
            //     cursor = cursor->next_brother;
            // }

        }
        else {

        }
        colX += 28;
    }

    DrawRectangleLines(colX, y, colWidths[0], headerHeight, DARKGRAY);
    DrawText("Name", colX + 8, y + 8, fontSize, DARKGRAY);
    colX += colWidths[0];

    DrawRectangleLines(colX, y, colWidths[1], headerHeight, DARKGRAY);
    DrawText("Type", colX + 8, y + 8, fontSize, DARKGRAY);
    colX += colWidths[1];

    DrawRectangleLines(colX, y, colWidths[2], headerHeight, DARKGRAY);
    DrawText("Size", colX + 8, y + 8, fontSize, DARKGRAY);
    colX += colWidths[2];

    DrawRectangleLines(colX, y, colWidths[3], headerHeight, DARKGRAY);
    DrawText("Modified Time", colX + 8, y + 8, fontSize, DARKGRAY);
}
